import os
import collections
import re
import subprocess
import json
from typing import Optional

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

MANIFEST = {
    # docker build -f docker/llvm.Dockerfile --target export -o . .
    "llvm": {
        "malloc-exe": os.path.join(ROOT, "llvm-malloc", "bin", "clang"),
        "arena-exe": os.path.join(ROOT, "llvm-arena", "bin", "clang"),
        "exe-args": ["-c", "-O3", os.path.join(ROOT, "sqlite3.c")],
    },
    # See SPEC2000.
    "197.parser": {
        "malloc-exe": os.path.join(ROOT, "197.parser", "src", "parser-malloc"),
        "arena-exe": os.path.join(ROOT, "197.parser", "src", "parser-arena"),
        "exe-args": ["2.1.dict", "-batch"],
        "exe-cwd": os.path.join(ROOT, "197.parser", "src"),
        "exe-stdin": os.path.join(ROOT, "197.parser", "src", "ref.in"),
    },
    # See SPEC2000.
    "176.gcc": {
        "malloc-exe": os.path.join(ROOT, "176.gcc", "src", "cc1-malloc"),
        "arena-exe": os.path.join(ROOT, "176.gcc", "src", "cc1-arena"),
        "exe-args": ["200.i"],
        "exe-cwd": os.path.join(ROOT, "176.gcc", "src"),
    },
    # See SPEC2000.
    "175.vpr": {
        "malloc-exe": os.path.join(ROOT, "175.vpr", "src", "vpr-malloc"),
        "arena-exe": os.path.join(ROOT, "175.vpr", "src", "vpr-arena"),
        "exe-args": ["net.in", "arch.in", "place.in", "route.out"],
        "exe-cwd": os.path.join(ROOT, "175.vpr", "src"),
    },
    # See SPEC2000.
    "boxed-sim": {
        "malloc-exe": os.path.join(ROOT, "boxed-sim", "boxed-malloc"),
        "arena-exe": os.path.join(ROOT, "boxed-sim", "boxed-arena"),
        "exe-args": ["-n", "128", "-s", "13"],
    },
    # See SPEC2000.
    "mudlle": {
        "malloc-exe": os.path.join(ROOT, "mudlle", "mudlle-malloc"),
        "arena-exe": os.path.join(ROOT, "mudlle", "mudlle-arena"),
        "exe-cwd": os.path.join(ROOT, "mudlle"),
        "exe-stdin": os.path.join(ROOT, "mudlle", "time.mud"),
    },
}

DETECTOR_SO = os.path.join(ROOT, "build", "libdetector_distribution.so")

LITTERER_SO = os.path.join(ROOT, "build", "liblitterer_distribution_standalone.so")

ALLOCATORS = {
    "pt": "",
    "je": os.path.join(ROOT, "jemalloc", "lib", "libjemalloc.so"),
    "mi": os.path.join(ROOT, "mimalloc", "build", "libmimalloc.so"),
}

CONFIGURATIONS_FOR_ARENA = {
    "0litter": {"LITTER_MULTIPLIER": "0", "LITTER_OCCUPANCY": "0"},
    "3litter": {"LITTER_MULTIPLIER": "3", "LITTER_OCCUPANCY": "0.666"},
}

CONFIGURATIONS_FOR_MALLOC = {
    "0litter": {"LITTER_MULTIPLIER": "0", "LITTER_OCCUPANCY": "0"},
    "1litter": {"LITTER_MULTIPLIER": "1", "LITTER_OCCUPANCY": "0.333"},
    "3litter": {"LITTER_MULTIPLIER": "3", "LITTER_OCCUPANCY": "0.666"},
    "10litter": {"LITTER_MULTIPLIER": "10", "LITTER_OCCUPANCY": "0.8"},
}

ITERATIONS = 13


def get_elapsed(output: str) -> int:
    match = re.search(r"Time elapsed:\s*(\d+)\s*ms", output)
    if not match:
        print(output)
        raise ValueError("Could not find elapsed time in output")
    return int(match.group(1))


def run_benchmark_and_get_stderr(
    exe: str,
    args: list[str],
    config: str,
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


for benchmark in MANIFEST.keys():
    manifest = MANIFEST[benchmark]

    print(f"Running benchmark {benchmark}...")

    print("Detecting size classes...")
    run_benchmark_and_get_stderr(
        manifest["malloc-exe"],
        manifest.get("exe-args", []),
        {},
        DETECTOR_SO,
        manifest.get("exe-stdin"),
        manifest.get("exe-cwd"),
    )

    print("Starting runs...")
    data = collections.defaultdict(lambda: collections.defaultdict(list))

    for config_name, config in CONFIGURATIONS_FOR_MALLOC.items():
        for alloc_name, alloc in ALLOCATORS.items():
            for i in range(ITERATIONS):
                elapsed = get_elapsed(
                    run_benchmark_and_get_stderr(
                        manifest["malloc-exe"],
                        manifest.get("exe-args", []),
                        config,
                        (alloc + " " + LITTERER_SO).strip(),
                        manifest.get("exe-stdin"),
                        manifest.get("exe-cwd"),
                    )
                )
                data[alloc_name][f"malloc.{config_name}"].append(elapsed)
                print(f"{alloc_name} / malloc / {config_name}: {elapsed} ms")

    for config_name, config in CONFIGURATIONS_FOR_ARENA.items():
        for alloc_name, alloc in ALLOCATORS.items():
            for i in range(ITERATIONS):
                elapsed = get_elapsed(
                    run_benchmark_and_get_stderr(
                        manifest["arena-exe"],
                        manifest.get("exe-args", []),
                        config,
                        (alloc + " " + LITTERER_SO).strip(),
                        manifest.get("exe-stdin"),
                        manifest.get("exe-cwd"),
                    )
                )
                data[alloc_name][f"arena.{config_name}"].append(elapsed)
                print(f"{alloc_name} / arena / {config_name}: {elapsed} ms")

    print(f"Writing results for {benchmark}...")
    with open(os.path.join(ROOT, "graphs", "data", f"{benchmark}.json"), "w") as f:
        json.dump(data, f)
