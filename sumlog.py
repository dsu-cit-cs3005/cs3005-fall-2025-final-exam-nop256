import csv
from collections import Counter

cause_counts = Counter()

total_rows = 0
death_rows = 0

total_wins = 0
total_kills = 0
total_shots_fired = 0
total_shots_hit = 0
total_damage_dealt = 0
total_damage_taken = 0
total_rounds_survived = 0
total_times_stuck = 0


def to_int(s):
    try:
        return int(s)
    except (TypeError, ValueError):
        return 0


with open("reaper_only_stats.csv", newline="") as f:
    reader = csv.DictReader(f)
    for row in reader:
        total_rows += 1

        # basic fields
        cause = row["causeOfDeath"]
        wins = to_int(row["won"])
        kills = to_int(row["kills"])
        shots_fired = to_int(row["shotsFired"])
        shots_hit = to_int(row["shotsHit"])
        dmg_dealt = to_int(row["damageDealt"])
        dmg_taken = to_int(row["damageTaken"])
        rounds_survived = to_int(row["roundsSurvived"])
        times_stuck = to_int(row["timesStuck"])

        # accumulate totals
        total_wins += wins
        total_kills += kills
        total_shots_fired += shots_fired
        total_shots_hit += shots_hit
        total_damage_dealt += dmg_dealt
        total_damage_taken += dmg_taken
        total_rounds_survived += rounds_survived
        total_times_stuck += times_stuck

        # death info
        if cause != "alive":
            death_rows += 1
            cause_counts[cause] += 1


# ---------- overall summary ----------
print(f"Total runs (rows): {total_rows}")
print(f"Total deaths     : {death_rows}")
print(f"Total wins       : {total_wins}")
print(f"Total kills      : {total_kills}")
print()

# K/D ratio (kills per death)
if death_rows > 0:
    kd_ratio = total_kills / death_rows
    kd_str = f"{kd_ratio:.2f}"
else:
    kd_str = "N/A (no deaths)"
print(f"K/D ratio        : {kd_str}")

# Win / loss ratio & win rate
losses = total_rows - total_wins
if losses > 0:
    wl_ratio = total_wins / losses
    wl_str = f"{wl_ratio:.2f}"
else:
    wl_str = "∞ (no losses)" if total_wins > 0 else "N/A"
win_rate = (total_wins / total_rows) * 100 if total_rows else 0.0
print(f"Win/Loss ratio   : {wl_str}")
print(f"Win rate         : {win_rate:5.2f}%")
print()

# Accuracy, damage, survival, stuck
accuracy = (total_shots_hit / total_shots_fired) * 100 if total_shots_fired else 0.0
avg_kills = total_kills / total_rows if total_rows else 0.0
avg_dmg_dealt = total_damage_dealt / total_rows if total_rows else 0.0
avg_dmg_taken = total_damage_taken / total_rows if total_rows else 0.0
avg_rounds = total_rounds_survived / total_rows if total_rows else 0.0
avg_stuck = total_times_stuck / total_rows if total_rows else 0.0

print("Aggregate performance:")
print(f"  Shot accuracy        : {accuracy:5.2f}% "
      f"({total_shots_hit} / {total_shots_fired})")
print(f"  Avg kills per run    : {avg_kills:5.2f}")
print(f"  Avg dmg dealt / run  : {avg_dmg_dealt:7.2f}")
print(f"  Avg dmg taken / run  : {avg_dmg_taken:7.2f}")
print(f"  Avg rounds survived  : {avg_rounds:7.2f}")
print(f"  Avg times stuck / run: {avg_stuck:7.2f}")
print()

# ---------- cause-of-death summary ----------
print("Cause-of-death summary:")
print(f"{'Cause of Death':40} {'Count':>8} {'% of deaths':>12}")
for cause, count in cause_counts.most_common():
    pct = 100.0 * count / death_rows if death_rows else 0
    print(f"{cause:40} {count:8d} {pct:12.2f}")

