import argparse
import json
import matplotlib.pyplot as plt
import numpy as np


def main(args: argparse.Namespace) -> None:
    # Plot setup
    plt.rcParams["pdf.fonttype"] = 42
    plt.rcParams["font.family"] = args.font
    with plt.style.context("bmh"):  # type: ignore[attr-defined]
        fig, ax = plt.subplots()
        fig.set_size_inches(8, 5)
        ax.set_facecolor("white")

        xmax_global = 0

        # Process each file
        for filename in args.filenames:
            with open(filename, "r") as f:
                histogram = json.load(f)["histogram"]
            print(f"\n{filename}:")
            print(f"  Number of bins: {len(histogram)}")
            print(f"  Total accesses: {sum(histogram):,}")

            # Compute CDF
            counts = np.array(histogram)
            total = np.sum(counts)
            cumulative = np.cumsum(counts)
            cdf_values = (cumulative / total) * 100
            reuse_distances = np.arange(len(histogram))

            # Add [0, 0] point.
            reuse_distances = np.insert(reuse_distances, 0, 0)
            cdf_values = np.insert(cdf_values, 0, 0)

            # Calculate xmax for this file
            xmax_idx = np.searchsorted(cdf_values, args.percentile)
            xmax = reuse_distances[xmax_idx]
            xmax_global = max(xmax_global, xmax)
            print(f"  {args.percentile}% percentile at x={xmax}")

            # Plot this curve
            ax.plot(reuse_distances, cdf_values, label=filename)

        # Use user-specified xmax if provided, otherwise use calculated value
        if args.xmax is not None:
            xmax_global = args.xmax
            print(f"\nUsing user-specified xmax={xmax_global}")
        else:
            print(f"\nLimiting x-axis to xmax={xmax_global}")

        ax.set_xlabel("Reuse Distance")
        ax.set_ylabel("Cumulative Probability [%]")
        ax.set_xlim(left=-1, right=xmax_global + xmax_global / 50)
        ymin = args.ymin if args.ymin is not None else -1
        ax.set_ylim(bottom=ymin, top=102)
        ax.legend(loc="lower right")

        fig.tight_layout()
        plt.savefig(f"reuse.{args.format}", format=args.format)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "filenames",
        type=str,
        nargs="+",
        help="path(s) to JSON file(s) containing output from the Pin tool",
    )
    parser.add_argument("--format", type=str, default="png")
    parser.add_argument("--font", type=str, default="Linux Libertine O")
    parser.add_argument(
        "--percentile",
        type=float,
        default=90.0,
        help="percentile of data to display on x-axis",
    )
    parser.add_argument("--xmax", type=float, default=None)
    parser.add_argument("--ymin", type=float, default=None)
    main(parser.parse_args())
