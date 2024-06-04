#! /usr/bin/env python3

import atexit
import argparse
import re
import shutil
import statistics
import subprocess
import tempfile


def main(args: argparse.Namespace) -> None:
    build_directory = tempfile.mkdtemp()
    atexit.register(shutil.rmtree, build_directory)
    subprocess.run(
        ["cmake", ".", "-B", build_directory, "-DCMAKE_BUILD_TYPE=Release"], check=True
    )
    subprocess.run(["cmake", "--build", build_directory, "--parallel"], check=True)

    for i in range(19, 20):
        runtimes = []
        dtlb_miss_rate = []
        llc_miss_rate = []
        dtlb_accesses = []
        llc_total_accesses = []

        for _ in range(args.n):
            stdout = subprocess.check_output(
                [
                    f"{build_directory}/benchmark_iterator",
                    f"--number-objects={1 << i}",
                    f"--allocation-size={args.allocation_size}",
                    f"--iterations={args.iterations}",
                    f"--allocation-policy={args.allocation_policy}",
                ],
                text=True,
            )

            runtime = int(re.search(r"Done. Time elapsed: (\d+) ms.", stdout).group(1))
            dtlb_read_misses = int(
                re.search(
                    r"PERF_COUNT_HW_CACHE_DTLB \| PERF_COUNT_HW_CACHE_OP_READ \| PERF_COUNT_HW_CACHE_RESULT_MISS = (\d+)",
                    stdout,
                ).group(1)
            )
            dtlb_read_accesses = int(
                re.search(
                    r"PERF_COUNT_HW_CACHE_DTLB \| PERF_COUNT_HW_CACHE_OP_READ \| PERF_COUNT_HW_CACHE_RESULT_ACCESS = (\d+)",
                    stdout,
                ).group(1)
            )
            dtlb_write_misses = int(
                re.search(
                    r"PERF_COUNT_HW_CACHE_DTLB \| PERF_COUNT_HW_CACHE_OP_WRITE \| PERF_COUNT_HW_CACHE_RESULT_MISS = (\d+)",
                    stdout,
                ).group(1)
            )
            dtlb_write_accesses = int(
                re.search(
                    r"PERF_COUNT_HW_CACHE_DTLB \| PERF_COUNT_HW_CACHE_OP_WRITE \| PERF_COUNT_HW_CACHE_RESULT_ACCESS = (\d+)",
                    stdout,
                ).group(1)
            )
            llc_misses = int(
                re.search(r"PERF_COUNT_HW_CACHE_MISSES = (\d+)", stdout).group(1)
            )
            llc_accesses = int(
                re.search(r"PERF_COUNT_HW_CACHE_REFERENCES = (\d+)", stdout).group(1)
            )

            runtimes.append(runtime)
            dtlb_miss_rate.append(
                100
                * (dtlb_read_misses + dtlb_write_misses)
                / (dtlb_read_accesses + dtlb_write_accesses)
            )
            llc_miss_rate.append(100 * llc_misses / llc_accesses)
            dtlb_accesses.append(dtlb_read_accesses + dtlb_write_accesses)
            llc_total_accesses.append(llc_accesses)

        print(
            ";".join(
                [
                    f"{1 << i}",
                    f"{statistics.median(runtimes)}",
                    f"{statistics.median(dtlb_miss_rate):.2f}",
                    f"{statistics.median(llc_miss_rate):.2f}",
                    f"{statistics.median(dtlb_accesses)}",
                    f"{statistics.median(llc_total_accesses)}",
                ]
            )
        )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    # Parameters passed directly to benchmark.
    parser.add_argument("--iterations", "-i", type=int, required=True)
    parser.add_argument(
        "--allocation-size",
        "-s",
        type=int,
        default=4096,
        help="size of each object in bytes",
    )
    parser.add_argument("--allocation-policy", "-p", type=str, required=True)

    # Script-specific parameters.
    parser.add_argument(
        "-n", type=int, default=11, help="number of runs per configuration"
    )
    main(parser.parse_args())
