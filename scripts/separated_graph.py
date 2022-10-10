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


def parse_run(lines, index):
    run = {}
    assert(re.match("=+\s*Litterer\s*=+$", lines[index]))

    # run["seed"] = re.match("seed\s*:\s*(.+)$", lines[index + 2]).group(1)
    run["occupancy"] = re.match("occupancy\s*:\s*(.+)", lines[index + 3]).group(1)
    # run["elapsed"] = re.match("Time elapsed\s*:\s*(.+)", lines[index + 14]).group(1)

    index += 19
    run["events"] = {}
    while index < len(lines) and lines[index][0].isalpha():
        event = lines[index].split()[0]
        run["events"][event] = {}
        index += 1
        while index < len(lines) and lines[index][0] == '\t':
            obj = lines[index].split()[0]
            weight = int(lines[index].split()[1])
            run["events"][event][obj] = weight
            index += 1

    return index, run


def parse_runs(filename):
    with open(filename, "r") as f:
        lines = f.readlines()

    index = 0
    runs = []
    while index < len(lines):
        index, run = parse_run(lines, index)
        runs.append(run)

    return sorted(runs, key = lambda x: x["occupancy"])


malloc_object = "/home/nvankempen/Programming/nicovank/allocators/jemalloc/lib/libjemalloc.so.2"
filename = "197.parser.jemalloc.shim.litter.record.log"
event = "instructions"
aggregate = statistics.median
TITLE = "Cycles counter by occupancy\n197.parser, jemalloc"


runs = parse_runs(filename)

grouped_by_occupancy = {}
for run in runs:
    if run["occupancy"] not in grouped_by_occupancy:
        grouped_by_occupancy[run["occupancy"]] = [ run ]
    else:
        grouped_by_occupancy[run["occupancy"]].append(run)

total = {}
in_malloc = {}
for occupancy, runs in grouped_by_occupancy.items():
    for run in runs:
        t1, t2 = 0, 0
        for obj, weight in run["events"][event].items():
            t1 += weight
            if obj == malloc_object:
                t2 += weight
        if occupancy not in total:
            total[occupancy] = [t1]
            in_malloc[occupancy] = [t2]
        else:
            total[occupancy].append(t1)
            in_malloc[occupancy].append(t2)

for occupancy in grouped_by_occupancy.keys():
    in_malloc[occupancy] = aggregate(in_malloc[occupancy])
    total[occupancy] = aggregate(total[occupancy])

x = np.array(list(map(float, total.keys())))
ax.plot(x, list(total.values()), label="total")
ax.plot(x, list(in_malloc.values()), label="malloc/free")

# TODO: y-ticks are not bold.
ticks = ax.get_xticklabels()
for tick in ticks:
    tick.set_fontweight("bold")
    tick.set_fontsize(10)
    tick.set_horizontalalignment("center")
plt.setp(ticks, ha='right', rotation=90)

ax.set_title(TITLE, pad=20, fontweight='bold', font=default_font, fontsize=18)
ax.legend()
fig.tight_layout()
plt.savefig("graph.png")
