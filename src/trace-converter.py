import argparse
import json
import math

SIZE_CLASSES = [
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
    5120,
    6144,
    7168,
    8192,
    10240,
    12288,
    14336,
    16384,
    20480,
    24576,
    28672,
    32768,
    40960,
    49152,
    57344,
    65536,
    81920,
    98304,
    114688,
    131072,
    163840,
    196608,
    229376,
    262144,
    327680,
    393216,
    458752,
    524288,
    655360,
    786432,
    917504,
    1048576,
    1310720,
    1572864,
    1835008,
    2097152,
    2621440,
    3145728,
    3670016,
    4194304,
    5242880,
    6291456,
    7340032,
    8388608,
    10485760,
    12582912,
    14680064,
    16777216,
    20971520,
    25165824,
    29360128,
    33554432,
    41943040,
    50331648,
    58720256,
    67108864,
]


def main(args):
    bins = [0] * len(SIZE_CLASSES)
    nAllocations = 0
    maxLiveAllocations = 0
    liveAllocations = 0
    with open(args.input, "r") as f:
        for line in f:
            data = json.loads(line)
            event = data["type"]
            if event == "malloc" or event == "calloc" or event == "realloc":
                assert len(data["args"]) > 0
                size = math.prod(data["args"])
                index = 0
                while size > SIZE_CLASSES[index] and index < len(SIZE_CLASSES):
                    index += 1
                bins[index] += 1
                nAllocations += 1
            if event == "malloc" or event == "calloc":
                liveAllocations += 1
                maxLiveAllocations = max(maxLiveAllocations, liveAllocations)
            if event == "free":
                liveAllocations -= 1

    with open(args.output, "w") as f:
        f.write(
            json.dumps(
                {
                    "SizeClasses": SIZE_CLASSES,
                    "Bins": bins,
                    "NAllocations": nAllocations,
                    "MaxLiveAllocations": maxLiveAllocations,
                }
            )
        )
        f.write("\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("input", help="input trace file")
    parser.add_argument("output", help="output file")
    main(parser.parse_args())
