"""Command-line options: ones that exit before the GUI starts."""

import re
import subprocess

import pytest

pytestmark = pytest.mark.simulator

EXPECTED_HELP = {
    "version": "Print version information and exit",
    "help": "Print this message and exit",
    "fifo_tty_file": "Set fifo tty file",
    "serial_tty_file": "Set serial tty file",
    "symbols": "Load symbols file",
    "hints": "Load disassembler hints file",
}


def run(aprsim, *args):
    return subprocess.run([str(aprsim), *args], capture_output=True, text=True, timeout=10)


@pytest.mark.parametrize("option", ["-v", "--version"])
def test_version(aprsim, option):
    result = run(aprsim, option)
    assert result.returncode == 0
    assert re.fullmatch(r"Apricos Simulator V\S+\n", result.stdout)


def test_help_lists_every_option(aprsim, tools):
    result = run(aprsim, "-h")
    assert result.returncode == 0
    listed = re.findall(r"^\t--(\S+)\s+-(\w)\s", result.stdout, re.MULTILINE)
    expected = [name for name in EXPECTED_HELP if tools.has_tty_emulation or "tty" not in name]
    assert [name for name, _ in listed] == expected


def test_help_describes_each_option(aprsim, tools):
    usage = run(aprsim, "-h").stdout
    descriptions = dict(re.findall(r"^\t--(\S+)\s+-\w\s+(.*)$", usage, re.MULTILINE))
    expected = {name: text for name, text in EXPECTED_HELP.items() if tools.has_tty_emulation or "tty" not in name}
    assert descriptions == expected


def test_unknown_option_fails(aprsim):
    result = run(aprsim, "--no-such-option")
    assert result.returncode == 1


def test_fifo_and_serial_together_fail(tty_aprsim, tmp_path):
    result = run(tty_aprsim, "-f", str(tmp_path / "fifo"), "-u", str(tmp_path / "serial"))
    assert result.returncode == 1
    assert "cannot be both set" in result.stderr
