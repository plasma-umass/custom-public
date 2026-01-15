import argparse
import json
import matplotlib.pyplot as plt
import numpy as np


def main(args: argparse.Namespace) -> None:
    with open(args.input, "r") as f:
        data = np.array(json.load(f))
    data = np.cumsum(data[::-1])[::-1]
    data = np.trim_zeros(data)
    data = np.append(data, [0])
    data = data / data[0]
    plt.plot(data)
    plt.yscale("log")
    plt.xlabel("Sequence Length")
    plt.ylabel("CDF")
    plt.title("Sequence Length CDF (log scale)")
    plt.xlim(0, 64)
    plt.savefig("plt.png", format="png")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=str)
    main(parser.parse_args())
