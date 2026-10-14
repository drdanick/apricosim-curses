"""Resizing the terminal: no crashes, and the screen is laid out again at the new size.

The October 2026 crashes (a double delwin in destroygui, and curses calls in
the SIGWINCH handler) were found with these scenarios.
"""

import signal

import pytest

from simtools import F1, Key, Signal, Simulator, Size, Snapshot, Wait, WaitFor

pytestmark = [pytest.mark.simulator, pytest.mark.screen]


def assert_laid_out(screen, rows, cols):
    """The panels fill the screen exactly: borders in every corner, the status line inside."""
    assert len(screen) == rows
    top, bottom = screen[0], screen[rows - 1]
    assert top.startswith("┌─ Registers") and top.endswith("┐") and len(top) == cols, top
    assert bottom.startswith("└") and bottom.endswith("┘") and len(bottom) == cols, bottom
    assert "Mode: " in screen[rows - 2]


@pytest.mark.parametrize("rows, cols", [(50, 160), (31, 100), (30, 100), (25, 80), (20, 75)])
def test_resize_lays_out_again(aprsim, ready, rows, cols):
    result = Simulator(aprsim).run([ready, Size(rows, cols), Wait(0.3),
                                    Snapshot("resized"), Key("q")])
    assert result.exited_cleanly, result.describe()
    assert_laid_out(result.snapshots["resized"], rows, cols)


@pytest.mark.parametrize("rows, pages", [(31, 2), (30, 4)])
def test_register_layout_follows_height(aprsim, ready, rows, pages):
    result = Simulator(aprsim).run([ready, Size(rows, 100), Wait(0.3), Snapshot("resized"), Key("q")])
    assert f"Registers (1/{pages})" in result.snapshots["resized"][0]


def test_register_page_is_kept_across_layouts(aprsim, ready):
    # Page 4 of 4 is page index 3, which is page 2 of 2 in the tall layout
    result = Simulator(aprsim, size=(25, 80)).run(
        [ready, Key("rrr"), WaitFor("Registers (4/4)"), Size(40, 120), Wait(0.3), Snapshot("tall"), Key("q")])
    assert "Registers (2/2)" in result.snapshots["tall"][0]


def test_below_minimum_size(aprsim, ready):
    result = Simulator(aprsim).run([ready, Size(10, 40), Wait(0.3), Size(40, 120), Wait(0.3),
                                    Snapshot("restored"), Key("q")])
    assert result.exited_cleanly, result.describe()
    assert_laid_out(result.snapshots["restored"], 40, 120)


def test_many_quick_resizes(aprsim, ready):
    sizes = [(30 + i % 20, 80 + (i * 7) % 80) for i in range(40)]
    actions = [ready] + [Size(r, c, wait=0.01) for r, c in sizes] + [Size(40, 120), Wait(0.5),
                                                                       Snapshot("final"), Key("q")]
    result = Simulator(aprsim).run(actions)
    assert result.exited_cleanly, result.describe()
    assert_laid_out(result.snapshots["final"], 40, 120)


def test_sigwinch_without_a_size_change(aprsim, ready):
    result = Simulator(aprsim).run([ready] + [Signal(signal.SIGWINCH)] * 3 + [Snapshot("after"), Key("q")])
    assert result.exited_cleanly, result.describe()
    assert_laid_out(result.snapshots["after"], 40, 120)


def test_resize_while_running(aprsim, ready, program):
    result = Simulator(aprsim, [program("steps").binary]).run(
        [ready, Key("mm "), Wait(0.3), Size(30, 100), Wait(0.3), Size(45, 140), Wait(0.5),
         Snapshot("running"), Key(F1)])
    assert result.exited_cleanly, result.describe()
    assert_laid_out(result.snapshots["running"], 45, 140)
    assert "Mode: RUN" in result.snapshots["running"][43]
