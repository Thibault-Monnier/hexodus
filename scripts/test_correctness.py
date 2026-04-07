#!/usr/bin/env python3
"""
Correctness tests for the Hexodus engine.

Communicates with the hexodus executable via stdin/stdout to verify:
  1. Basic move placement and undo consistency.
  2. Engine produces valid moves (coordinates appear on the board).
  3. Engine analysis is idempotent (analyse does not change board state).
  4. Reset restores the initial position.
  5. Undo reverts the last move properly.
  6. The engine does not crash under a longer game sequence.
"""

import os
import re
import subprocess
import sys

EXECUTABLE = os.path.join(os.path.dirname(__file__), "..", "cmake-build-release", "hexodus", )


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def run_session(commands: list[str], timeout: int = 30) -> str:
    """Send a sequence of commands to hexodus and return its combined stdout."""
    input_text = "\n".join(commands) + "\n"
    result = subprocess.run([EXECUTABLE], input=input_text, capture_output=True, text=True, timeout=timeout, )
    return result.stdout, result.stderr


def parse_best_move(output: str) -> tuple[tuple[int, int], tuple[int, int]]:
    """Extract the first 'Best move: (x1, y1) and (x2, y2)' from output."""
    m = re.search(r"Best move: \((-?\d+), (-?\d+)\) and \((-?\d+), (-?\d+)\)", output)
    assert m, f"Could not parse best move from output:\n{output}"
    return (int(m.group(1)), int(m.group(2))), (int(m.group(3)), int(m.group(4)))


def count_pieces(output: str) -> tuple[int, int]:
    """
    Count white ('X') and black ('O') pieces from the board print output.
    Returns (white_count, black_count).
    """
    # Only count inside the board section (after "Current game state:")
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    in_board = False
    white, black = 0, 0
    for line in output.splitlines():
        if "Current game state:" in line:
            in_board = True
            continue
        if "Quitting" in line or "Best move:" in line or "Undoing" in line or "Resetting" in line:
            in_board = False
            continue
        if in_board:
            clean_line = ansi_escape.sub('', line)
            clean_line.split()
            # If the line contains axis numbers, ignore them and count X, O
            white += clean_line.count("X")
            black += clean_line.count("O")
    return white, black


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

passed = 0
failed = 0


def check(name: str, condition: bool, detail: str = ""):
    global passed, failed
    if condition:
        passed += 1
        print(f"  ✓ {name}")
    else:
        failed += 1
        msg = f"  ✗ {name}"
        if detail:
            msg += f"  — {detail}"
        print(msg)


def test_initial_board():
    """After reset the board should contain exactly one white piece at (0,0)."""
    print("\n[test_initial_board]")
    stdout, _ = run_session(["print", "quit"])
    w, b = count_pieces(stdout)
    check("1 white piece", w == 1, f"white={w}")
    check("0 black pieces", b == 0, f"black={b}")


def test_make_move_and_print():
    """Place a move and verify both new pieces appear on the board."""
    print("\n[test_make_move_and_print]")
    stdout, _ = run_session(["move 1 0 0 1", "print", "quit"])
    w, b = count_pieces(stdout)
    check("1 white piece", w == 1, f"white={w}")
    check("2 black pieces", b == 2, f"black={b}")
    check("3 total pieces", w + b == 3, f"total={w + b}")


def test_undo():
    """Undo should revert the board to the previous state."""
    print("\n[test_undo]")
    stdout, _ = run_session(["move 1 0 0 1", "undo", "print", "quit"])
    w, b = count_pieces(stdout)
    check("back to 1 piece after undo", w + b == 1, f"total={w + b}")
    check("only white remains", w == 1 and b == 0, f"w={w}, b={b}")


def test_reset():
    """Reset should bring the game back to the initial state."""
    print("\n[test_reset]")
    stdout, _ = run_session(["move 1 0 0 1", "move -1 0 0 -1", "reset", "print", "quit"])
    w, b = count_pieces(stdout)
    check("1 piece after reset", w + b == 1, f"total={w + b}")
    check("only white", w == 1 and b == 0, f"w={w}, b={b}")


def test_analyse_is_idempotent():
    """Running 'analyse' twice should return the same move and not change the board."""
    print("\n[test_analyse_is_idempotent]")
    stdout, _ = run_session(["analyse", "analyse", "print", "quit"])
    moves = re.findall(r"Best move: \((-?\d+), (-?\d+)\) and \((-?\d+), (-?\d+)\)", stdout)
    check("got 2 analyse results", len(moves) == 2, f"found {len(moves)}")
    if len(moves) == 2:
        check("both analyse results identical", moves[0] == moves[1], f"{moves[0]} vs {moves[1]}")

    # Board should still have only the initial piece
    w, b = count_pieces(stdout)
    check("board unchanged after analyse", w == 1 and b == 0, f"w={w}, b={b}")


def test_moverequest_places_pieces():
    """'moverequest' should place pieces and they should appear on the board."""
    print("\n[test_moverequest_places_pieces]")
    stdout, _ = run_session(["moverequest", "print", "quit"])
    w, b = count_pieces(stdout)
    # Should have 3 pieces: 1 white (initial) + 2 black from moverequest
    check("1 white piece", w == 1, f"white={w}")
    check("2 black pieces", b == 2, f"black={b}")
    check("3 total after moverequest", w + b == 3, f"total={w + b}")


def test_engine_move_is_valid():
    """The engine's suggested move should have two distinct coordinates."""
    print("\n[test_engine_move_is_valid]")
    stdout, _ = run_session(["analyse", "quit"])
    move = parse_best_move(stdout)
    check("coord1 != coord2", move[0] != move[1], f"move={move}")
    # After analyse the board should be unchanged
    stdout2, _ = run_session(["analyse", "print", "quit"])
    w, b = count_pieces(stdout2)
    check("board unchanged (1 white)", w == 1 and b == 0, f"w={w}, b={b}")


def test_multiple_moves_no_crash():
    """Play several engine-vs-engine moves without crashing."""
    print("\n[test_multiple_moves_no_crash]")
    commands = ["moverequest"] * 6 + ["print", "quit"]
    stdout, stderr = run_session(commands, timeout=120)
    move_lines = re.findall(r"Best move:", stdout)
    check("6 moves played", len(move_lines) == 6, f"found {len(move_lines)}")
    # 1 initial + 6*2 = 13 pieces
    w, b = count_pieces(stdout)
    check("13 pieces on board", w + b == 13, f"got {w + b} pieces (w={w}, b={b})")
    check("no crash", "Quitting" in stdout)


def test_undo_after_moverequest():
    """Undo after a moverequest should remove the engine's pieces."""
    print("\n[test_undo_after_moverequest]")
    stdout, _ = run_session(["moverequest", "undo", "print", "quit"])
    w, b = count_pieces(stdout)
    check("1 piece after undo of moverequest", w + b == 1, f"total={w + b}")


def test_two_moves_then_undo_twice():
    """Two moves followed by two undos should restore the initial board."""
    print("\n[test_two_moves_then_undo_twice]")
    stdout, _ = run_session(["move 1 0 0 1", "move -1 0 0 -1", "undo", "undo", "print", "quit", ])
    w, b = count_pieces(stdout)
    check("1 piece after 2 undos", w + b == 1, f"total={w + b}")
    check("only white remains", w == 1 and b == 0, f"w={w}, b={b}")


def test_win_in_one():
    """Checks whether the bot sees a win in 1 move when 4 pieces are aligned."""
    print("\n[test_win_in_one]")
    # Initial: White at (0,0)
    # Black: 0 1, 0 2
    # White: 1 0, 2 0 -> White has (0,0), (1,0), (2,0)
    # Black: 0 3, 0 4
    # White: 3 0, 0 -1 -> White has (0,0), (1,0), (2,0), (3,0) on X-axis, plus (0,-1)
    # Black: 0 5, 0 6
    stdout, _ = run_session(["move 0 1 0 2", "move 1 0 2 0", "move 0 3 0 4", "move 3 0 0 -1", "analyse", "quit"])
    try:
        move = parse_best_move(stdout)

        valid_winning_pairs = [{(0, 5), (0, 6)}, {(0, 6), (0, 5)}]

        check("bot picks a winning move", set(move) in valid_winning_pairs, f"move={move}")
    except AssertionError as e:
        check("bot parses best move successfully", False, f"error parsing: {e}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    if not os.path.isfile(EXECUTABLE):
        print(f"ERROR: executable not found at {EXECUTABLE}")
        print("Build the project first:  cmake --build cmake-build-release --target hexodus")
        sys.exit(1)

    print(f"Running correctness tests against: {EXECUTABLE}")

    test_initial_board()
    test_make_move_and_print()
    test_undo()
    test_reset()
    test_analyse_is_idempotent()
    test_moverequest_places_pieces()
    test_engine_move_is_valid()
    test_multiple_moves_no_crash()
    test_undo_after_moverequest()
    test_two_moves_then_undo_twice()
    test_win_in_one()

    print(f"\n{'=' * 50}")
    print(f"Results: {passed} passed, {failed} failed, {passed + failed} total")
    if failed:
        sys.exit(1)
    else:
        print("All tests passed! ✓")
        sys.exit(0)
