import os
import shutil

from structs import RunInfo


def setup(spec_root: str) -> None:
    directory = os.path.join(
        spec_root, "benchspec", "CPU", "605.mcf_s", "run", "spec-utilities-run"
    )

    if os.path.exists(directory):
        shutil.rmtree(directory)
    os.makedirs(directory)
    shutil.copytree(
        os.path.join(
            spec_root, "benchspec", "CPU", "505.mcf_r", "data", "refspeed", "input"
        ),
        os.path.join(directory),
        dirs_exist_ok=True,
    )


def get_run_info(spec_root: str) -> RunInfo:
    directory = os.path.join(
        spec_root, "benchspec", "CPU", "605.mcf_s", "run", "spec-utilities-run"
    )

    executable = os.path.join(
        "..",
        "..",
        "exe",
        "mcf_s_peak.default",
    )

    commands = [[executable, "inp.in"]]

    return RunInfo(directory=directory, commands=commands)
