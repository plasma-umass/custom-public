import argparse
import json
import os
import statistics

import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

CONFIGS = [
    "arena.0litter",
    "arena.3litter",
    "malloc.0litter",
    "malloc.1litter",
    "malloc.3litter",
    "malloc.10litter",
]

CONFIG_TO_LEGEND = {
    "arena.0litter": "Region - $Adv_0$",
    "arena.3litter": "Region - $Adv_3$",
    "malloc.0litter": "Malloc - $Adv_0$",
    "malloc.1litter": "Malloc - $Adv_1$",
    "malloc.3litter": "Malloc - $Adv_3$",
    "malloc.10litter": "Malloc - $Adv_{10}$",
}

BENCHMARK_TO_NAME = {
    "175.vpr": "175.vpr",
    "176.gcc": "176.gcc",
    "197.parser": "197.parser",
    "blender.geometry_nodes": "geometry_nodes",
    "blender.sculpt": "sculpt",
    "llvm": "clang",
}

ALLOCATOR_TO_NAME = {
    "pt": "glibc malloc",
    "je": "jemalloc",
    "mi": "mimalloc",
}


def plot_row(ax, data, args, baseline=None):
    width_per_allocator = 0.8
    bar_width = 0.15 * 5 / 6
    space_between = 0.05 * 5 / 6

    assert abs(len(CONFIGS) * (bar_width + space_between) - 1) < 0.01

    arena_base = (253 / 255, 179 / 255, 56 / 255)
    malloc_base = (2 / 255, 81 / 255, 150 / 255)
    colors = {
        "arena.0litter": (*arena_base, 0.5),
        "arena.3litter": (*arena_base, 1.0),
        "malloc.0litter": (*malloc_base, 0.25),
        "malloc.1litter": (*malloc_base, 0.50),
        "malloc.3litter": (*malloc_base, 0.75),
        "malloc.10litter": (*malloc_base, 1.0),
    }

    if baseline is None:
        baseline = (
            statistics.mean(data["pt"]["arena.0litter"]) if args.normalize else None
        )

    for i, allocator in enumerate(data.keys()):
        for j, config in enumerate(CONFIGS):
            values = data[allocator][config]
            if baseline is not None:
                y = (statistics.mean(values) / baseline) if values else 0
            else:
                y = statistics.mean(values) / 1000 if values else 0

            x = j * bar_width + bar_width / 2
            x += space_between * j if j < 2 else space_between * (j + 1)
            x = (x * width_per_allocator) + (1 - width_per_allocator) / 2
            x += i

            width = bar_width * width_per_allocator

            # yerr = (
            #     (statistics.stdev(values) / baseline)
            #     if values and baseline
            #     else (
            #         0
            #         if args.normalize
            #         else (statistics.stdev(values) / 1000) if values else 0
            #     )
            # )

            ax.bar(
                x,
                y,
                width,
                color=colors[config],
                label=CONFIG_TO_LEGEND[config] if i == 0 else None,
                # yerr=yerr,
                # error_kw={"capsize": 3, "elinewidth": 1},
            )
            print(f"{allocator} {config}: {y}")

    _, ymax = ax.get_ylim()
    for i in range(len(data.keys())):
        ax.axline(
            (
                i
                + (bar_width + space_between) * 2 * width_per_allocator
                + (1 - width_per_allocator) / 2,
                0,
            ),
            (
                i
                + (bar_width + space_between) * 2 * width_per_allocator
                + (1 - width_per_allocator) / 2,
                ymax,
            ),
            linewidth=1,
            color=(0, 0, 0, 0.2),
        )

    for i in range(1, len(data.keys())):
        ax.axline(
            (i, 0),
            (i, ymax),
            linewidth=1,
            color=(0, 0, 0, 0.5),
        )

    ax.axline(
        (
            0,
            (
                1.0
                if args.normalize
                else statistics.mean(data["pt"]["arena.0litter"]) / 1000
            ),
        ),
        slope=0,
        lw=1,
        color=(0, 0, 0, 0.5),
        linestyle="dotted",
    )

    ax.set_xlim(0, len(data.keys()))
    ax.set_xticks([x + 0.5 for x in range(len(data.keys()))])
    ax.set_xticklabels([ALLOCATOR_TO_NAME.get(alloc, alloc) for alloc in data.keys()])
    ax.tick_params(axis="x", which="both", bottom=False)
    ax.xaxis.grid(False)
    ax.yaxis.grid(False)
    ax.set_ylim(bottom=0)
    ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.1f"))
    ax.set_facecolor("white")


def main(args: argparse.Namespace) -> None:
    all_data = []
    for path in args.files:
        with open(path) as f:
            name = os.path.splitext(os.path.basename(path))[0]
            all_data.append((name, json.load(f)))

    n = len(all_data)

    plt.rcParams["pdf.fonttype"] = 42
    plt.rcParams["font.family"] = args.font
    with plt.style.context("bmh"):
        fig, axes = plt.subplots(nrows=n, squeeze=False)
        axes = [axes[i][0] for i in range(n)]
        fig.set_size_inches(9, 2 * n)

        for i, (name, data) in enumerate(all_data):
            ax = axes[i]
            plot_row(ax, data, args)
            ax.set_ylabel(BENCHMARK_TO_NAME.get(name, name))

        handles, labels = axes[0].get_legend_handles_labels()
        fig.legend(
            handles,
            labels,
            loc="upper center",
            bbox_to_anchor=(0.5, 1.0),
            ncol=6,
            bbox_transform=fig.transFigure,
        )

        fig.supylabel("Normalized Time" if args.normalize else "Execution Time [s]")
        fig.tight_layout()
        fig.subplots_adjust(top=0.85 if n == 1 else 0.88 if n == 2 else 0.92)
        plt.savefig(f"plot.{args.format}", format=args.format, bbox_inches="tight")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("files", type=str, nargs="+")
    parser.add_argument("--font", type=str, default="Linux Libertine O")
    parser.add_argument("--format", type=str, default="png")
    parser.add_argument("--normalize", action="store_true", default=False)
    main(parser.parse_args())
