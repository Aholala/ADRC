"""Build the firmware TD core on the host, validate it, and plot its output."""

import argparse
import csv
import os
from pathlib import Path
import shutil
import subprocess
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


def main():
    repo = Path(__file__).resolve().parents[3]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=repo / "build" / "td-sim")
    parser.add_argument("--compiler", default="g++")
    args = parser.parse_args()
    compiler = shutil.which(args.compiler)
    if compiler is None:
        parser.error(f"C++ compiler not found: {args.compiler}")
    # MSYS2 编译器子进程及生成的程序需要同目录的运行库；只修改子进程环境。
    environment = os.environ.copy()
    environment["PATH"] = str(Path(compiler).parent) + os.pathsep + environment.get("PATH", "")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    core = repo / "Algorithm" / "ADRC"
    suffix = ".exe" if sys.platform == "win32" else ""
    common = [compiler, "-std=c++11", "-O2", "-Wall", "-Wextra", "-Werror", "-pedantic",
              "-I", str(core), str(core / "td.cpp")]
    for name, source in [
        ("td_test", repo / "tests" / "linear_td_test.cpp"),
        ("td_sim", core / "examples" / "td_sim.cpp"),
    ]:
        executable = output / (name + suffix)
        subprocess.run(common + [str(source), "-o", str(executable)], check=True,
                       env=environment)
        command = [str(executable)]
        if name == "td_sim":
            command.append(str(output / "td_sim.csv"))
        subprocess.run(command, check=True, env=environment)

    data = {}
    with (output / "td_sim.csv").open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            case = row.pop("case")
            data.setdefault(case, []).append(
                {key: float(value) if value else float("nan") for key, value in row.items()}
            )

    titles = {"step": "Step and reversal", "ramp": "Ramp (slope 0.5/s)",
              "sine": "Sine (1 Hz)", "noisy_sine": "Sine + bounded sample noise"}
    fig, axes = plt.subplots(4, 2, figsize=(12, 11), sharex=True, constrained_layout=True)
    for index, (case, rows) in enumerate(data.items()):
        time = [row["time"] for row in rows]
        left, right = axes[index]
        left.plot(time, [row["input"] for row in rows], color="#8794a5",
                  linewidth=1, label="Input")
        left.plot(time, [row["value"] for row in rows], color="#176bc1",
                  linewidth=1.6, label="TD value x1")
        right.plot(time, [row["rate"] for row in rows], color="#d46623",
                   linewidth=1.5, label="TD rate x2")
        if case != "step":
            right.plot(time, [row["reference_rate"] for row in rows], "--",
                       color="#4c8b57", linewidth=1,
                       label="Clean-signal derivative" if case == "noisy_sine"
                       else "Input derivative")
        left.set_title(titles[case], loc="left")
        left.set_ylabel("Signal units")
        right.set_ylabel("Signal units / s")
        for ax in (left, right):
            ax.grid(alpha=0.2)
            ax.legend(loc="upper right", fontsize=8)
    for ax in axes[-1]:
        ax.set_xlabel("Time (s)")
    fig.suptitle("Linear TD | omega = 20 rad/s | dt = 1 ms | shared C++ core", fontsize=14)
    for extension in ("png", "svg"):
        path = output / f"td_response.{extension}"
        fig.savefig(path, dpi=160)
        print(f"Plot: {path}")
    plt.close(fig)


if __name__ == "__main__":
    main()
