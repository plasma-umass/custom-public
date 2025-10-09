import os
import shutil

from structs import RunInfo


def setup(spec_root: str) -> None:
    directory = os.path.join(
        spec_root, "benchspec", "CPU", "600.perlbench_s", "run", "spec-utilities-run"
    )

    if os.path.exists(directory):
        shutil.rmtree(directory)
    os.makedirs(directory)
    shutil.copytree(
        os.path.join(
            spec_root, "benchspec", "CPU", "500.perlbench_r", "data", "refrate", "input"
        ),
        os.path.join(directory),
        dirs_exist_ok=True,
    )
    shutil.copytree(
        os.path.join(
            spec_root, "benchspec", "CPU", "500.perlbench_r", "data", "all", "input"
        ),
        os.path.join(directory),
        dirs_exist_ok=True,
    )


def get_run_info(spec_root: str) -> RunInfo:
    directory = os.path.join(
        spec_root, "benchspec", "CPU", "600.perlbench_s", "run", "spec-utilities-run"
    )

    executable = os.path.join("..", "..", "exe", "perlbench_s_peak.default")

    commands = [
        [
            executable,
            "-I./lib",
            "checkspam.pl",
            "2500",
            "5",
            "25",
            "11",
            "150",
            "1",
            "1",
            "1",
            "1",
        ],
        [executable, "-I./lib", "diffmail.pl", "4", "800", "10", "17", "19", "300 "],
        [executable, "-I./lib", "splitmail.pl", "6400", "12", "26", "16", "100", "0"],
    ]

    return RunInfo(directory=directory, commands=commands)
