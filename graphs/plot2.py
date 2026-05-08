import argparse
import json
import os
import statistics

import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

BENCHMARKS = [
    "mudlle",
    "175.vpr",
    "176.gcc",
    "197.parser",
    "llvm",
    "blender.geometry_nodes",
]

BENCHMARK_TO_NAME = {
    "mudlle": "mudlle",
    "175.vpr": "175.vpr",
    "176.gcc": "176.gcc",
    "197.parser": "197.parser",
    "llvm": "clang",
    "blender.geometry_nodes": "geometry_nodes",
}

ALLOCATORS = ["pt", "je", "mi"]

ALLOCATOR_TO_NAME = {
    "pt": "glibc malloc",
    "je": "jemalloc",
    "mi": "mimalloc",
}

arena_base = (253 / 255, 179 / 255, 56 / 255)
malloc_base = (2 / 255, 81 / 255, 150 / 255)

MALLOC_COLORS = {
    "pt": (*malloc_base, 0.33),
    "je": (*malloc_base, 0.66),
    "mi": (*malloc_base, 1.00),
}

ARENA_COLOR = (*arena_base, 1.0)

BAR_WIDTH = 0.14
BAR_GAP = 0.03
BAR_PADDING = (1.0 - (4 * BAR_WIDTH + 3 * BAR_GAP)) / 2


def bar_x(benchmark_index, bar_index):
    return (
        benchmark_index
        + BAR_PADDING
        + bar_index * (BAR_WIDTH + BAR_GAP)
        + BAR_WIDTH / 2
    )


def bar_value(values, baseline):
    if not values:
        return 0, 0
    m = statistics.mean(values)
    s = statistics.stdev(values) if len(values) > 1 else 0
    if baseline is not None:
        return m / baseline, s / baseline
    return m / 1000, s / 1000


def plot(ax, all_data, args):
    error_kw = {"capsize": 3, "elinewidth": 1} if args.yerr else None

    for b_idx, benchmark in enumerate(BENCHMARKS):
        data = all_data[benchmark]

        means = {a: statistics.mean(data[a]["arena.0litter"]) for a in ALLOCATORS}
        best_alloc = min(means, key=means.__getitem__)
        baseline = means[best_alloc] if args.normalize else None

        y, yerr = bar_value(data[best_alloc]["arena.0litter"], baseline)
        ax.bar(
            bar_x(b_idx, 0),
            y,
            BAR_WIDTH,
            color=ARENA_COLOR,
            label="Custom - $Adv_0$ (best)" if b_idx == 0 else None,
            yerr=yerr if args.yerr else None,
            error_kw=error_kw,
        )
        print(f"{benchmark} arena.0litter ({best_alloc}): {y:.3f} ± {yerr:.3f}")

        for j, alloc in enumerate(ALLOCATORS):
            y, yerr = bar_value(data[alloc]["malloc.0litter"], baseline)
            ax.bar(
                bar_x(b_idx, j + 1),
                y,
                BAR_WIDTH,
                color=MALLOC_COLORS[alloc],
                label=(
                    f"Malloc - $Adv_0$ ({ALLOCATOR_TO_NAME[alloc]})"
                    if b_idx == 0
                    else None
                ),
                yerr=yerr if args.yerr else None,
                error_kw=error_kw,
            )
            print(f"{benchmark} malloc.0litter ({alloc}): {y:.3f} ± {yerr:.3f}")

    n = len(BENCHMARKS)
    for i in range(1, n):
        ax.axvline(i, linewidth=1, color=(0, 0, 0, 0.5))

    ref_y = (
        1.0
        if args.normalize
        else statistics.mean(all_data[BENCHMARKS[0]]["pt"]["arena.0litter"]) / 1000
    )
    ax.axhline(ref_y, lw=1, color=(0, 0, 0, 0.5), linestyle="dotted")

    ax.set_xlim(0, n)
    ax.set_xticks([i + 0.5 for i in range(n)])
    ax.set_xticklabels([BENCHMARK_TO_NAME[b] for b in BENCHMARKS])
    ax.tick_params(axis="x", which="both", bottom=False)
    ax.xaxis.grid(False)
    ax.yaxis.grid(False)
    ax.set_ylim(bottom=0)
    ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.1f"))
    ax.set_facecolor("white")


def main(args: argparse.Namespace) -> None:
    all_data = {}
    for path in args.files:
        name = os.path.splitext(os.path.basename(path))[0]
        with open(path) as f:
            all_data[name] = json.load(f)

    plt.rcParams["pdf.fonttype"] = 42
    plt.rcParams["font.family"] = args.font
    with plt.style.context("bmh"):
        fig, ax = plt.subplots()
        scale = 1.5  # How much expect scale to be reduced.
        fig.set_size_inches(6.5 * scale, 1.5 * scale)

        plot(ax, all_data, args)
        ax.set_ylabel("Normalized Time" if args.normalize else "Execution Time [s]")

        handles, labels = ax.get_legend_handles_labels()
        fig.legend(
            handles,
            labels,
            loc="upper center",
            bbox_to_anchor=(0.5, 1.0),
            ncol=4,
            bbox_transform=fig.transFigure,
            columnspacing=0.8,
            handletextpad=0.4,
        )

        fig.tight_layout(rect=(0, 0, 1, args.top))
        plt.savefig(f"plot.{args.format}", format=args.format)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("files", type=str, nargs="+")
    parser.add_argument("--font", type=str, default="Linux Libertine O")
    parser.add_argument("--format", type=str, default="png")
    parser.add_argument("--normalize", action="store_true", default=False)
    parser.add_argument("--yerr", action="store_true", default=False)
    parser.add_argument("--top", type=float, default=0.88)
    main(parser.parse_args())
