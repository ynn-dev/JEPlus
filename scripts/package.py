# /// script
# requires-python = ">=3.9"
# ///

import subprocess
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

STREAMLINE_REPO = "NVIDIA-RTX/Streamline"
STREAMLINE_TAG = "v2.14.16"

STREAMLINE_FILES = [
    "bin/x64/sl.interposer.dll",
    "bin/x64/sl.common.dll",
    "bin/x64/sl.reflex.dll",
    "bin/x64/sl.pcl.dll",
    "bin/x64/sl.dlss_g.dll",
    
    "bin/x64/nvngx_dlss.dll",
    "bin/x64/nvngx_dlssg.dll",

    "bin/x64/reflex.license.txt",
    "bin/x64/nvngx_dlss.license.txt",

    "license.txt",
    "3rd-party-licenses.md",
]


def main() -> None:
    # build ASI
    subprocess.run(["uv", "run", "scripts/build.py", "Release"], cwd=ROOT, check=True)

    # download nvidia streamline sdk if it hasn't been cached
    sdk_name = f"streamline-sdk-{STREAMLINE_TAG}.zip"
    sdk_zip = ROOT / "build" / sdk_name
    if not sdk_zip.is_file():
        print(f"Downloading Streamline {STREAMLINE_TAG}")
        # Download under another name first, so an interrupted download is never mistaken for the cached zip
        partial = sdk_zip.with_suffix(".partial")
        urllib.request.urlretrieve(f"https://github.com/{STREAMLINE_REPO}/releases/download/{STREAMLINE_TAG}/{sdk_name}", partial)
        partial.replace(sdk_zip)

    # create final zip
    package = ROOT / "build" / "JEPlus.zip"
    with zipfile.ZipFile(package, "w", zipfile.ZIP_LZMA) as archive, zipfile.ZipFile(sdk_zip) as sdk:
        archive.write(ROOT / "build" / "Release" / "JEPlus.asi", "JEPlus/JEPlus.asi")
        archive.write(ROOT / "THIRD_PARTY_NOTICES.md", "JEPlus/THIRD_PARTY_NOTICES.md")
        for name in ["JEPlus.toml", "mod-meta.yaml", "mod-image.png"]:
            archive.write(ROOT / "assets" / name, f"JEPlus/{name}")
        for path in STREAMLINE_FILES:
            name = Path(path).name
            # make DLSS SR have its own folder so that it can be handled separately to allow for toggleable SR upgrade
            folder = "bin/dlss_sr" if name == "nvngx_dlss.dll" else "bin"
            archive.writestr(f"JEPlus/{folder}/{name}", sdk.read(path))

    print(f"Done. Packaged {package}")


if __name__ == "__main__":
    main()
