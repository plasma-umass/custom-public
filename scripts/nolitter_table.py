from collections import defaultdict
import re
import statistics

# -ddd
EVENTS = [
    "context-switches",
    "cpu-migrations",
    "page-faults",
    "cycles",
    "instructions",
    "branches",
    "branch-misses",
    "slots",
    "topdown-retiring",
    "topdown-bad-spec",
    "topdown-fe-bound",
    "topdown-be-bound",
    "L1-dcache-loads",
    "L1-dcache-load-misses",
    "LLC-loads",
    "LLC-load-misses",
    "L1-icache-load-misses",
    "dTLB-loads",
    "dTLB-load-misses",
    "iTLB-load-misses",
]

filenames = [
    f"197.parser.custom.nolitter.log",
    f"boxed-sim.custom.nolitter.log",
    f"mudlle.custom.nolitter.log",
    f"175.vpr.custom.nolitter.log",
]

aggregate = statistics.median

def parse_perf_counter(name, line):
    return int(re.match(f"\s*(\S+)\s*{name}", line).group(1).replace(",", ""))


def parse_run(lines, index):
    run = {}
    assert(re.match("Time elapsed", lines[index]))
    run["elapsed"] = float(re.match("Time elapsed\s*:\s*(.+)", lines[index]).group(1))

    for i in range(len(EVENTS)):
        run[EVENTS[i]] = parse_perf_counter(EVENTS[i], lines[index + 5 + i])

    return index + 32, run


def parse_runs(filename):
    with open(filename, "r") as f:
        lines = f.readlines()

    lines = list(map(lambda x: x.strip(), lines))

    index = 0
    runs = []
    while index < len(lines):
        index, run = parse_run(lines, index)
        runs.append(run)

    return runs

for filename in filenames:
    runs = parse_runs(filename)
    total = defaultdict(lambda: [])
    for run in runs:
        for key in run.keys():
            total[key].append(run[key])

    data = {}
    for key in total.keys():
        data[key] = aggregate(total[key])
    
    print(filename)
    maxlen = max([len(s) for s in EVENTS])
    for key, value in data.items():
        print(f"{key:{maxlen}}: {value}")
    print()
