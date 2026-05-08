import argparse
import json
import math

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
        fig.set_size_inches(4, 3)
        ax.set_facecolor("white")

        bin_edges = [
            8,
            16,
            32,
            48,
            64,
            80,
            96,
            112,
            128,
            160,
            192,
            224,
            256,
            320,
            384,
            448,
            512,
            640,
            768,
            896,
            1024,
            1280,
            1536,
            1792,
            2048,
            2560,
            3072,
            3584,
            4096,
        ]

        # Each bin covers (bin_edges[i-1], bin_edges[i]], first bin covers [1, bin_edges[0]]
        consolidated = []
        for i, edge in enumerate(bin_edges):
            lo = bin_edges[i - 1] if i > 0 else 0
            consolidated.append(bins[lo:edge].sum())

        bar_positions = list(range(len(consolidated)))
        ax.bar(
            bar_positions,
            consolidated,
            width=0.85,
            align="center",
            color=(2 / 255, 81 / 255, 150 / 255),
        )

        ax.set_xlabel("Size [B]")
        ax.set_ylabel("Number of allocations")
        ax.set_xticks(bar_positions)
        ax.set_xticklabels([str(b) for b in bin_edges], rotation=90)
        ax.tick_params(axis="x", length=0)
        ax.tick_params(axis="y", which="minor", length=0)
        ax.set_yscale("log")
        ax.xaxis.grid(False)
        fig.tight_layout()
        plt.savefig(f"distribution.{args.format}", format=args.format)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=str)
    parser.add_argument("--format", type=str, default="png")
    parser.add_argument("--font", type=str, default="Linux Libertine O")

    args = parser.parse_args()
    main(args)
