"""Running programs: stepping, RUN mode, breakpoints, and the ports."""

import pytest

from simtools import ENTER, F1, Key, Simulator, Snapshot, Wait, WaitFor, WaitForTty, pb

pytestmark = pytest.mark.simulator

# What cpu.asm writes to port 7, in order (see its comments)
CPU_RESULTS = [
    ("branch not taken at reset", 0x01),
    ("ADD carry sets O and Z", 0x0A),
    ("AND zero-extends", 0x05),
    ("OR only reaches bits 3..0", 0xF3),
    ("NOT", 0xF0),
    ("SHFl 4", 0x30),
    ("SHFr 4 is logical", 0x08),
    ("SPOP ADD", 0x0C),
    ("stack pointer after push and pop", 0xFF),
    ("ST then LD", 0x5A),
    ("LDI", 0x99),
    ("bare JMP to $mr", 0x42),
]


def run_mode(ready):
    return [ready, Key("mm ")]


def test_hello(tty_aprsim, ready, program):
    result = Simulator(tty_aprsim, [program("hello").binary], tty=True).run(
        run_mode(ready) + [WaitForTty(5), Key(F1)])
    assert result.exited_cleanly, result.describe()
    assert result.tty == b"\x7f\x00HI\n"


@pytest.fixture(scope="module")
def cpu_output(tty_aprsim, ready, program):
    result = Simulator(tty_aprsim, [program("cpu").binary], tty=True).run(
        run_mode(ready) + [WaitForTty(len(CPU_RESULTS)), Key(F1)])
    assert result.exited_cleanly, result.describe()
    return result.tty


@pytest.mark.parametrize("index, expected", [(i, value) for i, (_, value) in enumerate(CPU_RESULTS)],
                         ids=[name for name, _ in CPU_RESULTS])
def test_cpu(cpu_output, index, expected):
    assert len(cpu_output) == len(CPU_RESULTS), cpu_output.hex(" ")
    assert cpu_output[index] == expected


@pytest.mark.screen
def test_instruction_mode_steps_one_instruction(aprsim, ready, program):
    result = Simulator(aprsim, [program("steps").binary]).run(
        [ready, Key("m"), Key(" "), Key(" "), Key(" "), WaitFor("Instruction Count: 3"), Snapshot("s"), Key("q")])
    screen = result.screen_text()
    assert "Instruction Count: 3  Cycle Count: 12" in screen     # NOP takes 4 cycles
    assert "[Program Counter : 3 " in screen


@pytest.mark.screen
def test_state_mode_steps_one_cycle(aprsim, ready, program):
    result = Simulator(aprsim, [program("steps").binary]).run(
        [ready] + [Key(" ")] * 5 + [WaitFor("Cycle Count: 5"), Key("q")])
    screen = result.screen_text()
    assert "Instruction Count: 1  Cycle Count: 5  Mode: STATE" in screen
    assert "[Program Counter : 1 " in screen


def breakpoint_run(aprsim, ready, steps):
    """Set a breakpoint at MARK in steps.asm, run to it, then continue."""
    mark = steps.address("mark")
    return Simulator(aprsim, [steps.binary], tty=True).run(
        [ready] + [Key("j")] * mark + [Key("b"), Key("mm "), WaitFor(f"Instruction Count: {mark} "),
                                       Wait(0.3), Snapshot("stopped"), Key(" "), WaitForTty(1), Key(F1)])


@pytest.mark.screen
def test_breakpoint_stops_run_mode(tty_aprsim, ready, program):
    steps = program("steps")
    result = breakpoint_run(tty_aprsim, ready, steps)
    assert result.exited_cleanly, result.describe()
    # Stopped before MARK: still 8 instructions (the NOPs) after waiting
    assert f"Instruction Count: {steps.address('mark')} " in "\n".join(result.snapshots["stopped"])
    assert result.tty == b"\x42"        # written only after continuing past the breakpoint


@pytest.mark.screen
@pytest.mark.xfail(strict=True, reason="sim.c mainloop: when a breakpoint is hit in the first RUN slice, "
                   "running was never set, so refreshAll() is skipped and the registers and memory "
                   "panels still show the state from before the run")
def test_breakpoint_stop_shows_the_program_counter(tty_aprsim, ready, program):
    steps = program("steps")
    result = breakpoint_run(tty_aprsim, ready, steps)
    assert f"[Program Counter : {steps.address('mark')} " in "\n".join(result.snapshots["stopped"])


def test_keyboard_input(tty_aprsim, ready, program):
    actions = run_mode(ready) + [Wait(0.3)]
    for count, key in enumerate(["a", "Z", ENTER], start=1):
        actions += [Key(key), WaitForTty(count)]
    result = Simulator(tty_aprsim, [program("echo").binary], tty=True).run(actions + [Key(F1)])
    assert result.exited_cleanly, result.describe()
    assert result.tty == b"aZ\n"        # Enter (CR) reaches the program as LF


def blank_disk() -> bytearray:
    return bytearray(32 * 64 * 256)


def sector(image, track, number) -> bytes:
    start = track * 16384 + number * 256
    return bytes(image[start:start + 256])


def test_disk_ports(tty_aprsim, ready, program, tmp_path):
    disk = blank_disk()
    disk[1 * 16384 + 2 * 256:1 * 16384 + 3 * 256] = bytes([0xC3]) + bytes(range(1, 256))
    result = Simulator(tty_aprsim, [program("disk").binary], tty=True, files={"0.dsk": bytes(disk)}).run(
        run_mode(ready) + [WaitForTty(4), Key(F1)], workdir=tmp_path)
    assert result.exited_cleanly, result.describe()
    # disk 0 selected; still selected after asking for disk 5, which has no file;
    # the first byte of track 1 sector 2; done
    assert result.tty == bytes([0x00, 0x00, 0xC3, 0x77])
    written = sector((tmp_path / "0.dsk").read_bytes(), 2, 3)
    expected = bytearray(sector(disk, 1, 2))
    expected[0x10] = 0xA5
    assert written == bytes(expected)


def test_disk_ports_without_a_disk(tty_aprsim, ready, program):
    result = Simulator(tty_aprsim, [program("disk").binary], tty=True).run(
        run_mode(ready) + [WaitForTty(4), Key(F1)])
    assert result.exited_cleanly, result.describe()
    # no disk open (0xFF) both times; reading does nothing, so the buffer page stays zero
    assert result.tty == bytes([0xFF, 0xFF, 0x00, 0x77])


def test_later_programs_overwrite_earlier_ones(tty_aprsim, ready, program):
    result = Simulator(tty_aprsim, [program("hello").binary, program("cpu").binary], tty=True).run(
        run_mode(ready) + [WaitForTty(len(CPU_RESULTS)), Key(F1)])
    assert result.tty == bytes(value for _, value in CPU_RESULTS)


def test_file_without_pb_magic_is_ignored(tty_aprsim, ready, program):
    hello = program("hello")
    result = Simulator(tty_aprsim, [pb(hello.code, magic=b"XX")], tty=True).run(
        run_mode(ready) + [Wait(0.5), Key(F1)])
    assert result.exited_cleanly, result.describe()
    assert result.tty == b""            # memory is all NOPs


@pytest.mark.xfail(strict=True, reason="sim.c loadProgram reads the origin into a signed short, so a "
                   "program at 0x8000 or above is written before the start of memory[] instead")
def test_program_at_0x8000(tty_aprsim, ready, program):
    result = Simulator(tty_aprsim, [program("jumphigh").binary, program("high").binary], tty=True).run(
        run_mode(ready) + [WaitForTty(1), Key(F1)])
    assert result.exited_cleanly, result.describe()
    assert result.tty == b"\x48"
