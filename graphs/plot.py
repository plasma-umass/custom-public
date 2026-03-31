import argparse
import json
import statistics

import matplotlib.pyplot as plt

CONFIGS = [
    "arena.0litter",
    "arena.3litter",
    "malloc.0litter",
    "malloc.1litter",
    "malloc.3litter",
]

CONFIG_TO_LEGEND = {
    "arena.0litter": "Arena - $Adv_0$",
    "arena.3litter": "Arena - $Adv_3$",
    "malloc.0litter": "Malloc - $Adv_0$",
    "malloc.1litter": "Malloc - $Adv_1$",
    "malloc.3litter": "Malloc - $Adv_3$",
}


def main(args: argparse.Namespace) -> None:
    with open(args.file) as f:
        data = json.load(f)

    plt.rcParams["pdf.fonttype"] = 42
    plt.rcParams["font.family"] = args.font
    with plt.style.context("bmh"):
        fig, ax = plt.subplots()
        fig.set_size_inches(8, 2)
        ax.set_facecolor("white")

        width_per_allocator = 0.7
        bar_width = 0.15
        space_between = 0.05

        arena_base = (253 / 255, 179 / 255, 56 / 255)
        malloc_base = (2 / 255, 81 / 255, 150 / 255)
        colors = {
            "arena.0litter": (*arena_base, 0.5),
            "arena.3litter": (*arena_base, 1.0),
            "malloc.0litter": (*malloc_base, 0.33),
            "malloc.1litter": (*malloc_base, 0.66),
            "malloc.3litter": (*malloc_base, 1.0),
        }

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
                # yerr = statistics.stdev(values) / 1000 if len(values) > 1 else 0

                x = j * 0.15 + bar_width / 2
                x += space_between * j if j < 2 else space_between * (j + 1)
                x = (x * width_per_allocator) + (1 - width_per_allocator) / 2
                x += i

                width = bar_width * width_per_allocator

                ax.bar(
                    x,
                    y,
                    width,
                    color=colors[config],
                    label=CONFIG_TO_LEGEND[config] if i == 0 else None,
                    # yerr=yerr,
                    # error_kw={"capsize": 3, "elinewidth": 1},
                )

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

        ax.legend(loc="upper center", bbox_to_anchor=(0.5, 1.5), ncol=5)
        ax.set_ylabel("Normalized Time" if args.normalize else "Execution Time [s]")
        ax.set_xticks([x + 0.5 for x in range(len(data.keys()))])
        ax.set_xticklabels([alloc + "malloc" for alloc in data.keys()])
        ax.tick_params(axis="x", which="both", bottom=False)
        ax.xaxis.grid(False)
        ax.yaxis.grid(False)
        ax.set_ylim(bottom=0)
        fig.tight_layout()
        plt.savefig(f"plot.{args.format}", format=args.format)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("file", type=str)
    parser.add_argument("--font", type=str, default="Linux Libertine O")
    parser.add_argument("--format", type=str, default="png")
    parser.add_argument("--normalize", action="store_true", default=False)
    main(parser.parse_args())
