import os
import collections
import subprocess
import json
from typing import Optional


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BLENDER_ROOT = os.path.join(ROOT, "blender", "blender")

MANIFEST = {
    # See BLENDER.
    "blender.geometry_nodes": {
        "malloc-exe": os.path.join(BLENDER_ROOT, "build-minus-all", "bin", "blender"),
        "arena-exe": os.path.join(BLENDER_ROOT, "build-vanilla", "bin", "blender"),
        "exe-args": [
            "--factory-startup",
            "--threads",
            "1",
            "-noaudio",
            "--enable-autoexec",
            "--python-exit-code",
            "1",
            "--background",
            f"{BLENDER_ROOT}/tests/benchmarks/geometry_nodes/foreach_geometry_element/foreach_zone_bfield.blend",
            "--python-expr",
            f'import sys, pickle, base64;sys.path.append(r"{BLENDER_ROOT}/tests/performance");import tests.geometry_nodes;args = pickle.loads(base64.b64decode(b"gASVJwAAAAAAAAB9lIwKbG9nX3ByZWZpeJSME2ZvcmVhY2hfem9uZV9iZmllbGSUcy4="));tests.geometry_nodes._run(args)',
        ],
    },
    # See BLENDER.
    "blender.sculpt": {
        "malloc-exe": os.path.join(BLENDER_ROOT, "build-minus-all", "bin", "blender"),
        "arena-exe": os.path.join(BLENDER_ROOT, "build-vanilla", "bin", "blender"),
        "exe-args": [
            "--factory-startup",
            "--threads",
            "1",
            "-noaudio",
            "--enable-autoexec",
            "--python-exit-code",
            "1",
            "--background",
            f"{BLENDER_ROOT}/tests/benchmarks/sculpt/brushes/base.blend",
            "--python-expr",
            f'import sys, pickle, base64;sys.path.append(r"{BLENDER_ROOT}/tests/performance");import tests.sculpt;args = pickle.loads(base64.b64decode(b"gASVjwAAAAAAAAB9lCiMBG1vZGWUjAx0ZXN0cy5zY3VscHSUjApTY3VscHRNb2RllJOUSwOFlFKUjApicnVzaF90eXBllGgCjAlCcnVzaFR5cGWUk5SMC0NsYXkgU3RyaXBzlIWUUpSMD3NwYXRpYWxfcmVvcmRlcpSJjARuYW1llIwTZHludG9wb19jbGF5X3N0cmlwc5R1Lg=="));tests.sculpt._run_brush_test(args)',
        ],
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


def get_elapsed(output: str) -> list[int]:
    lines = output.splitlines()
    if ("-" * 31) not in lines:
        print("\n".join(lines))
    start = lines.index("-" * 31) + 1
    end = lines.index("-" * 31, start)
    return [int(float(x) * 1000) for x in lines[start:end]]


def run_benchmark_and_get_stdout(
    exe: str,
    args: str,
    config: str,
    ld_preload: str,
    stdin: Optional[str],
    cwd: Optional[str],
) -> str:
    env = {
        **os.environ,
        **config,
        "LD_PRELOAD": ld_preload,
        "BLENDER_RESTORE_LD_PRELOAD": "",
    }
    # print(" ".join([exe, *[f"'{a}'" for a in args]]))
    return subprocess.run(
        [exe, *args],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        env=env,
        text=True,
        cwd=cwd,
        stdin=open(stdin) if stdin else None,
    ).stdout


for benchmark in MANIFEST.keys():
    manifest = MANIFEST[benchmark]

    print(f"Running benchmark {benchmark}...")

    print("Detecting size classes...")
    run_benchmark_and_get_stdout(
        manifest["malloc-exe"],
        manifest["exe-args"],
        {},
        DETECTOR_SO,
        manifest.get("exe-stdin"),
        manifest.get("exe-cwd"),
    )

    print("Starting runs...")
    data = collections.defaultdict(lambda: collections.defaultdict(list))

    for config_name, config in CONFIGURATIONS_FOR_MALLOC.items():
        for alloc_name, alloc in ALLOCATORS.items():
            # iterations are built into the benchmark scripts.
            elapsed = get_elapsed(
                run_benchmark_and_get_stdout(
                    manifest["malloc-exe"],
                    manifest["exe-args"],
                    config,
                    (alloc + " " + LITTERER_SO).strip(),
                    manifest.get("exe-stdin"),
                    manifest.get("exe-cwd"),
                )
            )
            data[alloc_name][f"malloc.{config_name}"] = elapsed
            print(
                f"{alloc_name} / malloc / {config_name}: {sum(elapsed) / len(elapsed):.2f} ms"
            )

    for config_name, config in CONFIGURATIONS_FOR_ARENA.items():
        for alloc_name, alloc in ALLOCATORS.items():
            # iterations are built into the benchmark scripts.
            elapsed = get_elapsed(
                run_benchmark_and_get_stdout(
                    manifest["arena-exe"],
                    manifest["exe-args"],
                    config,
                    (alloc + " " + LITTERER_SO).strip(),
                    manifest.get("exe-stdin"),
                    manifest.get("exe-cwd"),
                )
            )
            data[alloc_name][f"arena.{config_name}"] = elapsed
            print(
                f"{alloc_name} / arena / {config_name}: {sum(elapsed) / len(elapsed):.2f} ms"
            )

    print(f"Writing results for {benchmark}...")
    with open(os.path.join(ROOT, "graphs", "data", f"{benchmark}.json"), "w") as f:
        json.dump(data, f)
