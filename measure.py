#! /usr/bin/env python3

import itertools
import os
import re
import shutil
import subprocess
import tempfile

N = 5
OUTPUT = "measure.csv"

BENCHMARK_FOOTPRINT = 50_000_000 # 50MB
BENCHMARK_ITERATIONS = 10_000_000_000

SETTINGS = [
    {"LITTER_MULTIPLIER": "20", "LITTER_OCCUPANCY": "0.95"},
    {"LITTER_MULTIPLIER": "1", "LITTER_OCCUPANCY": "0.4"},
    {"LITTER_MULTIPLIER": "5", "LITTER_OCCUPANCY": "0"},
    {"LITTER_MULTIPLIER": "5", "LITTER_OCCUPANCY": "1"},
]

def main():
    build_directory = tempfile.mkdtemp()
    subprocess.run(
        f"cmake . -B {build_directory} -DCMAKE_BUILD_TYPE=Release",
        shell=True,
        check=True,
    )
    subprocess.run(
        f"cmake --build {build_directory} --parallel", shell=True, check=True
    )

    with open(OUTPUT, "w") as f:
        for s in itertools.chain(range(8, 289, 8), range(320, 4097, 32)):
            print(f"Testing with s = {s}...")

            # 1. Detect.
            subprocess.run(
                f"{build_directory}/benchmark_iterator -f {BENCHMARK_FOOTPRINT} -i {BENCHMARK_ITERATIONS} -s {s}",
                shell=True,
                check=True,
                env={
                    "LD_PRELOAD": f"{build_directory}/libdetector_distribution.so",
                    "DYLD_INSERT_LIBRARIES": f"{build_directory}/libdetector_distribution.dylib",
                    **os.environ,
                },
                stdout=subprocess.DEVNULL,
            )

            row = [s]

            # 2. Run vanilla.
            times = []
            for _ in range(N):
                stdout = subprocess.check_output(
                    f"{build_directory}/benchmark_iterator -f {BENCHMARK_FOOTPRINT} -i {BENCHMARK_ITERATIONS} -s {s}",
                    shell=True,
                    text=True,
                    stderr=subprocess.DEVNULL,
                    env=os.environ,
                )
                times.append(int(re.search(r"Done. Time elapsed: (\d+) ms.", stdout).group(1)))
            row.append(sum(times) / N)

            # 3. Run with all settings.
            for setting in SETTINGS:
                env = {
                    "LD_PRELOAD": f"{build_directory}/liblitterer_distribution_standalone.so",
                    "DYLD_INSERT_LIBRARIES": f"{build_directory}/liblitterer_distribution_standalone.dylib",
                    **setting,
                    **os.environ,
                }
                times = []
                for _ in range(N):
                    stdout = subprocess.check_output(
                        f"{build_directory}/benchmark_iterator -f {BENCHMARK_FOOTPRINT} -i {BENCHMARK_ITERATIONS} -s {s}",
                        shell=True,
                        text=True,
                        stderr=subprocess.DEVNULL,
                        env=env,
                    )
                    times.append(int(re.search(r"Done. Time elapsed: (\d+) ms.", stdout).group(1)))
                row.append(sum(times) / N)
            
            
            f.write("\t".join(str(e) for e in row) + "\n")
            print("\t".join(str(e) for e in row))
            
    shutil.rmtree(build_directory)


if __name__ == "__main__":
    main()
