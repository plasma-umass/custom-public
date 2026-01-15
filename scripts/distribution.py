import argparse
import json

import matplotlib.pyplot as plt
import numpy as np


def main(args: argparse.Namespace) -> None:
    with open(args.path, "r") as f:
        data = json.load(f)

    # Only work with these size classes; if we add other modes this will need updating.
    assert data["sizeClasses"] == list(range(1, 4097))

    bins = np.array(data["bins"])
    ignored = data["ignored"]
    print(
        f"Number of ignored allocations: {ignored} ({(ignored / sum(bins)):.1f}% of total)"
    )

    plt.rcParams["pdf.fonttype"] = 42
    plt.rcParams["font.family"] = args.font
    with plt.style.context("bmh"):  # type: ignore[attr-defined]
        fig, ax = plt.subplots()
        ax.set_facecolor("white")

        consolidated = bins.reshape(-1, 16).sum(axis=1)

        ax.bar(list(range(len(consolidated))), consolidated, width=1, align="edge")

        ax.set_xlabel("Size of allocation [B]")
        ax.set_ylabel("Number of allocations")
        ax.set_yscale("log")
        fig.tight_layout()
        plt.savefig(f"distribution.{args.format}", format=args.format)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=str)
    parser.add_argument("--format", type=str, default="png")
    parser.add_argument("--font", type=str, default="Linux Libertine O")

    args = parser.parse_args()
    main(args)
