"""Where the simulator under test and the assembler are.

==================  ================================  ===========================================
Variable            Meaning                           Default
==================  ================================  ===========================================
``APRSIM``          The simulator                     ``src/aprsim`` in this repository
``APRICOSASM_JAR``  The assembler jar                 ``apricosasm.jar`` in the repository root
``JAVA``            The Java launcher                 ``java`` on the ``PATH``
==================  ================================  ===========================================

The simulator defaults to this repository's build, never the installed
``aprsim``, so the tests check the code in front of you. It may be given as a
path or as a binary name looked up on the ``PATH``. With pytest,
``--aprsim NAME-OR-PATH`` takes precedence over the variable.
"""

from __future__ import annotations

import os
import shutil
import subprocess
from dataclasses import dataclass
from functools import cached_property
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
BUILT_SIMULATOR = REPO / "src" / "aprsim"
ASSEMBLER_JAR = REPO / "apricosasm.jar"
TESTS_DIR = REPO / "tests"
PROGRAMS_DIR = TESTS_DIR / "programs"


class MissingTool(RuntimeError):
    """A tool the tests need could not be found."""


@dataclass(frozen=True)
class Tools:
    """Locations of the external tools; ``None`` means "use the variable, then the default"."""

    aprsim: str | None = None
    jar: str | None = None
    java_launcher: str | None = None

    @cached_property
    def simulator(self) -> Path:
        choice = self.aprsim or os.environ.get("APRSIM")
        if not choice:
            if not BUILT_SIMULATOR.is_file():
                raise MissingTool(f"{BUILT_SIMULATOR} is not built: run make in {REPO}, "
                                  "or choose a simulator with --aprsim or APRSIM")
            return BUILT_SIMULATOR
        path = Path(choice).expanduser()
        if os.sep in choice or path.is_file():
            if not path.is_file():
                raise MissingTool(f"no simulator at {path} (from --aprsim or APRSIM)")
            return path.resolve()
        found = shutil.which(choice)
        if not found:
            raise MissingTool(f"{choice} is not on the PATH (from --aprsim or APRSIM)")
        return Path(found)

    @cached_property
    def simulator_usage(self) -> str:
        """What ``aprsim -h`` prints."""
        return subprocess.run([str(self.simulator), "-h"], capture_output=True, text=True, timeout=10).stdout

    @property
    def has_tty_emulation(self) -> bool:
        """Whether the simulator takes ``-f`` (built with ``--enable-tty-emulation``)."""
        return "--fifo_tty_file" in self.simulator_usage

    @cached_property
    def java(self) -> str:
        java = self.java_launcher or os.environ.get("JAVA") or shutil.which("java")
        if not java:
            raise MissingTool("java was not found; install a JDK or set JAVA")
        return java

    @cached_property
    def assembler_jar(self) -> Path:
        jar = Path(self.jar or os.environ.get("APRICOSASM_JAR") or ASSEMBLER_JAR).expanduser()
        if not jar.is_file():
            raise MissingTool(f"the assembler was not found at {jar}; put apricosasm.jar in the repository root "
                              "or set APRICOSASM_JAR")
        return jar.resolve()
