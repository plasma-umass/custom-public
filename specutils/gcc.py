import os
import shutil

from structs import RunInfo


def setup(spec_root: str) -> None:
    directory = os.path.join(
        spec_root, "benchspec", "CPU", "602.gcc_s", "run", "spec-utilities-run"
    )

    if os.path.exists(directory):
        shutil.rmtree(directory)
    os.makedirs(directory)
    shutil.copytree(
        os.path.join(
            spec_root, "benchspec", "CPU", "502.gcc_r", "data", "refspeed", "input"
        ),
        os.path.join(directory),
        dirs_exist_ok=True,
    )


def get_run_info(spec_root: str) -> RunInfo:
    directory = os.path.join(
        spec_root, "benchspec", "CPU", "602.gcc_s", "run", "spec-utilities-run"
    )

    executable = os.path.join(
        "..",
        "..",
        "exe",
        "sgcc_peak.default",
    )

    commands = [
        [
            executable,
            "gcc-pp.c",
            "-O5",
            "-fipa-pta",
            "-o",
            "gcc-pp.opts-O5_-fipa-pta.s",
        ],
        [
            executable,
            "gcc-pp.c",
            "-O5",
            "-finline-limit=1000",
            "-fselective-scheduling",
            "-fselective-scheduling2",
        ],
        [
            executable,
            "gcc-pp.c",
            "-O5",
            "-finline-limit=24000",
            "-fgcse",
            "-fgcse-las",
            "-fgcse-lm",
            "-fgcse-sm",
        ],
    ]

    return RunInfo(directory=directory, commands=commands)
