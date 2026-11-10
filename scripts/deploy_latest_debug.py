# /// script
# requires-python = ">=3.9"
# ///

import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ASI = ROOT / "build" / "Debug" / "JEPlus.asi"
DEPLOY_DIR = Path(r"D:\SteamLibrary\steamapps\common\Judgment\runtime\media\mods\JEPlus")


def main() -> None:
    subprocess.run(["uv", "run", "scripts/format.py"], cwd=ROOT, check=True)
    subprocess.run(["uv", "run", "scripts/build.py", "Debug"], cwd=ROOT, check=True)

    DEPLOY_DIR.mkdir(parents=True, exist_ok=True)
    target = DEPLOY_DIR / ASI.name
    shutil.copy2(ASI, target)

    print(f"Done. Deployed {ASI.name} to {target}")


if __name__ == "__main__":
    main()
