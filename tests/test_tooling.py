"""The test tools themselves (no simulator needed)."""

import signal

import pytest

from simtools import Key, Signal, Size, Wait, WaitFor, WaitForTty, assemble, parse_actions, pb, read_pb


def test_parse_actions():
    assert parse_actions(r"size:40x120,key:q,key:\x1bOP,wait:0.5,waitfor:Mode,tty:3,winch,signal:INT") == [
        Size(40, 120), Key(b"q"), Key(b"\x1bOP"), Wait(0.5), WaitFor("Mode"), WaitForTty(3),
        Signal(signal.SIGWINCH), Signal(signal.SIGINT)]


def test_parse_actions_rejects_unknown():
    with pytest.raises(ValueError):
        parse_actions("jump:3")


def test_pb_round_trip():
    data = pb(b"\x01\x02\x03", origin=0x1234)
    assert data == b"PB\x03\x00\x34\x12\x01\x02\x03"
    assert read_pb(data) == (0x1234, b"\x01\x02\x03")


def test_assemble(program):
    cpu = program("cpu")
    assert cpu.origin == 0
    assert cpu.code[:2] == b"\xCF" + bytes([cpu.address("reset_taken")])   # JMP RESET_TAKEN
    assert cpu.address("far") == 0x100
    assert program("high").origin == 0x8000
