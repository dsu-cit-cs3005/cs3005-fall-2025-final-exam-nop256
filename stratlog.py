#!/usr/bin/env python3
import math

MODE_NAMES = ["ESC", "EXP", "HUNT", "REPOS", "DRIFT"]
ESCAPE_CAUSE_NAMES = ["close", "flame", "rail", "damage"]
KNOWN_BUCKET_NAMES = ["low", "mid", "high"]

total_games = 0

total_moves = [0] * 5      # by mode
total_stays = [0] * 5      # by mode
total_dir   = [0] * 9      # dir 0..8
total_escape_causes = [0] * 4
total_known = [0] * 3
total_stuck = 0

def find_value(parts, prefix):
    """Find the token that starts with prefix and return the text after '='."""
    for p in parts:
        if p.startswith(prefix) and "=" in p:
            return p.split("=", 1)[1]
    return None

with open("reaper_stats.csv") as f:
    for line in f:
        line = line.strip()
        if not line.startswith("[SWEEPER-BRAIN-SUMMARY]"):
            continue

        parts = line.split()

        # try to pull each field by its label instead of fixed index
        moves_str = find_value(parts, "movesByMode(")
        stays_str = find_value(parts, "staysByMode(")
        dir_str   = find_value(parts, "dirCounts(")
        esc_str   = find_value(parts, "escapeCauses(")
        known_str = find_value(parts, "knownBuckets(")
        stuck_str = find_value(parts, "timesStuck")

        # if any are missing, skip this line (old format or malformed)
        if None in (moves_str, stays_str, dir_str, esc_str, known_str, stuck_str):
            continue

        total_games += 1

        moves = list(map(int, moves_str.split(",")))
        stays = list(map(int, stays_str.split(",")))
        dirs  = list(map(int, dir_str.split(",")))
        esc   = list(map(int, esc_str.split(",")))
        known = list(map(int, known_str.split(",")))
        stuck = int(stuck_str)

        for i in range(5):
            total_moves[i] += moves[i]
            total_stays[i] += stays[i]
        for i in range(9):
            total_dir[i] += dirs[i]
        for i in range(4):
            total_escape_causes[i] += esc[i]
        for i in range(3):
            total_known[i] += known[i]

        total_stuck += stuck

print(f"Parsed {total_games} brain-summary lines\n")

print("Moves + stays by mode:")
for i, name in enumerate(MODE_NAMES):
    print(f"  {name:6s}  moves={total_moves[i]:6d}  stays={total_stays[i]:6d}")

total_moves_sum = sum(total_moves)
if total_moves_sum > 0:
    print("\nMove mode percentages (by moves only):")
    for i, name in enumerate(MODE_NAMES):
        pct = 100.0 * total_moves[i] / total_moves_sum
        print(f"  {name:6s}  {pct:6.2f}%")

print("\nDirection choice counts (dir 0..8):")
for d in range(9):
    print(f"  dir {d}: {total_dir[d]}")

total_dir_sum = sum(total_dir)
if total_dir_sum > 0:
    print("\nDirection percentages (by chosen move dir):")
    for d in range(9):
        pct = 100.0 * total_dir[d] / total_dir_sum
        print(f"  dir {d}: {pct:6.2f}%")

print("\nEscape causes (times ESCAPE was triggered by each condition):")
for i, name in enumerate(ESCAPE_CAUSE_NAMES):
    print(f"  {name:6s}: {total_escape_causes[i]}")

print("\nKnowledge bucket counts at decision time:")
for i, name in enumerate(KNOWN_BUCKET_NAMES):
    print(f"  {name:6s}: {total_known[i]}")

print(f"\nTotal timesStuck across all games: {total_stuck}")
