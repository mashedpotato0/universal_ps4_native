#!/usr/bin/env python3
# ps4 package parser and extractor utilities
import os
import shutil
import struct
import subprocess
import sys
from pathlib import Path

# import prepare for sfo parsing
try:
    import prepare
except ImportError:
    scripts_dir = Path(__file__).resolve().parent
    sys.path.insert(0, str(scripts_dir))
    import prepare


def is_pkg_file(path: Path) -> bool:
    # check if file is a ps4 pkg
    if not path.is_file():
        return False
    if path.suffix.lower() == ".pkg":
        return True
    try:
        with open(path, "rb") as f:
            return f.read(4) == b"\x7fCNT"
    except Exception:
        return False


def read_pkg_sfo(pkg_path: Path) -> dict:
    # read param sfo directly from pkg table
    try:
        with open(pkg_path, "rb") as f:
            h = f.read(0x100)
            if len(h) < 0x20 or h[:4] != b"\x7fCNT":
                return {}
            entry_count = struct.unpack(">I", h[0x10:0x14])[0]
            table_offset = struct.unpack(">I", h[0x18:0x1c])[0]
            f.seek(table_offset)
            table = f.read(entry_count * 32)
            for i in range(entry_count):
                eid = struct.unpack(">I", table[i * 32 : i * 32 + 4])[0]
                if eid == 0x1000:
                    off, sz = struct.unpack(">II", table[i * 32 + 16 : i * 32 + 24])
                    f.seek(off)
                    return prepare.sfo(f.read(sz))
    except Exception:
        return {}
    return {}


def get_pkg_info(pkg_path: Path) -> dict:
    # extract title and category metadata
    sfo = read_pkg_sfo(pkg_path)
    category = sfo.get("CATEGORY", "").strip().lower()
    title_id = sfo.get("TITLE_ID", "").strip()
    title = sfo.get("TITLE", "").strip()
    app_ver = sfo.get("APP_VER", "01.00").strip()

    # categorize package role
    if category in ("gd", "gda", "gp") and "patch" not in pkg_path.name.lower() and "update" not in pkg_path.name.lower() and category != "gp":
        cat_type = "base"
        cat_label = "Base Game"
    elif category == "ac":
        cat_type = "dlc"
        cat_label = "Add-on / DLC"
    elif category == "gp" or "patch" in pkg_path.name.lower() or "update" in pkg_path.name.lower():
        cat_type = "patch"
        cat_label = "Update / Patch"
    else:
        cat_type = "base"
        cat_label = "Package"

    return {
        "path": pkg_path,
        "name": pkg_path.name,
        "size": pkg_path.stat().st_size,
        "title": title or "Unknown Title",
        "title_id": title_id or "Unknown",
        "category": category,
        "type": cat_type,
        "label": cat_label,
        "app_ver": app_ver,
        "sfo": sfo,
    }


def scan_pkgs(target_paths) -> list:
    # collect all valid pkg files from files or folders
    if isinstance(target_paths, (str, Path)):
        target_paths = [Path(target_paths)]

    discovered = []
    seen = set()

    for p in target_paths:
        path = Path(p)
        if not path.exists():
            continue
        if path.is_file():
            if is_pkg_file(path) and path.resolve() not in seen:
                seen.add(path.resolve())
                discovered.append(get_pkg_info(path))
        elif path.is_dir():
            # scan folder for pkg files only
            for item in sorted(path.iterdir()):
                if is_pkg_file(item) and item.resolve() not in seen:
                    seen.add(item.resolve())
                    discovered.append(get_pkg_info(item))

    return discovered


def parse_ver_tuple(ver_str: str) -> tuple:
    # parse version string to numeric tuple
    parts = []
    for chunk in ver_str.split("."):
        try:
            parts.append(int(chunk))
        except ValueError:
            parts.append(0)
    return tuple(parts)


def sort_pkgs_for_extraction(pkgs: list) -> list:
    # sort base game first then dlc then patches in version order
    type_priority = {
        "base": 0,
        "dlc": 1,
        "patch": 2,
    }

    def sort_key(item):
        prio = type_priority.get(item["type"], 9)
        ver = parse_ver_tuple(item.get("app_ver", "01.00"))
        # larger file size first for base game
        size_neg = -item.get("size", 0) if prio == 0 else item.get("size", 0)
        return (prio, ver, size_neg)

    return sorted(pkgs, key=sort_key)


def find_pkg_tool() -> str:
    # locate ps4 pkg tool executable
    tool = shutil.which("ps4-pkg-tool") or str(Path.home() / ".local/bin/ps4-pkg-tool")
    if not Path(tool).exists():
        raise RuntimeError("ps4-pkg-tool not found in PATH or ~/.local/bin/ps4-pkg-tool")
    return str(tool)


def extract_pkgs(pkgs: list, dest_dir: Path, tool_path: str = None) -> Path:
    # extract sequence of pkgs in dependency order
    if not pkgs:
        raise ValueError("no packages provided for extraction")

    tool = tool_path or find_pkg_tool()
    dest_dir = Path(dest_dir)
    dest_dir.mkdir(parents=True, exist_ok=True)

    sorted_list = sort_pkgs_for_extraction(pkgs)
    total = len(sorted_list)

    print(f"preparing to extract {total} package(s) into {dest_dir}...")
    for idx, item in enumerate(sorted_list, 1):
        pkg_file = item["path"]
        label = item["label"]
        ver = item.get("app_ver", "")
        size_mb = item["size"] / (1024 * 1024)
        print(f"[{idx}/{total}] extracting {label}: {pkg_file.name} (v{ver}, {size_mb:.1f} MB)...")
        res = subprocess.run([tool, str(pkg_file), str(dest_dir)])
        if res.returncode != 0:
            raise RuntimeError(f"failed to extract {pkg_file} (exit code {res.returncode})")

    print(f"all packages successfully extracted into {dest_dir}")
    return dest_dir
