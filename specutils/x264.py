import os
import shutil

from structs import RunInfo


def setup(spec_root: str) -> None:
    directory = os.path.join(
        spec_root, "benchspec", "CPU", "625.x264_s", "run", "spec-utilities-run"
    )

    if os.path.exists(directory):
        shutil.rmtree(directory)
    os.makedirs(directory)
    shutil.copytree(
        os.path.join(
            spec_root, "benchspec", "CPU", "525.x264_r", "data", "refrate", "input"
        ),
        os.path.join(directory),
        dirs_exist_ok=True,
    )


def get_run_info(spec_root: str) -> RunInfo:
    directory = os.path.join(
        spec_root, "benchspec", "CPU", "625.x264_s", "run", "spec-utilities-run"
    )

    executable = os.path.join(
        "..",
        "..",
        "exe",
        "x264_s_peak.default",
    )

    commands = [
        [
            executable,
            "--pass",
            "1",
            "--stats",
            "x264_stats.log",
            "--bitrate",
            "1000",
            "--frames",
            "1000",
            "-o",
            "BuckBunny_New.264",
            "BuckBunny.yuv",
            "1280x720",
        ],
        [
            executable,
            "--pass",
            "2",
            "--stats",
            "x264_stats.log",
            "--bitrate",
            "1000",
            "--dumpyuv",
            "200",
            "--frames",
            "1000",
            "-o",
            "BuckBunny_New.264",
            "BuckBunny.yuv",
            "1280x720",
        ],
        [
            executable,
            "--seek",
            "500",
            "--dumpyuv",
            "200",
            "--frames",
            "1250",
            "-o",
            "BuckBunny_New.264",
            "BuckBunny.yuv",
            "1280x720",
        ],
    ]

    return RunInfo(directory=directory, commands=commands)
