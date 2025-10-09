from dataclasses import dataclass


@dataclass
class RunInfo:
    directory: str
    commands: list[list[str]]
