import pandas as pd
import matplotlib.pyplot as plt

INPUT_FILE = "mlfqclean.txt"

# Part before @ in the IIIT email.
USERNAME = "saarthak.singh"

# ------------------------------------------------------------
# Read cleaned trace.
# Format:
# TICK PID STATE QUEUE SLICE TOTAL INQ ARR FIRST DONE WAIT RESP TURN
# ------------------------------------------------------------
df = pd.read_csv(INPUT_FILE, sep=r"\s+", skiprows=3)

# Keep only the processes in this MLFQ test.
df = df[df["PID"].between(3, 7)].copy()

# Remove any accidental duplicate (TICK, PID) observations.
df = df.drop_duplicates(subset=["TICK", "PID"], keep="first")

# ------------------------------------------------------------
# Time axis
# ------------------------------------------------------------
# All five test processes arrive at tick 38 in this run.
# Using the earliest ARR gives the benchmark/scheduler start
# reference while preserving every original TICK value.
start_tick = int(df["ARR"].min())
df["ELAPSED"] = df["TICK"] - start_tick

# ------------------------------------------------------------
# Detect the actual priority-boost events from the trace.
#
# A boost is detected when ALL active processes simultaneously
# drop from low queues (Q2/Q3) to Q0/Q1 between two snapshots.
# This identifies:
#     absolute tick 97  -> elapsed 59
#     absolute tick 144 -> elapsed 106
#
# No queue values are changed.
# ------------------------------------------------------------
snapshots = df.groupby("TICK")["QUEUE"].agg(list)
ticks = sorted(snapshots.index)

boost_ticks = []

for prev_tick, curr_tick in zip(ticks[:-1], ticks[1:]):
    prev_q = snapshots.loc[prev_tick]
    curr_q = snapshots.loc[curr_tick]

    # Require all five test processes in both snapshots.
    prev_rows = df[df["TICK"] == prev_tick]
    curr_rows = df[df["TICK"] == curr_tick]

    if len(prev_rows) != 5 or len(curr_rows) != 5:
        continue

    # Global boost signature:
    # previous queues are low (2/3), current queues are high (0/1),
    # with a substantial reduction for every process.
    prev_map = prev_rows.set_index("PID")["QUEUE"]
    curr_map = curr_rows.set_index("PID")["QUEUE"]

    if (
        (prev_map >= 2).all()
        and (curr_map <= 1).all()
        and (curr_map < prev_map).all()
    ):
        boost_ticks.append(curr_tick)

# ------------------------------------------------------------
# Plot
# ------------------------------------------------------------
fig, ax = plt.subplots(figsize=(13, 7))

# Matplotlib's default color cycle gives one distinct color per PID.
for pid in sorted(df["PID"].unique()):
    p = df[df["PID"] == pid].sort_values("TICK")

    ax.scatter(
        p["ELAPSED"],
        p["QUEUE"],
        s=38,
        label=f"PID {pid}",
        zorder=3
    )

    # Thin connecting path shows queue movement over time.
    ax.plot(
        p["ELAPSED"],
        p["QUEUE"],
        linewidth=0.9,
        alpha=0.45
    )

# Mark ONLY the boost events detected from the actual trace.
for boost_tick in boost_ticks:
    elapsed = boost_tick - start_tick

    ax.axvline(
        elapsed,
        linestyle="--",
        linewidth=1.3,
        alpha=0.8,
        label="Priority Boost" if boost_tick == boost_ticks[0] else None
    )

    ax.annotate(
        f"Boost\n(t={elapsed})",
        xy=(elapsed, 3.0),
        xytext=(6, -4),
        textcoords="offset points",
        rotation=90,
        va="top",
        ha="left",
        fontsize=9
    )

ax.set_yticks([0, 1, 2, 3])
ax.set_ylim(-0.2, 3.35)

ax.set_xlabel("Time elapsed since scheduler start (ticks)")
ax.set_ylabel("MLFQ Queue ID")
ax.set_title("MLFQ Queue Movement and Periodic Priority Boost")

ax.grid(axis="y", alpha=0.25)
ax.legend(title="Process")

# Required watermark.
ax.text(
    0.99,
    0.02,
    USERNAME,
    transform=ax.transAxes,
    ha="right",
    va="bottom",
    fontsize=11,
    alpha=0.55
)

fig.tight_layout()

plt.savefig("mlfq_timeline.png", dpi=300, bbox_inches="tight")
plt.show()

print("Scheduler/benchmark start tick:", start_tick)
print("Detected boost ticks:", boost_ticks)
print("Elapsed boost ticks:", [t - start_tick for t in boost_ticks])