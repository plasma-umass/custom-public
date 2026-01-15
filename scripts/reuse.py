import argparse
import json
import matplotlib.pyplot as plt
import numpy as np


def main(args: argparse.Namespace):
    with open(args.filename, "r") as f:
        histogram = json.load(f)["histogram"]
    print(f"Number of bins: {len(histogram)}")
    print(f"Total accesses: {sum(histogram):,}")

    # Compute CDF
    counts = np.array(histogram)
    total = np.sum(counts)
    cumulative = np.cumsum(counts)
    cdf_values = (cumulative / total) * 100
    reuse_distances = np.arange(len(histogram))

    # Add [0, 0] point.
    reuse_distances = np.insert(reuse_distances, 0, 0)
    cdf_values = np.insert(cdf_values, 0, 0)

    # Calculate xmax based on percentile
    xmax_idx = np.searchsorted(cdf_values, args.percentile)
    xmax = reuse_distances[xmax_idx]
    print(f"Limiting x-axis to show {args.percentile}% of values (xmax={xmax})")

    # Plot
    plt.rcParams["pdf.fonttype"] = 42
    plt.rcParams["font.family"] = args.font
    with plt.style.context("bmh"):
        fig, ax = plt.subplots()
        fig.set_size_inches(8, 5)
        ax.set_facecolor("white")

        ax.plot(reuse_distances, cdf_values)

        ax.set_xlabel("Reuse Distance")
        ax.set_ylabel("Cumulative Probability [%]")
        ax.set_xlim(left=-xmax / 50, right=xmax + xmax / 50)
        ax.set_ylim(bottom=0, top=100)

        fig.tight_layout()
        plt.savefig(f"reuse.{args.format}", format=args.format)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "filename",
        type=str,
        help="path to the JSON file containing output from the Pin tool",
    )
    parser.add_argument("--format", type=str, default="png")
    parser.add_argument("--font", type=str, default="Linux Libertine O")
    parser.add_argument(
        "--percentile",
        type=float,
        default=90.0,
        help="percentile of data to display on x-axis",
    )
    main(parser.parse_args())
