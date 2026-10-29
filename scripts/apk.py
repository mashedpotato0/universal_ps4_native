#!/usr/bin/env python3
# build native android apk for ps4 games using aarch64 translator
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile
import xml.sax.saxutils
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCRIPTS = ROOT / "scripts"
OUT = ROOT / "out"

sys.path.insert(0, str(SCRIPTS))
try:
    import prepare
    import pkg
except ImportError:
    pass

try:
    from PIL import Image
except ImportError:
    Image = None


def find_tool(name: str) -> str:
    # search path and sdk directories
    cand = shutil.which(name)
    if cand:
        return cand

    search_roots = [
        Path.home() / "Unity/Hub/Editor/6000.6.5f1/Editor/Data/PlaybackEngines/AndroidPlayer/SDK",
        Path.home() / "Unity/Hub/Editor",
        Path.home() / "Android/Sdk",
        Path.home() / ".android-sdk",
        Path("/opt/android-sdk"),
    ]

    for root in search_roots:
        if not root.exists():
            continue
        matches = list(root.rglob(name))
        for m in sorted(matches, key=lambda p: str(p), reverse=True):
            if m.is_file():
                if name.endswith(".jar") or os.access(m, os.X_OK):
                    return str(m)

    return ""


def ensure_keystore(keystore_path: Path = None) -> tuple:
    # return or create debug keystore
    target = keystore_path or (Path.home() / ".android" / "debug.keystore")
    target.parent.mkdir(parents=True, exist_ok=True)

    ks_pass = "android"
    alias = "androiddebugkey"
    key_pass = "android"

    if not target.exists():
        keytool = shutil.which("keytool") or "/usr/sbin/keytool"
        if not Path(keytool).exists():
            raise RuntimeError("keytool not found cannot create android signing key")
        cmd = [
            keytool, "-genkeypair", "-v",
            "-keystore", str(target),
            "-storepass", ks_pass,
            "-alias", alias,
            "-keypass", key_pass,
            "-keyalg", "RSA",
            "-keysize", "2048",
            "-validity", "10000",
            "-dname", "CN=Android Debug,O=Android,C=US",
        ]
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    return target, ks_pass, alias, key_pass


def create_resized_icons(icon_source: Path, res_dir: Path):
    # generate android density launcher icons
    if not icon_source.exists() or Image is None:
        return

    density_sizes = {
        "mdpi": (48, 48),
        "hdpi": (72, 72),
        "xhdpi": (96, 96),
        "xxhdpi": (144, 144),
        "xxxhdpi": (192, 192),
    }

    try:
        with Image.open(icon_source) as im:
            rgba = im.convert("RGBA")
            for density, size in density_sizes.items():
                out_dir = res_dir / f"mipmap-{density}"
                out_dir.mkdir(parents=True, exist_ok=True)
                resized = rgba.resize(size, Image.Resampling.LANCZOS)
                resized.save(out_dir / "ic_launcher.png", "PNG")
    except Exception:
        pass


def ensure_translator_lib() -> Path:
    # compile android aarch64 translator shared library if missing
    lib_path = OUT / "android" / "arm64-v8a" / "libps4_translator.so"
    if lib_path.exists() and lib_path.stat().st_size > 10000:
        return lib_path

    build_script = ROOT / "build_android.sh"
    if not build_script.exists():
        raise FileNotFoundError(f"build_android.sh not found: {build_script}")

    print("building android arm64-v8a translator library...")
    subprocess.run(["bash", str(build_script)], check=True)
    if not lib_path.exists():
        raise RuntimeError(f"failed to generate {lib_path}")
    return lib_path


def slugify(text: str) -> str:
    # create package name component
    s = re.sub(r"[^\w]", "", text.lower())
    return s or "game"


def build_apk(game_dir: Path, out_apk: Path = None, package_name: str = None, app_name: str = None, keystore: Path = None) -> Path:
    # package ps4 game with native aarch64 translator into android apk
    game_dir = Path(game_dir).resolve()
    if not game_dir.exists():
        raise FileNotFoundError(f"game directory not found: {game_dir}")

    sfo_path = game_dir / "sce_sys" / "param.sfo"
    sfo = {}
    if sfo_path.exists() and "prepare" in sys.modules:
        try:
            sfo = prepare.sfo(sfo_path.read_bytes())
        except Exception:
            pass

    title = app_name or sfo.get("TITLE", game_dir.name)
    title_id = sfo.get("TITLE_ID", game_dir.name)
    app_ver = sfo.get("APP_VER", "01.00")

    # determine package name
    if not package_name:
        if "sekiro" in title.lower():
            pkg_name = "com.fromsoftware.sekiro"
        else:
            clean_id = slugify(title_id)
            pkg_name = f"com.universalps4.{clean_id}"
    else:
        pkg_name = package_name

    clean_slug = "".join(c if c.isalnum() else "_" for c in title.lower()).strip("_")
    dest_apk = Path(out_apk).resolve() if out_apk else (ROOT / f"{clean_slug}.apk")
    dest_apk.parent.mkdir(parents=True, exist_ok=True)

    print(f"building android apk for {title} ({title_id} v{app_ver})...")
    print(f"package: {pkg_name}")

    aapt_bin = find_tool("aapt")
    zipalign_bin = find_tool("zipalign")
    apksigner_bin = find_tool("apksigner")
    android_jar = find_tool("android.jar")

    if not aapt_bin:
        raise RuntimeError("aapt not found in android sdk build-tools")
    if not zipalign_bin:
        raise RuntimeError("zipalign not found in android sdk build-tools")
    if not apksigner_bin:
        raise RuntimeError("apksigner not found in android sdk build-tools")
    if not android_jar:
        raise RuntimeError("android.jar not found in android sdk platforms")

    # ensure translator shared library is built
    lib_path = ensure_translator_lib()

    with tempfile.TemporaryDirectory() as td:
        temp_dir = Path(td)
        res_dir = temp_dir / "res"
        values_dir = res_dir / "values"
        values_dir.mkdir(parents=True, exist_ok=True)

        # strings.xml
        escaped_title = xml.sax.saxutils.escape(title)
        strings_xml = values_dir / "strings.xml"
        strings_xml.write_text(
            f'<?xml version="1.0" encoding="utf-8"?>\n'
            f'<resources>\n'
            f'    <string name="app_name">{escaped_title}</string>\n'
            f'</resources>\n',
            encoding="utf-8"
        )

        # launcher icon
        icon_src = None
        for cand in [game_dir / "sce_sys" / "icon0.png", game_dir / "icon0.png"]:
            if cand.is_file():
                icon_src = cand
                break
        if icon_src:
            create_resized_icons(icon_src, res_dir)

        # androidmanifest.xml
        manifest_file = temp_dir / "AndroidManifest.xml"
        manifest_file.write_text(
            f'<?xml version="1.0" encoding="utf-8"?>\n'
            f'<manifest xmlns:android="http://schemas.android.com/apk/res/android"\n'
            f'    package="{pkg_name}"\n'
            f'    android:versionCode="1"\n'
            f'    android:versionName="{app_ver}">\n'
            f'    <uses-sdk android:minSdkVersion="24" android:targetSdkVersion="34" />\n'
            f'    <uses-feature android:glEsVersion="0x00030000" android:required="true" />\n'
            f'    <uses-permission android:name="android.permission.INTERNET" />\n'
            f'    <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />\n'
            f'    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />\n'
            f'    <application\n'
            f'        android:label="@string/app_name"\n'
            f'        android:icon="@mipmap/ic_launcher"\n'
            f'        android:hasCode="false"\n'
            f'        android:allowBackup="false"\n'
            f'        android:extractNativeLibs="true">\n'
            f'        <activity\n'
            f'            android:name="android.app.NativeActivity"\n'
            f'            android:label="@string/app_name"\n'
            f'            android:theme="@android:style/Theme.NoTitleBar.Fullscreen"\n'
            f'            android:screenOrientation="landscape"\n'
            f'            android:configChanges="orientation|keyboardHidden|screenSize"\n'
            f'            android:exported="true">\n'
            f'            <meta-data\n'
            f'                android:name="android.app.lib_name"\n'
            f'                android:value="ps4_translator" />\n'
            f'            <intent-filter>\n'
            f'                <action android:name="android.intent.action.MAIN" />\n'
            f'                <category android:name="android.intent.category.LAUNCHER" />\n'
            f'            </intent-filter>\n'
            f'        </activity>\n'
            f'    </application>\n'
            f'</manifest>\n',
            encoding="utf-8"
        )

        unaligned_apk = temp_dir / "unaligned.apk"
        aligned_apk = temp_dir / "aligned.apk"

        # package resources with aapt
        print("packaging base android container with aapt...")
        aapt_cmd = [
            aapt_bin, "package", "-f", "-m",
            "-M", str(manifest_file),
            "-S", str(res_dir),
            "-I", str(android_jar),
            "-F", str(unaligned_apk)
        ]
        subprocess.run(aapt_cmd, check=True, stdout=subprocess.DEVNULL)

        # inject native arm64 translator and game binary
        print("injecting aarch64 translator and ps4 game assets...")
        with zipfile.ZipFile(unaligned_apk, "a") as zout:
            # store shared library uncompressed for direct page mapping
            lib_bytes = lib_path.read_bytes()
            zout.writestr("lib/arm64-v8a/libps4_translator.so", lib_bytes, compress_type=zipfile.ZIP_STORED)

            # inject eboot.bin and metadata
            eboot_path = game_dir / "eboot.bin"
            if eboot_path.is_file():
                print(f"packaging eboot.bin ({eboot_path.stat().st_size / (1024*1024):.1f} MB)...")
                zout.writestr("assets/game/eboot.bin", eboot_path.read_bytes(), compress_type=zipfile.ZIP_DEFLATED)

            if sfo_path.is_file():
                zout.writestr("assets/game/param.sfo", sfo_path.read_bytes(), compress_type=zipfile.ZIP_DEFLATED)

            discmap = game_dir / "sce_discmap.plt"
            if discmap.is_file():
                zout.writestr("assets/game/sce_discmap.plt", discmap.read_bytes(), compress_type=zipfile.ZIP_DEFLATED)

            modules_dir = game_dir / "sce_module"
            if modules_dir.is_dir():
                for prx in sorted(modules_dir.glob("*.prx")):
                    zout.writestr(f"assets/game/sce_module/{prx.name}", prx.read_bytes(), compress_type=zipfile.ZIP_DEFLATED)

        # 4kb page align with zipalign
        print("aligning apk packages with zipalign...")
        subprocess.run([zipalign_bin, "-f", "-p", "4", str(unaligned_apk), str(aligned_apk)], check=True)

        # sign with android debug key
        print("signing apk with android debug key...")
        ks_file, ks_pass, ks_alias, key_pass = ensure_keystore(keystore)
        subprocess.run([
            apksigner_bin, "sign",
            "--ks", str(ks_file),
            "--ks-pass", f"pass:{ks_pass}",
            "--ks-key-alias", ks_alias,
            "--key-pass", f"pass:{key_pass}",
            str(aligned_apk),
        ], check=True)

        verify_res = subprocess.run([apksigner_bin, "verify", str(aligned_apk)], capture_output=True, text=True)
        if verify_res.returncode != 0:
            raise RuntimeError(f"apk signature verification failed: {verify_res.stderr}")

        shutil.copy2(aligned_apk, dest_apk)

    size_mb = dest_apk.stat().st_size / (1024 * 1024)
    print(f"\nandroid apk successfully built: {dest_apk} ({size_mb:.1f} MB)")
    print(f"package name: {pkg_name}")
    print(f"to install on your phone: adb install -r \"{dest_apk}\"")
    return dest_apk


def main():
    import argparse
    parser = argparse.ArgumentParser(description="build android apk for ps4 games using aarch64 translator")
    parser.add_argument("target", type=Path, help="game directory or .pkg file")
    parser.add_argument("--out", type=Path, default=None, help="output apk path")
    parser.add_argument("--app-name", type=str, default=None, help="app display name")
    parser.add_argument("--package-name", type=str, default=None, help="package name identifier")
    parser.add_argument("--keystore", type=Path, default=None, help="custom keystore path")
    args = parser.parse_args()

    build_apk(args.target, out_apk=args.out, package_name=args.package_name, app_name=args.app_name, keystore=args.keystore)


if __name__ == "__main__":
    main()
