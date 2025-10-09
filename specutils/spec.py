import argparse
import json
import os
import subprocess
import time

import mcf  # 605
import gcc  # 602
import perlbench  # 600
import omnetpp  # 620
import xalancbmk  # 623
import x264  # 625


def main(args: argparse.Namespace) -> None:
    if args.benchmark == 600:
        perlbench.setup(args.root)
        run_info = perlbench.get_run_info(args.root)
    if args.benchmark == 602:
        gcc.setup(args.root)
        run_info = gcc.get_run_info(args.root)
    if args.benchmark == 605:
        mcf.setup(args.root)
        run_info = mcf.get_run_info(args.root)
    if args.benchmark == 620:
        omnetpp.setup(args.root)
        run_info = omnetpp.get_run_info(args.root)
    if args.benchmark == 623:
        xalancbmk.setup(args.root)
        run_info = xalancbmk.get_run_info(args.root)
    if args.benchmark == 625:
        x264.setup(args.root)
        run_info = x264.get_run_info(args.root)
    else:
        raise ValueError("unknown benchmark number")

    env = {
        "OMP_NUM_THREADS": str(args.j),
        "OMP_THREAD_LIMIT": str(args.j),
        **json.loads(args.environment),
    }

    runs = []
    for i in range(args.n):
        start = time.time()
        for command in run_info.commands:
            subprocess.run(
                command,
                cwd=run_info.directory,
                env=env,
                # stdout=subprocess.DEVNULL,
                # stderr=subprocess.DEVNULL,
                check=True,
            )
        elapsed = time.time() - start
        print(f"Run #{i + 1}: {elapsed:.2f} seconds")
        runs.append(elapsed)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=str)
    parser.add_argument(
        "benchmark",
        type=int,
        choices=[600, 602, 605, 620, 623, 625, 631, 641, 657, 619, 638, 644],
    )
    parser.add_argument(
        "--environment",
        type=str,
        default="{}",
        help="JSON object of extra environment variables",
    )
    parser.add_argument(
        "-j",
        type=int,
        default=os.cpu_count(),
        help="will set OMP_NUM_THREADS and OMP_THREAD_LIMIT",
    )
    parser.add_argument("-n", type=int, default=1)
    main(parser.parse_args())
