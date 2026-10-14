"""Assembling test programs with apricosasm.

Test programs live in ``tests/programs``. Each is assembled on its own, with
symbols (``-s``) and disassembly hints (``-h``), in a scratch directory::

    program = assemble(tools, "hello.asm", workdir)
    sim = Simulator(tools.simulator, [program.binary])
"""

from __future__ import annotations

import re
import shutil
import struct
import subprocess
from dataclasses import dataclass
from pathlib import Path

from .config import PROGRAMS_DIR, Tools


class AssemblyError(RuntimeError):
    """The assembler reported an error."""


@dataclass(frozen=True)
class Program:
    """An assembled program: the PB file, and the assembler's symbols and hints files."""

    name: str
    binary: bytes
    symbols_file: bytes
    hints_file: bytes

    @property
    def origin(self) -> int:
        return read_pb(self.binary)[0]

    @property
    def code(self) -> bytes:
        return read_pb(self.binary)[1]

    @property
    def symbols(self) -> dict[str, int]:
        """Label (lowercase, as the assembler writes them) to address."""
        symbols = {}
        for line in self.symbols_file.decode().splitlines():
            address, _, label = line.strip().partition(":")
            if address.isdigit():
                symbols[label] = int(address)
        return symbols

    def address(self, label: str) -> int:
        return self.symbols[label.lower()]


def assemble(tools: Tools, source: str | Path, workdir: Path) -> Program:
    """Assemble ``source`` (a file name in ``tests/programs``, or a path) in ``workdir``."""
    source = Path(source)
    if not source.is_absolute() and not source.exists():
        source = PROGRAMS_DIR / source
    workdir.mkdir(parents=True, exist_ok=True)
    target = workdir / source.name
    shutil.copy(source, target)
    result = subprocess.run([tools.java, "-jar", str(tools.assembler_jar), "-sh", target.name],
                            cwd=workdir, capture_output=True, text=True)
    output = result.stdout + result.stderr
    if result.returncode != 0 or re.search(r"^Error assembling|FAIL", output, re.MULTILINE):
        raise AssemblyError(f"{source.name}:\n{output}")
    return Program(name=source.stem,
                   binary=target.with_suffix(".bin").read_bytes(),
                   symbols_file=(workdir / "symbols.sym").read_bytes(),
                   hints_file=(workdir / "hints.dbg").read_bytes())


def read_pb(data: bytes) -> tuple[int, bytes]:
    """Split a PB file into its origin and its bytes."""
    if data[:2] != b"PB":
        raise ValueError("not a PB file")
    size, origin = struct.unpack_from("<HH", data, 2)
    return origin, data[6:6 + size]


def pb(code: bytes, origin: int = 0, magic: bytes = b"PB") -> bytes:
    """A PB file that loads ``code`` at ``origin`` (``magic`` can be changed to make a bad file)."""
    return magic + struct.pack("<HH", len(code), origin) + code
