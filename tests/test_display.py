"""What the simulator shows, and the keys that change the view."""

import re
import subprocess

import pytest

from simtools import PAGE_DOWN, PAGE_UP, Key, Simulator, Snapshot, Wait, WaitFor

pytestmark = [pytest.mark.simulator, pytest.mark.screen]


def memory_rows(screen):
    """The memory panel's rows, as (marker, address, value, rest of the row)."""
    rows = []
    for line in screen:
        match = re.match(r"│([> ])\s+(@?)0x([0-9A-F]{4}):\s+([0-9A-F]{2})(.*?)│", line)
        if match:
            rows.append(((match.group(1) + match.group(2)).strip(), int(match.group(3), 16), int(match.group(4), 16),
                         match.group(5)))
    return rows


def view(aprsim, ready, keys, programs=(), args=(), files=None):
    """The screen after typing each of ``keys`` (waiting a little after each)."""
    actions = [ready] + [Key(k) for k in keys] + [Wait(0.2), Snapshot("view"), Key("q")]
    result = Simulator(aprsim, programs, args=args, files=files).run(actions)
    assert result.exited_cleanly, result.describe()
    return result.snapshots["view"]


def test_title_shows_the_version(aprsim, ready):
    version = subprocess.run([str(aprsim), "-v"], capture_output=True, text=True).stdout.strip()
    assert f" {version} " in view(aprsim, ready, [])[0]


@pytest.mark.parametrize("presses, mode", [(0, "STATE"), (1, "INSTRUCTION"), (2, "RUN"), (3, "STATE")])
def test_m_cycles_the_mode(aprsim, ready, presses, mode):
    screen = view(aprsim, ready, ["m"] * presses)
    assert f"Mode: {mode}" in screen[-2]


@pytest.mark.parametrize("keys, page", [([], 1), (["r"], 2), (["r", "r"], 1), (["R"], 2)])
def test_register_pages(aprsim, ready, keys, page):
    screen = view(aprsim, ready, keys)
    assert f"Registers ({page}/2)" in screen[0]
    assert ("CPU Flags" in "\n".join(screen)) == (page == 2)


def test_memory_shows_the_loaded_program(aprsim, ready, program):
    hello = program("hello")
    rows = memory_rows(view(aprsim, ready, [], [hello.binary]))
    assert [value for _, _, value, _ in rows[:len(hello.code)]] == list(hello.code)
    assert rows[0][0] == ">"       # the program counter is at 0


def test_d_toggles_disassembly(aprsim, ready, program):
    hello = program("hello").binary
    assert "ASET 1" in memory_rows(view(aprsim, ready, ["d"], [hello]))[0][3]
    assert "ASET" not in memory_rows(view(aprsim, ready, ["d", "d"], [hello]))[0][3]


def test_symbols_are_shown(aprsim, ready, program):
    hello = program("hello")
    spin = hello.address("spin")
    rows = memory_rows(view(aprsim, ready, [], [hello.binary], args=["-s", "symbols.sym"],
                            files={"symbols.sym": hello.symbols_file}))
    assert rows[spin][3].split()[-1] == "spin"


def test_hints_file_loads(aprsim, ready, program):
    hello = program("hello")
    rows = memory_rows(view(aprsim, ready, ["d"], [hello.binary], args=["-d", "hints.dbg"],
                            files={"hints.dbg": hello.hints_file}))
    assert "ASET 1" in rows[0][3]


@pytest.mark.parametrize("keys, first", [(["j"], 1), (["j", "j", "k"], 1), (["k"], 0),
                                         ([PAGE_DOWN], 0x100), ([PAGE_DOWN, PAGE_DOWN, PAGE_UP], 0x100),
                                         (["j", "j", "p"], 0)],
                         ids=["j", "jjk", "k-at-top", "pgdn", "pgdn-pgdn-pgup", "p"])
def test_memory_scrolling(aprsim, ready, keys, first):
    assert memory_rows(view(aprsim, ready, keys))[0][1] == first


def test_b_toggles_a_breakpoint(aprsim, ready):
    rows = memory_rows(view(aprsim, ready, ["j", "b", "k"]))
    assert [marker for marker, _, _, _ in rows[:3]] == [">", "@", ""]
    rows = memory_rows(view(aprsim, ready, ["b", "b"]))
    assert rows[0][0] == ">"


def test_n_finds_the_next_breakpoint(aprsim, ready):
    keys = ["j"] * 5 + ["b"] + [PAGE_UP] + ["n"]
    assert memory_rows(view(aprsim, ready, keys))[0][1] == 5
