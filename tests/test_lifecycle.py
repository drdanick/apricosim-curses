"""Starting and quitting the simulator."""

import signal

import pytest

from simtools import F1, Key, Signal, Simulator, Wait, WaitForTty

pytestmark = pytest.mark.simulator


@pytest.mark.parametrize("key", ["q", "Q", F1], ids=["q", "Q", "F1"])
def test_quit_key(aprsim, ready, key):
    result = Simulator(aprsim).run([ready, Key(key)])
    assert result.exited_cleanly, result.describe()


def test_quit_with_a_program_loaded(aprsim, ready, program):
    result = Simulator(aprsim, [program("cpu").binary]).run([ready, Key("q")])
    assert result.exited_cleanly, result.describe()


def test_f1_quits_while_running(aprsim, ready, program):
    result = Simulator(aprsim, [program("steps").binary]).run([ready, Key("mm "), Wait(0.5), Key(F1)])
    assert result.exited_cleanly, result.describe()


def test_q_is_ignored_while_running(aprsim, ready, program):
    result = Simulator(aprsim, [program("steps").binary]).run(
        [ready, Key("mm "), Wait(0.3), Key("q"), Wait(0.5)], exit_wait=0.5)
    assert result.still_running, result.describe()


def test_q_quits_after_pausing(aprsim, ready, program):
    result = Simulator(aprsim, [program("steps").binary]).run(
        [ready, Key("mm "), Wait(0.3), Key(" "), Wait(0.3), Key("q")])
    assert result.exited_cleanly, result.describe()


def test_f1_quits_while_the_program_waits_for_a_key(aprsim, ready, program):
    result = Simulator(aprsim, [program("echo").binary]).run([ready, Key("mm "), Wait(0.5), Key(F1)])
    assert result.exited_cleanly, result.describe()


def test_sigint_exits(aprsim, ready):
    result = Simulator(aprsim).run([ready, Signal(signal.SIGINT)])
    assert result.exited_cleanly, result.describe()


def test_quit_with_tty(tty_aprsim, ready, program):
    result = Simulator(tty_aprsim, [program("hello").binary], tty=True).run(
        [ready, Key("mm "), WaitForTty(5), Key(F1)])
    assert result.exited_cleanly, result.describe()


def test_missing_program_file_is_ignored(aprsim, ready):
    result = Simulator(aprsim, args=["no-such-file.bin"]).run([ready, Key("q")])
    assert result.exited_cleanly, result.describe()
