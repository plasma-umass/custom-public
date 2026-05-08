import os
import collections
import re
import signal
import subprocess
import json
from typing import Optional


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

MANIFEST = {
    # docker build -f docker/llvm.Dockerfile --target export -o . .
    "llvm": {
        "malloc-exe": os.path.join(ROOT, "llvm-malloc", "bin", "clang"),
        "exe-args": ["-c", "-O3", os.path.join(ROOT, "sqlite3.c")],
    },
    # See SPEC2000.
    "197.parser": {
        "malloc-exe": os.path.join(ROOT, "197.parser", "src", "parser-malloc"),
        "exe-args": ["2.1.dict", "-batch"],
        "exe-cwd": os.path.join(ROOT, "197.parser", "src"),
        "exe-stdin": os.path.join(ROOT, "197.parser", "src", "ref.in"),
    },
    # See SPEC2000.
    "176.gcc": {
        "malloc-exe": os.path.join(ROOT, "176.gcc", "src", "cc1-malloc"),
        "exe-args": ["200.i"],
        "exe-cwd": os.path.join(ROOT, "176.gcc", "src"),
    },
    # See SPEC2000.
    "175.vpr": {
        "malloc-exe": os.path.join(ROOT, "175.vpr", "src", "vpr-malloc"),
        "exe-args": ["net.in", "arch.in", "place.in", "route.out"],
        "exe-cwd": os.path.join(ROOT, "175.vpr", "src"),
    },
    # See SPEC2000.
    "boxed-sim": {
        "malloc-exe": os.path.join(ROOT, "boxed-sim", "boxed-malloc"),
        "exe-args": ["-n", "128", "-s", "13"],
    },
}

DETECTOR_SO = os.path.join(ROOT, "build", "libdetector_distribution.so")

LITTERER_SO = os.path.join(ROOT, "build", "liblitterer_distribution_standalone.so")

ALLOCATORS = {
    "pt": "",
    "je": os.path.join(ROOT, "jemalloc", "lib", "libjemalloc.so"),
    "mi": os.path.join(ROOT, "mimalloc", "build", "libmimalloc.so"),
}

MULTIPLIER = 10
OCCUPANCIES = [i / 100 for i in range(0, 100, 10)]
ITERATIONS = 11

PERF_EVENTS = [
    "instructions",
    "cycles",
    "L1-dcache-loads",
    "L1-dcache-load-misses",
    "LLC-loads",
    "LLC-load-misses",
    "dTLB-loads",
    "dTLB-load-misses",
]


def get_elapsed(output: str) -> int:
    match = re.search(r"Time elapsed:\s*(\d+)\s*ms", output)
    if not match:
        print(output)
        raise ValueError("Could not find elapsed time in output")
    return int(match.group(1))


def human_readable(n: int) -> str:
    for unit, threshold in [("G", 1_000_000_000), ("M", 1_000_000), ("k", 1_000)]:
        if n >= threshold:
            return f"{n / threshold:.1f}{unit}"
    return str(n)


def get_perf_counter(output: str, event: str) -> int:
    match = re.search(rf"^\s*([\d,]+)\s+{re.escape(event)}", output, re.MULTILINE)
    if not match:
        print(output)
        raise ValueError(f"Could not find perf counter '{event}' in output")
    return int(match.group(1).replace(",", ""))


def run_and_get_stderr(
    exe: str,
    args: list,
    config: dict,
    ld_preload: str,
    stdin: Optional[str],
    cwd: Optional[str],
) -> str:
    env = {
        **os.environ,
        **config,
        "LD_PRELOAD": ld_preload,
    }
    return subprocess.run(
        [exe, *args],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        env=env,
        text=True,
        cwd=cwd,
        stdin=open(stdin) if stdin else None,
    ).stderr


def run_with_perf(
    exe: str,
    args: list,
    config: dict,
    ld_preload: str,
    stdin: Optional[str],
    cwd: Optional[str],
) -> dict:
    env = {
        **os.environ,
        **config,
        "LD_PRELOAD": ld_preload,
        "LITTER_SLEEP": "1",
    }
    with subprocess.Popen(
        [exe, *args],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        env=env,
        text=True,
        cwd=cwd,
        stdin=open(stdin) if stdin else None,
    ) as program:
        bench_stderr = ""
        perf = None
        for line in iter(program.stderr.readline, ""):
            bench_stderr += line
            if line.strip().startswith("Sleeping") and perf is None:
                perf = subprocess.Popen(
                    ["perf", "stat", "-e", ",".join(PERF_EVENTS), "-p", str(program.pid)],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.PIPE,
                )
        program.wait()

    assert perf is not None, "Process never slept; was LITTER_SLEEP set?"
    perf.send_signal(signal.SIGINT)
    perf_stderr = perf.stderr.read()
    perf.wait()

    entry = {"elapsed": get_elapsed(bench_stderr)}
    for event in PERF_EVENTS:
        entry[event] = get_perf_counter(perf_stderr.decode(), event)
    return entry


for benchmark, manifest in MANIFEST.items():
    print(f"Running benchmark {benchmark}...")

    print("Detecting size classes...")
    run_and_get_stderr(
        manifest["malloc-exe"],
        manifest["exe-args"],
        {},
        DETECTOR_SO,
        manifest.get("exe-stdin"),
        manifest.get("exe-cwd"),
    )

    data = collections.defaultdict(lambda: collections.defaultdict(list))

    for alloc_name, alloc in ALLOCATORS.items():
        for occupancy in OCCUPANCIES:
            config = {
                "LITTER_MULTIPLIER": str(MULTIPLIER),
                "LITTER_OCCUPANCY": str(occupancy),
            }
            for i in range(ITERATIONS):
                entry = run_with_perf(
                    manifest["malloc-exe"],
                    manifest["exe-args"],
                    config,
                    (alloc + " " + LITTERER_SO).strip(),
                    manifest.get("exe-stdin"),
                    manifest.get("exe-cwd"),
                )
                data[alloc_name][str(occupancy)].append(entry)
                print(f"{alloc_name} / {occupancy:.2f}: {entry['elapsed']} ms, LLC misses {human_readable(entry['LLC-load-misses'])}")

    print(f"Writing results for {benchmark}...")
    with open(
        os.path.join(ROOT, "graphs", "data", f"cycle.{benchmark}.json"), "w"
    ) as f:
        json.dump(data, f)
