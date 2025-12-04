import csv
from collections import Counter

cause_counts = Counter()
total_rows = 0
death_rows = 0

with open("reaper_only_stats.csv", newline="") as f:
    reader = csv.DictReader(f)
    for row in reader:
        total_rows += 1
        cause = row["causeOfDeath"]
        # If you only want *actual deaths* (exclude alive wins)
        if cause != "alive":
            death_rows += 1
            cause_counts[cause] += 1

print(f"Total rows: {total_rows}")
print(f"Total deaths: {death_rows}\n")

print("Cause-of-death summary:")
print(f"{'Cause of Death':40} {'Count':>8} {'% of deaths':>12}")
for cause, count in cause_counts.most_common():
    pct = 100.0 * count / death_rows if death_rows else 0
    print(f"{cause:40} {count:8d} {pct:12.2f}")

