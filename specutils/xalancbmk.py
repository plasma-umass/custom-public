import os
import shutil

from structs import RunInfo


def setup(spec_root: str) -> None:
    directory = os.path.join(
        spec_root, "benchspec", "CPU", "623.xalancbmk_s", "run", "spec-utilities-run"
    )

    if os.path.exists(directory):
        shutil.rmtree(directory)
    os.makedirs(directory)
    shutil.copytree(
        os.path.join(
            spec_root, "benchspec", "CPU", "523.xalancbmk_r", "data", "refrate", "input"
        ),
        os.path.join(directory),
        dirs_exist_ok=True,
    )


def get_run_info(spec_root: str) -> RunInfo:
    directory = os.path.join(
        spec_root, "benchspec", "CPU", "623.xalancbmk_s", "run", "spec-utilities-run"
    )

    executable = os.path.join(
        "..",
        "..",
        "exe",
        "xalancbmk_s_peak.default",
    )

    commands = [[executable, "-v", "t5.xml", "xalanc.xsl"]]

    return RunInfo(directory=directory, commands=commands)
