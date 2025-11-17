import argparse
import os


def main(args: argparse.Namespace) -> None:
    benchmarks = [
        f[:-4]
        for f in os.listdir(args.log_directory)
        if os.path.isfile(os.path.join(args.log_directory, f)) and f.endswith(".log")
    ]

    benchmarks.sort()
    for benchmark in benchmarks:
        log_path = os.path.join(args.log_directory, f"{benchmark}.log")
        log_times_ms = []
        with open(log_path, "r") as f:
            for line in f.readlines():
                if line.startswith("Time elapsed: "):
                    time_ms = int(line[len("Time elapsed: ") : -len(" ms")])
                    log_times_ms.append(time_ms)

        assert len(log_times_ms) % args.n == 0

        width = len(log_times_ms) // args.n
        times_ms = [
            sum(log_times_ms[i : i + width]) for i in range(0, len(log_times_ms), width)
        ]
        print(benchmark, times_ms)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("n", type=int, help="the number of runs per log file")
    parser.add_argument(
        "--log-directory", type=str, help="where the log files are located"
    )
    main(parser.parse_args())
