"""Shared pytest fixtures for the simulator tests.

Every test runs its own simulator, so the suite runs in parallel with
pytest-xdist (``pytest -n auto``, as tox does).
"""

from __future__ import annotations

from pathlib import Path
from typing import Callable

import pytest

from simtools import MissingTool, Program, Tools, Wait, WaitFor, assemble


def pytest_addoption(parser):
    parser.addoption("--aprsim", metavar="NAME-OR-PATH", default=None,
                     help="the simulator: a path, or a binary name on the PATH "
                          "(default: $APRSIM, then src/aprsim in this repository)")


def pytest_configure(config):
    config.addinivalue_line("markers", "simulator: runs aprsim (about a second each)")
    config.addinivalue_line("markers", "screen: reads the simulator's screen (needs pyte)")


def pytest_collection_modifyitems(config, items):
    try:
        import pyte  # noqa: F401
    except ImportError:
        skip = pytest.mark.skip(reason="reading the screen needs pyte (pip install pyte, or use tox)")
        for item in items:
            if "screen" in item.keywords:
                item.add_marker(skip)


def _require(get: Callable):
    try:
        return get()
    except MissingTool as missing:
        pytest.skip(str(missing))


@pytest.fixture(scope="session")
def tools(request) -> Tools:
    return Tools(aprsim=request.config.getoption("--aprsim"))


@pytest.fixture(scope="session")
def aprsim(tools) -> Path:
    """The simulator binary under test."""
    return _require(lambda: tools.simulator)


@pytest.fixture(scope="session")
def tty_aprsim(tools, aprsim) -> Path:
    """The simulator, for tests that capture port 7 (it must take ``-f``)."""
    if not tools.has_tty_emulation:
        pytest.skip(f"{aprsim} has no TTY emulation (configure with --enable-tty-emulation "
                    "--enable-port-emulation)")
    return aprsim


@pytest.fixture(scope="session")
def program(tools, tmp_path_factory) -> Callable[[str], Program]:
    """Assembles a program from ``tests/programs`` by name (``program("hello")``), once per session."""
    cache: dict[str, Program] = {}

    def get(name: str) -> Program:
        if name not in cache:
            _require(lambda: (tools.java, tools.assembler_jar))
            cache[name] = assemble(tools, f"{name}.asm", tmp_path_factory.mktemp(name))
        return cache[name]

    return get


@pytest.fixture(scope="session")
def ready():
    """An action that waits until the simulator has drawn its first screen."""
    try:
        import pyte  # noqa: F401
    except ImportError:
        return Wait(1.0)
    return WaitFor("Mode:")
