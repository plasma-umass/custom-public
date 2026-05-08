import argparse
import os
import signal
import subprocess

EVENTS = [
    "context-switches",
    "cpu-migrations",
    "page-faults",
    "cycles",
    "instructions",
    "branches",
    "branch-misses",
    "L1-dcache-loads",
    "L1-dcache-load-misses",
    "LLC-loads",
    "LLC-load-misses",
    "dTLB-loads",
    "dTLB-load-misses",
]


def run(args):
    preload = []
    if args.malloc_so:
        preload.append(args.malloc_so)
    if not args.built_with_litterer:
        preload.append(os.path.join(args.litter_root, "build", "liblitterer.so"))

    env = {
        **os.environ,
        "LD_PRELOAD": " ".join(preload),
        "LITTER_OCCUPANCY": str(args.occupancy),
        "LITTER_SLEEP": str(1),
        "LITTER_MULTIPLIER": str(args.multiplier),
    }

    if args.stdin:
        stdin = open(args.stdin)
    else:
        stdin = subprocess.DEVNULL

    with subprocess.Popen(
        args.command,
        stdin=stdin,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        env=env,
    ) as program:
        for raw_line in iter(program.stderr.readline, b""):
            line = raw_line.decode().strip()
            print(line)
            if line.startswith("Sleeping"):
                perf = subprocess.Popen(
                    ["perf", "stat", "-e", ",".join(EVENTS), "-p", f"{program.pid}"],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.PIPE,
                )
                print(f"Successfully attached to process {program.pid}.")
        return_code = program.wait()
        assert return_code == 0

        perf.send_signal(signal.SIGINT)
        if perf.stderr:
            print(perf.stderr.read().decode().strip())
        if perf.stdout:
            print(perf.stdout.read().decode().strip())
        print(
            "=================================================================================="
        )
        print()

        if perf.stderr:
            perf.stderr.close()
        if perf.stdout:
            perf.stdout.close()
        perf.wait()

    if args.stdin:
        stdin.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-n", type=int, required=True, help="number of iterations.")
    parser.add_argument("--litter-root", type=str, default=".")
    parser.add_argument("--malloc_so", type=str)
    parser.add_argument("--stdin", type=str)
    parser.add_argument("--multiplier", type=int, default=20)
    parser.add_argument("--occupancy", type=float, default=0.95)
    parser.add_argument("--built-with-litterer", action="store_true")
    parser.add_argument(
        "---",
        dest="command",
        default=[],
        help=argparse.SUPPRESS,
        nargs=argparse.REMAINDER,
    )
    args, _ = parser.parse_known_args()

    for _ in range(args.n):
        run(args)


if __name__ == "__main__":
    main()
