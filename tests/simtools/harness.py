"""Driving aprsim inside a pseudo-terminal.

aprsim is a full-screen curses program. A pseudo-terminal (PTY) gives it a
terminal without a window: the simulator runs on the slave side, and the
harness sits on the master side, acting as the terminal emulator.

- Typing is writing bytes to the master: writing ``q`` is pressing ``q``.
- Resizing is ``ioctl(master, TIOCSWINSZ)``, exactly what a terminal emulator
  does when its window is dragged, followed by ``SIGWINCH`` (which the kernel
  would send to the terminal's foreground process).
- The master must be read continuously, or the simulator blocks once the PTY
  buffer fills. What it reads is fed to a terminal emulator (``pyte``, if
  installed), so tests can look at the screen.
- With ``tty=True``, everything the program writes to port 7 goes to a FIFO,
  which the harness reads as well.

When the simulator dies, reading the master fails with EIO (nothing holds the
slave open any more). That is a symptom, not a harness bug. The PTY also
reaches end-of-file a moment before the exited process can be reaped, so the
harness polls for the exit status rather than checking once.

Example::

    sim = Simulator(binary, programs=[hello])
    result = sim.run([WaitFor("Mode: STATE"), Key("mm "), WaitForTty(5), Key(F1)])
    assert result.exited_cleanly
    assert result.tty == b"\\x7f\\x00HI\\n"
"""

from __future__ import annotations

import fcntl
import os
import select
import signal
import struct
import subprocess
import tempfile
import termios
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable, Union

# Keys as xterm sends them (the harness runs aprsim with TERM=xterm)
F1 = "\x1bOP"
PAGE_UP = "\x1b[5~"
PAGE_DOWN = "\x1b[6~"
ENTER = "\r"

DEFAULT_SIZE = (40, 120)    # rows, columns
KEY_WAIT = 0.2              # seconds to keep draining after a key, by default
SIZE_WAIT = 0.3             # ... after a resize or a signal
EXIT_WAIT = 3.0             # seconds to wait for the simulator to exit after the last action
TYPE_CHUNK = 64             # bytes typed at a time (see _Run._type)


# ---------------------------------------------------------------- actions

@dataclass(frozen=True)
class Size:
    """Set the terminal size (TIOCSWINSZ); the kernel sends SIGWINCH."""
    rows: int
    cols: int
    wait: float = SIZE_WAIT


@dataclass(frozen=True)
class Key:
    """Type ``text`` (all at once), then keep draining output for ``wait`` seconds."""
    text: str | bytes
    wait: float = KEY_WAIT


@dataclass(frozen=True)
class Wait:
    """Keep draining output for ``seconds``."""
    seconds: float


@dataclass(frozen=True)
class WaitFor:
    """Wait until the screen shows ``text`` (at most ``timeout`` seconds). Needs pyte."""
    text: str
    timeout: float = 3.0


@dataclass(frozen=True)
class WaitForTty:
    """Wait until at least ``count`` bytes have arrived on port 7 (at most ``timeout`` seconds)."""
    count: int
    timeout: float = 3.0


@dataclass(frozen=True)
class Signal:
    """Send ``signum`` to the simulator, e.g. SIGWINCH without a size change."""
    signum: int
    wait: float = SIZE_WAIT


@dataclass(frozen=True)
class Snapshot:
    """Save the current screen as ``result.snapshots[name]``. Needs pyte."""
    name: str


Action = Union[Size, Key, Wait, WaitFor, WaitForTty, Signal, Snapshot]


def parse_actions(spec: str) -> list[Action]:
    """Parse the command-line action syntax, a comma-separated list:

    ``size:ROWSxCOLS``, ``key:TEXT`` (escapes such as ``\\r`` and ``\\x1bOP``
    are decoded), ``wait:SECONDS``, ``waitfor:TEXT``, ``tty:COUNT``,
    ``winch``, ``signal:NAME``, ``snapshot:NAME``.
    """
    actions: list[Action] = []
    for item in spec.split(","):
        name, _, arg = item.partition(":")
        if name == "size":
            rows, cols = (int(n) for n in arg.lower().split("x"))
            actions.append(Size(rows, cols))
        elif name == "key":
            actions.append(Key(arg.encode().decode("unicode_escape").encode("latin-1")))
        elif name == "wait":
            actions.append(Wait(float(arg)))
        elif name == "waitfor":
            actions.append(WaitFor(arg))
        elif name == "tty":
            actions.append(WaitForTty(int(arg)))
        elif name == "winch":
            actions.append(Signal(signal.SIGWINCH))
        elif name == "signal":
            actions.append(Signal(signal.Signals[arg.upper() if arg.upper().startswith("SIG") else "SIG" + arg.upper()]))
        elif name == "snapshot":
            actions.append(Snapshot(arg))
        else:
            raise ValueError(f"unknown action: {item}")
    return actions


# ---------------------------------------------------------------- results

@dataclass
class Result:
    """How a run ended, and what the simulator showed and printed."""

    returncode: int | None       # None: still running at the end (the harness killed it)
    gone_before: Action | None   # the first action that couldn't run because the simulator had gone
    log: bytes                   # everything written to the terminal (curses output, escapes included)
    tty: bytes                   # everything written to port 7 (only with tty=True)
    screen: list[str] | None     # the final screen, one string per row (None without pyte)
    snapshots: dict[str, list[str]] = field(default_factory=dict)
    workdir: Path | None = None  # where it ran, if kept

    @property
    def still_running(self) -> bool:
        return self.returncode is None

    @property
    def signal(self) -> int | None:
        """The signal that killed the simulator, if one did."""
        return -self.returncode if self.returncode is not None and self.returncode < 0 else None

    @property
    def exited_cleanly(self) -> bool:
        return self.returncode == 0

    @property
    def status(self) -> str:
        if self.returncode is None:
            return "still running (killed by harness)"
        if self.returncode < 0:
            return f"killed by signal {-self.returncode} ({signal.Signals(-self.returncode).name})"
        return f"exited with code {self.returncode}"

    def describe(self) -> str:
        """The status, plus where the simulator went away; for assertion messages."""
        text = self.status
        if self.gone_before is not None:
            text += f"; program gone before {self.gone_before}"
        return text

    def screen_text(self) -> str:
        return "\n".join(self.screen or [])


# ---------------------------------------------------------------- the simulator

class Simulator:
    """One way of running aprsim: a binary, its arguments and files.

    ``programs`` are PB files (bytes) passed as positional arguments, in
    order. ``files`` are extra files to write to the working directory first
    (e.g. ``{"0.dsk": image}``), and ``args`` extra arguments placed before the
    programs and after ``-f`` (e.g. ``["-s", "symbols.sym"]``). With ``tty=True`` a FIFO is
    created (in ``fifo_dir``, by default the working directory) and passed
    with ``-f``, and port 7 output is captured.

    The simulator runs in ``cwd`` if given, else in a fresh temporary
    directory per run (or ``run``'s ``workdir``).
    """

    def __init__(self, binary: str | os.PathLike, programs: Iterable[bytes] = (), *,
                 args: Iterable[str] = (), files: dict[str, bytes] | None = None,
                 tty: bool = False, size: tuple[int, int] = DEFAULT_SIZE,
                 cwd: Path | None = None, fifo_dir: Path | None = None):
        self.binary = str(binary)
        self.programs = list(programs)
        self.args = list(args)
        self.files = dict(files or {})
        self.tty = tty
        self.size = size
        self.cwd = cwd
        self.fifo_dir = fifo_dir

    def run(self, actions: Iterable[Action], workdir: Path | None = None,
            exit_wait: float = EXIT_WAIT) -> Result:
        """Start the simulator, perform ``actions`` in order, and wait for it to exit.

        If it is still running ``exit_wait`` seconds after the last action, it
        is killed and the result says so. Without ``workdir`` it runs in a
        temporary directory that is removed afterwards.
        """
        actions = list(actions)
        workdir = workdir or self.cwd
        if workdir is not None:
            workdir.mkdir(parents=True, exist_ok=True)
            result = self._run_in(workdir, actions, exit_wait)
            result.workdir = workdir
            return result
        with tempfile.TemporaryDirectory(prefix="aprsim-test-") as scratch:
            return self._run_in(Path(scratch), actions, exit_wait)

    def command(self, fifo: str | None = None) -> list[str]:
        command = [self.binary]
        if fifo:
            command += ["-f", fifo]
        return command + self.args + [f"program{i}.bin" for i in range(len(self.programs))]

    def _run_in(self, directory: Path, actions: list[Action], exit_wait: float) -> Result:
        for name, data in self.files.items():
            (directory / name).write_bytes(data)
        for index, program in enumerate(self.programs):
            (directory / f"program{index}.bin").write_bytes(program)

        fifo_fd = keep_open = None
        if self.tty:
            fifo = (self.fifo_dir or directory) / "tty.fifo"
            if fifo.exists():
                fifo.unlink()
            os.mkfifo(fifo)
            # Opening the read end non-blocking succeeds at once and lets
            # aprsim's blocking open of the write end complete. Holding a write
            # end ourselves stops the FIFO reporting end-of-file before aprsim
            # connects, which would make select() spin.
            fifo_fd = os.open(fifo, os.O_RDONLY | os.O_NONBLOCK)
            keep_open = os.open(fifo, os.O_WRONLY | os.O_NONBLOCK)
        try:
            run = _Run(self.command(str(fifo) if self.tty else None), directory, self.size, fifo_fd)
            return run.perform(actions, exit_wait)
        finally:
            for fd in (keep_open, fifo_fd):
                if fd is not None:
                    os.close(fd)


class _Run:
    """One running simulator: the PTY, the screen model and the captured output."""

    def __init__(self, command: list[str], cwd: Path, size: tuple[int, int], fifo_fd: int | None):
        self.fifo_fd = fifo_fd
        self.log = bytearray()
        self.tty = bytearray()
        self.alive = True
        self.screen = _new_screen(*size)

        # subprocess forks and execs in C, which is safe in a multi-threaded
        # process (pytest-xdist workers are); pty.fork() runs Python in the child.
        # The price is that the PTY doesn't become the simulator's controlling
        # terminal, so resizes must signal it explicitly (see Size).
        self.fd, terminal = os.openpty()
        os.set_blocking(self.fd, False)     # see _type
        self._set_size(*size)
        try:
            self.process = subprocess.Popen(command, cwd=cwd, env={**os.environ, "TERM": "xterm"},
                                            stdin=terminal, stdout=terminal, stderr=terminal,
                                            start_new_session=True)
        finally:
            os.close(terminal)

    def perform(self, actions: list[Action], exit_wait: float) -> Result:
        gone_before = None
        snapshots: dict[str, list[str]] = {}
        try:
            for action in actions:
                if not self.alive:
                    gone_before = action
                    break
                self._do(action, snapshots)
            deadline = time.time() + exit_wait
            while self.process.poll() is None and time.time() < deadline:
                self._drain(0.02)
            self._read_tty()
            returncode = self.process.poll()
        finally:
            if self.process.poll() is None:
                self.process.kill()
                self.process.wait()
            os.close(self.fd)
        return Result(returncode=returncode, gone_before=gone_before, log=bytes(self.log),
                      tty=bytes(self.tty), screen=_rows(self.screen), snapshots=snapshots)

    def _do(self, action: Action, snapshots: dict[str, list[str]]) -> None:
        if isinstance(action, Size):
            self._set_size(action.rows, action.cols)
            # The simulator has no controlling terminal (see __init__), so the
            # kernel doesn't send SIGWINCH for the new size; send it ourselves.
            try:
                os.kill(self.process.pid, signal.SIGWINCH)
            except ProcessLookupError:
                self.alive = False
            if self.screen is not None:
                self.screen[0].resize(action.rows, action.cols)
            self._drain(action.wait)
        elif isinstance(action, Key):
            data = action.text.encode("latin-1") if isinstance(action.text, str) else action.text
            self._type(data)
            self._drain(action.wait)
        elif isinstance(action, Wait):
            self._drain(action.seconds)
        elif isinstance(action, WaitFor):
            if self.screen is None:
                raise RuntimeError("WaitFor needs the pyte package to model the screen")
            self._drain(action.timeout, lambda: action.text in "\n".join(_rows(self.screen)))
        elif isinstance(action, WaitForTty):
            self._drain(action.timeout, lambda: len(self.tty) >= action.count)
        elif isinstance(action, Signal):
            try:
                os.kill(self.process.pid, action.signum)
            except ProcessLookupError:
                self.alive = False
            self._drain(action.wait)
        elif isinstance(action, Snapshot):
            if self.screen is None:
                raise RuntimeError("Snapshot needs the pyte package to model the screen")
            snapshots[action.name] = _rows(self.screen)
        else:
            raise TypeError(f"not an action: {action!r}")

    def _set_size(self, rows: int, cols: int) -> None:
        fcntl.ioctl(self.fd, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))

    def _type(self, data: bytes) -> None:
        """Write ``data`` to the terminal in small pieces, reading output in between.

        Writing a long sequence in one go can deadlock: the write blocks once
        the PTY's input buffer is full, while the simulator blocks drawing,
        because its output isn't being read. The master is non-blocking (macOS
        reports a PTY writable even when a write would block), so a full
        buffer just means reading some output and trying again.
        """
        while data and self.alive:
            readable, writable, _ = select.select([self.fd], [self.fd], [], 0.05)
            if readable:
                self._read_terminal()
            if writable and self.alive:
                try:
                    written = os.write(self.fd, data[:TYPE_CHUNK])
                except BlockingIOError:
                    time.sleep(0.005)
                    continue
                except OSError:
                    self.alive = False
                    return
                data = data[written:]

    def _drain(self, seconds: float, done=None) -> None:
        """Read the terminal and the FIFO for ``seconds``, or until ``done()`` is true."""
        end = time.time() + seconds
        while self.alive and time.time() < end:
            if done is not None and done():
                return
            sources = [self.fd] + ([self.fifo_fd] if self.fifo_fd is not None else [])
            ready = select.select(sources, [], [], min(0.05, max(0.0, end - time.time())))[0]
            if self.fifo_fd in ready:
                self._read_tty()
            if self.fd in ready:
                self._read_terminal()
        self._read_tty()

    def _read_terminal(self) -> None:
        """Read what the simulator drew (the terminal must be readable)."""
        try:
            data = os.read(self.fd, 65536)
        except BlockingIOError:
            return
        except OSError:         # EIO: nothing holds the slave open any more
            data = b""
        if not data:
            self.alive = False
        else:
            self.log.extend(data)
            if self.screen is not None:
                self.screen[1].feed(data)

    def _read_tty(self) -> None:
        if self.fifo_fd is None:
            return
        while True:
            try:
                chunk = os.read(self.fifo_fd, 65536)
            except BlockingIOError:
                return
            if not chunk:
                return
            self.tty.extend(chunk)


def _new_screen(rows: int, cols: int):
    """A (pyte.Screen, pyte.ByteStream) pair, or None if pyte isn't installed."""
    try:
        import pyte
    except ImportError:
        return None

    class Screen(pyte.Screen):
        """pyte's screen, plus REP (CSI n b: repeat the last character n times).

        ncurses draws long lines with REP in some locales (the C locale, as
        under tox), and pyte ignores it.
        """
        last = " "

        def draw(self, data: str) -> None:
            if data:
                self.last = data[-1]
            super().draw(data)

        def repeat_last(self, count: int = 1, *args, **kwargs) -> None:
            super().draw(self.last * max(count, 1))

    class ByteStream(pyte.ByteStream):
        csi = {**pyte.ByteStream.csi, "b": "repeat_last"}

    screen = Screen(cols, rows)
    stream = ByteStream(screen)
    # pyte ignores charset switches in UTF-8 mode, so curses' line drawing
    # (ESC ( 0) would show as "lqqk". The simulator only writes ASCII.
    stream.use_utf8 = False
    return screen, stream


def _rows(screen) -> list[str] | None:
    return None if screen is None else [line.rstrip() for line in screen[0].display]
