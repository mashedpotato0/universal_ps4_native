#!/usr/bin/env python3
# standalone windows release packager
import os
import shutil
import subprocess
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT_RELEASE = ROOT / "out" / "release"
PACKAGE_NAME = "universal_ps4_native-windows-x64"
ZIP_PATH = OUT_RELEASE / f"{PACKAGE_NAME}.zip"


def main():
    # ensure output release directory
    OUT_RELEASE.mkdir(parents=True, exist_ok=True)
    stage_dir = OUT_RELEASE / PACKAGE_NAME
    if stage_dir.exists():
        shutil.rmtree(stage_dir)
    stage_dir.mkdir(parents=True, exist_ok=True)

    print("packaging standalone windows release...")

    # root launcher executable and scripts
    shutil.copy2(ROOT / "play.exe", stage_dir / "play.exe")
    shutil.copy2(ROOT / "run.bat", stage_dir / "run.bat")
    shutil.copy2(ROOT / "setup.bat", stage_dir / "setup.bat")
    if (ROOT / "play").exists():
        shutil.copy2(ROOT / "play", stage_dir / "play")
    if (ROOT / "run.sh").exists():
        shutil.copy2(ROOT / "run.sh", stage_dir / "run.sh")
    if (ROOT / "bbport.ini.example").exists():
        shutil.copy2(ROOT / "bbport.ini.example", stage_dir / "bbport.ini.example")
        shutil.copy2(ROOT / "bbport.ini.example", stage_dir / "bbport.ini")

    # documentation and licensing
    for doc in ["README.md", "LICENSE", "ACKNOWLEDGMENTS.md"]:
        if (ROOT / doc).exists():
            shutil.copy2(ROOT / doc, stage_dir / doc)

    # directories to bundle
    dirs_to_copy = [
        ("bin/windows", "bin/windows"),
        ("patches", "patches"),
        ("data", "data"),
        ("scripts", "scripts"),
    ]

    for src_rel, dst_rel in dirs_to_copy:
        src = ROOT / src_rel
        dst = stage_dir / dst_rel
        if src.exists():
            shutil.copytree(src, dst, dirs_exist_ok=True)

    # clean pycache
    for pycache in stage_dir.rglob("__pycache__"):
        shutil.rmtree(pycache)
    for pyc in stage_dir.rglob("*.pyc"):
        pyc.unlink()

    # create zip archive
    if ZIP_PATH.exists():
        ZIP_PATH.unlink()

    print(f"creating archive {ZIP_PATH}...")
    with zipfile.ZipFile(ZIP_PATH, "w", zipfile.ZIP_DEFLATED) as zipf:
        for file in stage_dir.rglob("*"):
            if file.is_file():
                arcname = file.relative_to(stage_dir)
                zipf.write(file, arcname)

    size_mb = ZIP_PATH.stat().st_size / (1024 * 1024)
    print(f"release package created: {ZIP_PATH} ({size_mb:.2f} MB)")


if __name__ == "__main__":
    main()
