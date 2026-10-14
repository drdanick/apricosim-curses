#!/usr/bin/env python3
"""Drive a curses program (normally aprsim) in a pseudo-terminal and report how it exited.

Usage:
    ptyharness.py [--log FILE] [--screen] [--tty FILE] ACTIONS -- PROGRAM [ARGS...]

ACTIONS is a comma-separated list, run in order:
    size:ROWSxCOLS   set the terminal size (TIOCSWINSZ, then SIGWINCH)
    key:TEXT         type TEXT (escapes such as \\r and \\x1bOP, which is F1, are decoded)
    wait:SECONDS     keep draining output for SECONDS
    waitfor:TEXT     wait (at most 3 s) until the screen shows TEXT; needs pyte
    tty:COUNT        wait (at most 3 s) until COUNT bytes have arrived on port 7; needs --tty
    winch            send SIGWINCH without changing the size
    signal:NAME      send a signal, e.g. signal:INT
    snapshot:NAME    print the screen at this point; needs pyte

The first action should normally be a size, so curses starts at a known size.
Commas can't be typed with key:, because they separate actions.

Options:
    --log FILE    save the raw terminal output (curses escapes included)
    --screen      print the final screen (needs pyte: pip install pyte)
    --tty FILE    capture port 7: the harness makes a FIFO, adds "-f FIFO" after
                  PROGRAM, and saves what arrives in FILE

Exit status: 0 if the program exited normally, 1 if it was killed by a signal,
2 if it was still running at the end (the harness then kills it).

Lives in apricosim-curses/tests; see tests/README.md.
"""

import sys
import tempfile
from pathlib import Path

from simtools.harness import Simulator, Snapshot, WaitFor, parse_actions


def main(argv: list[str]) -> int:
    log = tty = None
    show_screen = False
    while argv and argv[0].startswith("--"):
        option = argv.pop(0)
        if option == "--log":
            log = Path(argv.pop(0))
        elif option == "--tty":
            tty = Path(argv.pop(0))
        elif option == "--screen":
            show_screen = True
        else:
            sys.exit(f"unknown option {option}\n\n{__doc__}")
    if len(argv) < 3 or argv[1] != "--":
        sys.exit(__doc__)
    actions = parse_actions(argv[0])
    program, *arguments = argv[2:]
    if show_screen or any(isinstance(a, (WaitFor, Snapshot)) for a in actions):
        try:
            import pyte  # noqa: F401
        except ImportError:
            sys.exit("--screen, waitfor: and snapshot: need pyte (pip install pyte)")

    with tempfile.TemporaryDirectory(prefix="ptyharness-") as fifo_dir:
        sim = Simulator(program, args=arguments, tty=tty is not None, cwd=Path.cwd(), fifo_dir=Path(fifo_dir))
        result = sim.run(actions)

    if log:
        log.write_bytes(result.log)
    if tty:
        tty.write_bytes(result.tty)
    for action in actions:
        if isinstance(action, Snapshot) and action.name in result.snapshots:
            print(f"--- {action.name} ---")
            print("\n".join(result.snapshots[action.name]))
    if show_screen:
        print("--- screen ---")
        print(result.screen_text())
    if result.gone_before is not None:
        print(f"program gone before {result.gone_before}")
    print(result.status)
    return 2 if result.still_running else 1 if result.signal else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
