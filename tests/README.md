# Simulator tests

An automated test suite for `aprsim`. It runs the simulator headlessly in a pseudo-terminal, types keys, resizes the terminal, and checks how it exits, what it draws, and what programs write to its ports.

| Area | Tests |
|---|---|
| Command-line options | `-v`, `-h`, bad options, `-f` with `-u` (`test_cli.py`) |
| Starting and quitting | `q`, `Q`, F1, SIGINT; quitting while running or waiting for a key (`test_lifecycle.py`) |
| Resizing | many sizes, below the 75x20 minimum, quick resize storms, SIGWINCH alone, resizing in RUN mode; the screen is laid out again each time (`test_resize.py`) |
| The display | title, modes, register pages, memory view, disassembly, symbols and hints files, scrolling, breakpoints (`test_display.py`) |
| Execution | instruction semantics, stepping in STATE and INSTRUCTION mode, breakpoints in RUN mode, keyboard, disk and TTY ports, loading programs (`test_execution.py`) |
| The tools themselves | `test_tooling.py` |

Known simulator bugs have tests marked as expected failures (`xfail(strict=True)`), with the bug described in the reason. When a bug is fixed, its test starts passing, which pytest reports as a failure until the mark is removed.

## Running the tests

You need:
- Python 3.10 or newer, and `tox` (or `pytest`, plus `pyte` for the screen tests);
- the simulator built in this repository (`make` at the top level), configured with `--enable-tty-emulation --enable-port-emulation` for the tests that capture port 7;
- Java, and `apricosasm.jar` in the root of this repository (it is git-ignored), to assemble the test programs.

| Tool | pytest flag | Environment variable | Default |
|---|---|---|---|
| Simulator | `--aprsim NAME-OR-PATH` | `APRSIM` | `src/aprsim` in this repository |
| Assembler | | `APRICOSASM_JAR` | `apricosasm.jar` in the repository root |
| Java | | `JAVA` | `java` on the `PATH` |

The simulator defaults to this repository's build, never the installed `aprsim`, so the tests check the code in front of you. Rebuild with `make` before running them. Write a path as `--aprsim=PATH` (with `=`). If a tool can't be found, the tests that need it are skipped and the reason is shown. Without `pyte`, the tests that read the screen are skipped.

From `apricosim-curses/tests`:

```sh
tox                         # everything, in parallel (about 10 seconds)
tox -e fast                 # only the tests that don't start the simulator
tox -- -k resize            # pass options through to pytest
pytest test_resize.py       # or run pytest directly
pytest --aprsim=/usr/local/bin/aprsim   # test another build
```

## Layout

```
tests/
    README.md
    tox.ini             test environments and pytest settings
    conftest.py         shared fixtures
    test_*.py           the tests
    ptyharness.py       the harness on the command line, for debugging by hand
    simtools/           the test tooling, a Python package
    programs/           assembly test programs
```

### The `simtools` package

| Module | What it does |
|---|---|
| `config` | Finds the simulator, the assembler jar and Java (`Tools`) |
| `harness` | Runs `aprsim` in a pseudo-terminal and performs actions (`Simulator`, `Result`) |
| `assembler` | Assembles test programs with `-sh` (`assemble`, `Program`), and reads and writes PB files (`read_pb`, `pb`) |

**`Simulator(binary, programs=[], args=[], files={}, tty=False, size=(40, 120), cwd=None)`** describes a run: PB files to load (as bytes), extra arguments (such as `-s symbols.sym`), files to put in the working directory first (such as `0.dsk`), whether to capture port 7 through a FIFO, and the starting terminal size. It runs in a fresh temporary directory unless `cwd` (or `run`'s `workdir`) is given. `sim.run(actions)` starts the simulator, performs the actions in order, waits up to 3 seconds for it to exit, and returns a `Result`.

Actions:

| Action | Effect |
|---|---|
| `Size(rows, cols)` | Resize the terminal (TIOCSWINSZ, then SIGWINCH) |
| `Key(text)` | Type text, then keep reading output for 0.2 s. `F1`, `ENTER`, `PAGE_UP` and `PAGE_DOWN` are provided. |
| `Wait(seconds)` | Keep reading output |
| `WaitFor(text, timeout=3)` | Wait until the screen shows the text |
| `WaitForTty(count, timeout=3)` | Wait until `count` bytes have arrived on port 7 |
| `Signal(signum)` | Send a signal, e.g. SIGWINCH without a size change |
| `Snapshot(name)` | Save the screen as `result.snapshots[name]` |

A `Result` has:
- **`returncode`:** `None` if the simulator was still running at the end (the harness then killed it), negative for a signal. Also `exited_cleanly`, `still_running`, `signal` and `status` (e.g. `killed by signal 11 (SIGSEGV)`).
- **`gone_before`:** the first action that couldn't run because the simulator had already gone. `describe()` combines this with the status, for assertion messages.
- **`screen`:** the final screen as a list of rows, and `snapshots`. Both need `pyte`, a terminal emulator that replays the curses output.
- **`tty`:** the bytes written to port 7.
- **`log`:** the raw terminal output, escape sequences included.

**The fixtures** in `conftest.py`:

| Fixture | What it gives you |
|---|---|
| `aprsim` | The simulator binary under test |
| `tty_aprsim` | The same, for tests that capture port 7 (skipped without TTY emulation) |
| `program("name")` | `programs/name.asm`, assembled once per session: `binary`, `code`, `origin`, `symbols_file`, `hints_file`, `address("label")` |
| `ready` | An action that waits until the simulator has drawn its first screen |

A typical test:

```python
from simtools import F1, Key, Simulator, WaitForTty

def test_hello(tty_aprsim, ready, program):
    result = Simulator(tty_aprsim, [program("hello").binary], tty=True).run(
        [ready, Key("mm "), WaitForTty(5), Key(F1)])     # RUN mode, run, wait for 5 bytes, quit
    assert result.exited_cleanly, result.describe()
    assert result.tty == b"\x7f\x00HI\n"
```

Mark tests that start the simulator `simulator` (excluded by `tox -e fast`), and tests that read the screen `screen`.

### Test programs

`programs/*.asm` are small standalone programs, assembled with `apricosasm.jar -sh`. Most write their results to port 7, which the tests read through the FIFO, and end in a sleep loop, since there is no HALT. Each says at the top what it writes. To add one, put it in `programs/` and use `program("name")`.

### Things that affect timing

In RUN mode, the simulator only passes a key to the program while the program is blocked in `PRTin 2`. Keys that arrive at other times are swallowed, a swallowed space pauses the run, and `q` is ignored, so quit with F1. The tests wait for a response (`WaitFor`, `WaitForTty`) rather than for fixed times wherever they can.

### How the harness works

The simulator runs on the slave side of a pseudo-terminal, with `TERM=xterm`; the harness acts as the terminal on the master side.
- **Typing** is writing to the master.
- **Resizing** sets the size with `TIOCSWINSZ` and sends `SIGWINCH`. The simulator is started without a controlling terminal (so the harness is safe to use from threaded processes such as pytest-xdist workers), so the kernel wouldn't send the signal itself.
- **Reading:** the master must be read continuously, or the simulator blocks once the PTY buffer fills.
- **The screen** is modelled with `pyte`. The model also handles REP (`CSI n b`), which ncurses uses for long lines in some locales.
- **Port 7:** for `-f`, the harness creates the FIFO and opens its read end before starting the simulator. The simulator blocks in `open()` until a reader exists.

When the simulator dies, reading the master fails with `EIO`. The PTY also reaches end-of-file a moment before the exited process can be reaped, so the harness polls for the exit status for a few seconds rather than checking once.

## Debugging by hand

`ptyharness.py` runs the harness from the command line with the same actions as a comma-separated list. It works on any curses program and in any directory:

```sh
H=~/dev/apricos/apricosim-curses/tests/ptyharness.py
python3 $H "size:40x120,wait:1,size:30x100,size:10x40,winch,key:q" -- ../src/aprsim
python3 $H --screen "size:40x120,waitfor:Mode,key:d,snapshot:disasm,key:q" -- ../src/aprsim prog.bin
python3 $H --tty out.tty "size:40x120,wait:1,key:mm ,wait:2,key:\x1bOP" -- aprsim hello.bin
```

Run `python3 $H` with no arguments for the full usage. It prints how the program ended and exits with 0 for a clean exit, 1 for death by a signal and 2 if the program was still running at the end, so it can be used in shell loops. `--screen`, `waitfor:` and `snapshot:` need `pyte`.
