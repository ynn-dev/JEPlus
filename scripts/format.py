# /// script
# requires-python = ">=3.9"
# ///

import subprocess
import time
from pathlib import Path

PATH = Path(__file__).resolve().parent.parent / "src"

def main() -> None:
    print("Formatting...")

    files = sorted([*PATH.rglob("*.cpp"), *PATH.rglob("*.h")])

    if not files:
        print(f"No files found in '{PATH}'")
        return

    start = time.perf_counter()
    subprocess.run(["uvx", "clang-format", "-i", *map(str, files)], check=True)
    elapsed = time.perf_counter() - start

    for file in files:
        print(file.relative_to(PATH))
        
    print(f"Done. Formatted {len(files)} file(s) in {elapsed:.2f}s")


if __name__ == "__main__":
    main()
