#! /usr/bin/env python3

import atexit
import itertools
import json
import os
import re
import shutil
import subprocess
import tempfile

N = 5
OUTPUT = "measure.csv"

BENCHMARK_FOOTPRINT = 50_000_000  # 50MB
BENCHMARK_ITERATIONS = 10_000_000

SETTINGS = [
    {"LITTER_MULTIPLIER": "20", "LITTER_OCCUPANCY": "0.95"},
    {"LITTER_MULTIPLIER": "1", "LITTER_OCCUPANCY": "0.4"},
]


def main():
    build_directory = tempfile.mkdtemp()
    atexit.register(shutil.rmtree, build_directory)
    subprocess.run(
        [f"cmake", ".", "-B", build_directory, "-DCMAKE_BUILD_TYPE=Release"],
        check=True,
    )
    subprocess.run([f"cmake", "--build", build_directory, "--parallel"], check=True)

    with open(OUTPUT, "w") as f:
        for s in itertools.chain(
            range(1, 63, 1), range(64, 289, 8), range(320, 4097, 32)
        ):
            print(f"Testing with s = {s}...")
            data = {}
            data["size"] = s

            # 1. Detect.
            subprocess.run(
                [
                    f"{build_directory}/benchmark_iterator",
                    "-f",
                    f"{BENCHMARK_FOOTPRINT}",
                    "-i",
                    f"{BENCHMARK_ITERATIONS}",
                    "-s",
                    f"{s}",
                ],
                cwd=build_directory,
                check=True,
                stdout=subprocess.DEVNULL,
                env={
                    "LD_PRELOAD": f"{build_directory}/libdetector_distribution.so",
                    "DYLD_INSERT_LIBRARIES": f"{build_directory}/libdetector_distribution.dylib",
                    **os.environ,
                },
            )

            row = [s]

            # 2. Run vanilla.
            times = []
            for _ in range(N):
                stdout = subprocess.check_output(
                    [
                        f"{build_directory}/benchmark_iterator",
                        "-f",
                        f"{BENCHMARK_FOOTPRINT}",
                        "-i",
                        f"{BENCHMARK_ITERATIONS}",
                        "-s",
                        f"{s}",
                    ],
                    cwd=build_directory,
                    text=True,
                    env=os.environ,
                )
                times.append(
                    int(re.search(r"Done. Time elapsed: (\d+) ms.", stdout).group(1))
                )
            row.append(sum(times) / N)
            data["vanilla"] = times

            # 3. Run with all settings.
            data["settings"] = []
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
                        [
                            f"{build_directory}/benchmark_iterator",
                            "-f",
                            f"{BENCHMARK_FOOTPRINT}",
                            "-i",
                            f"{BENCHMARK_ITERATIONS}",
                            "-s",
                            f"{s}",
                        ],
                        cwd=build_directory,
                        text=True,
                        stderr=subprocess.DEVNULL,
                        env=env,
                    )
                    times.append(
                        int(
                            re.search(r"Done. Time elapsed: (\d+) ms.", stdout).group(1)
                        )
                    )
                row.append(sum(times) / N)
                data["settings"].append(times)

            f.write(json.dumps(data))
            f.write("\n")
            print("\t".join(str(e) for e in row))


if __name__ == "__main__":
    main()
