import matplotlib.pyplot as plt
import numpy as np
import re
import seaborn as sns
import statistics

plt.style.use('ggplot')
fig, ax = plt.subplots()
monospace_font = 'Andale Mono' # 'Courier New'
default_font = 'Arial' # Monospace'
sns.set(font=default_font,
    rc={
        'font.size' : 12,
        'axes.titlesize' : 24,
        # 'axes.labelsize' : 14,
        'axes.axisbelow': False,
        'axes.edgecolor': 'lightgrey',
        'axes.facecolor': 'None',
        'axes.grid': False,
        'axes.labelcolor': 'dimgrey',
        'axes.spines.right': False,
        'axes.spines.top': False,
        'figure.facecolor': 'white',
        'lines.solid_capstyle': 'round',
        'patch.edgecolor': 'w',
        'patch.force_edgecolor': True,
        'text.color': 'black',
        'xtick.bottom': False,
        'xtick.color': 'dimgrey',
        'xtick.direction': 'out',
        'xtick.top': False,
        'ytick.color': 'dimgrey',
        'ytick.direction': 'out',
        'ytick.left': False,
        'ytick.right': False})

benchmarks = [
    "197.parser",
    "197.parser.jemalloc",
    "197.parser.mimalloc",
    # "boxed-sim",
    # "mudlle"
]

NORMALIZE_TIME = {
    "197.parser": 37.620861500000004,
    "197.parser.jemalloc": 37.620861500000004,
    "197.parser.mimalloc": 37.620861500000004,

    "boxed-sim": 15.469659,
}

NORMALIZE_INSTRUCTIONS = {
    # "197.parser": 274773916548.5,
    # "boxed-sim": 148013892694.0,
    
    "197.parser": 460358284343.5,
    "197.parser.jemalloc": 460358284343.5,
    "197.parser.mimalloc": 460358284343.5,
    "boxed-sim": 151901586379.5,
}

# Copied from stat.py
EVENTS = [
    "context-switches",
    "cpu-migrations",
    "page-faults",
    "cycles:u",
    "cycles:k",
    "instructions:u",
    "instructions:k",
    "branches",
    "branch-misses",
    "L1-dcache-loads",
    "L1-dcache-load-misses",
    "LLC-loads",
    "LLC-load-misses",
    "dTLB-loads",
    "dTLB-load-misses",
]

ELAPSED_TIME_BY_OCCUPANCY = 0
LLC_MISS_RATE_BY_OCCUPANCY = 1
INSTRUCTIONS_BY_OCCUPANCY = 2

TITLES = [
    "Elapsed Time by Occupancy\n(1x multiplier, normalized)",
    "LLC Miss Rate by Occupancy\n(1x multiplier)",
    "Instructions Executed by Occupancy\n(1x multiplier)"
]


# SET GRAPH TYPE HERE
GRAPH_TYPE = INSTRUCTIONS_BY_OCCUPANCY
aggregate = statistics.median


def parse_perf_counter(name, line):
    return re.match(f"\s*(\S+)\s*{name}", line).group(1).replace(",", "")


def parse_run(lines, index):
    run = {}
    assert(re.match("=+\s*Litterer\s*=+$", lines[index]))
    
    # Assuming shuffle, multiplier, sleep, timestamp, and malloc are constant.
    run["seed"] = re.match("seed\s*:\s*(.+)$", lines[index + 2]).group(1)
    run["occupancy"] = re.match("occupancy\s*:\s*(.+)", lines[index + 3]).group(1)
    run["elapsed"] = re.match("Time elapsed\s*:\s*(.+)", lines[index + 14]).group(1)

    for i in range(len(EVENTS)):
        run[EVENTS[i]] = parse_perf_counter(EVENTS[i], lines[index + 17 + i])   

    return index + 36, run


def parse_runs(filename):
    with open(filename, "r") as f:
        lines = f.readlines()
    
    lines = list(map(lambda x: x.strip(), lines))

    index = 0
    runs = []
    while index < len(lines):
        index, run = parse_run(lines, index)
        runs.append(run)

    # Outliers, temporary solution...
    # if filename == "197.parser.shim.litter.log":
    #     runs = list(filter(lambda x: float(x["elapsed"]) < 240, runs))

    print("Number of parsed runs:", len(runs))
    return sorted(runs, key = lambda x: x["occupancy"])


for benchmark in benchmarks:
    runs = parse_runs(f"{benchmark}.shim.litter.log")

    grouped_by_occupancy = {}
    for run in runs:
        if run["occupancy"] not in grouped_by_occupancy:
            grouped_by_occupancy[run["occupancy"]] = [ run ]
        else:
            grouped_by_occupancy[run["occupancy"]].append(run)

    x = np.array(list(map(float, grouped_by_occupancy.keys())))

    if GRAPH_TYPE == ELAPSED_TIME_BY_OCCUPANCY:
        y = np.array(list(map(lambda x: aggregate(list(map(lambda y: float(y["elapsed"]), x))), grouped_by_occupancy.values())))
        y /= NORMALIZE_TIME[benchmark]
    elif GRAPH_TYPE == LLC_MISS_RATE_BY_OCCUPANCY:
        y = np.array(list(map(lambda x: aggregate(list(map(lambda y: 100.0 * int(y["LLC-load-misses"]) / int(y["LLC-loads"]), x))), grouped_by_occupancy.values())))
    elif GRAPH_TYPE == INSTRUCTIONS_BY_OCCUPANCY:
        y = np.array(list(map(lambda x: aggregate(list(map(lambda y: int(y["instructions:u"]) + int(y["instructions:k"]), x))), grouped_by_occupancy.values())))
        y /= NORMALIZE_INSTRUCTIONS[benchmark]
    else:
        raise "Unknown graph type"
    
    ax.plot(x, y, label=benchmark)

    # TODO: y-ticks are not bold.
    ticks = ax.get_xticklabels()
    for tick in ticks:
        tick.set_fontweight("bold")
        tick.set_fontsize(10)
        tick.set_horizontalalignment("center")
    plt.setp(ticks, ha='right', rotation=90)
    

# if GRAPH_TYPE == ELAPSED_TIME_BY_OCCUPANCY:
#     plt.ylim(bottom=0)
plt.ylim(bottom=0)

ax.set_title(f"{TITLES[GRAPH_TYPE]}", pad=20, fontweight='bold', font=default_font, fontsize=18)
ax.legend()
fig.tight_layout()
plt.savefig("graph.png")
