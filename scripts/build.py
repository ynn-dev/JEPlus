# /// script
# requires-python = ">=3.9"
# ///

import argparse
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "build"


def main() -> None:
    parser = argparse.ArgumentParser(description="Configure and build JEPlus.")
    parser.add_argument("build_type", nargs="?", default="Release", choices=["Debug", "Release"])
    args = parser.parse_args()

    start = time.perf_counter()
    subprocess.run(["uvx", "cmake", "-S", str(ROOT), "-B", str(BUILD_DIR), "-A", "x64"], check=True)
    subprocess.run(["uvx", "cmake", "--build", str(BUILD_DIR), "--config", args.build_type], check=True)
    elapsed = time.perf_counter() - start

    print(f"Done. Built {args.build_type} in {elapsed:.2f}s")


if __name__ == "__main__":
    main()
