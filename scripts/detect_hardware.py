#!/usr/bin/env python3
# detect system hardware and compute optimal compiler and runtime flags
import argparse
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def get_cpu_info():
    # read cpu model cores and features
    model = "Unknown CPU"
    vendor = "Generic"
    flags = []
    try:
        with open("/proc/cpuinfo", "r") as f:
            for line in f:
                if "model name" in line and model == "Unknown CPU":
                    model = line.split(":", 1)[1].strip()
                elif "vendor_id" in line and vendor == "Generic":
                    v = line.split(":", 1)[1].strip()
                    vendor = "AMD" if "AMD" in v else ("Intel" if "Intel" in v else v)
                elif "flags" in line and not flags:
                    flags = line.split(":", 1)[1].strip().split()
    except Exception:
        pass

    threads = os.cpu_count() or 1
    features = []
    for feat in ["avx512f", "avx512vl", "avx2", "avx", "fma", "bmi2", "sse4_2", "popcnt", "aes"]:
        if feat in flags:
            features.append(feat.upper())

    return {
        "model": model,
        "vendor": vendor,
        "threads": threads,
        "flags": flags,
        "features": features,
    }


def get_ram_info():
    # read total system memory in gigabytes
    ram_gib = 8.0
    try:
        with open("/proc/meminfo", "r") as f:
            for line in f:
                if "MemTotal" in line:
                    ram_gib = int(line.split()[1]) / (1024 * 1024)
                    break
    except Exception:
        pass
    return ram_gib


def run_gpu_tool(gpu_bin, flag):
    # run capability tool and return stripped stdout or empty
    try:
        return subprocess.check_output([str(gpu_bin), flag], stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        return ""


def parse_id(text):
    # parse signed device index with -1 meaning none
    try:
        return int(text.splitlines()[-1])
    except (ValueError, IndexError):
        return -1


def get_gpu_info():
    # probe gpu using ps4-gpu-capabilities tool
    gpu_bin = ROOT / "out/ps4-gpu-capabilities"
    gpu_list = []
    discrete_id = -1
    integrated_id = -1
    live_res = 0
    gpu_type = "unknown"
    vram_mb = 0

    if gpu_bin.is_file() and os.access(gpu_bin, os.X_OK):
        for line in run_gpu_tool(gpu_bin, "--list").splitlines():
            if line.startswith("["):
                gpu_list.append(line.strip())
        discrete_id = parse_id(run_gpu_tool(gpu_bin, "--find-discrete"))
        integrated_id = parse_id(run_gpu_tool(gpu_bin, "--find-integrated"))
        live_out = run_gpu_tool(gpu_bin, "--live-resolution")
        live_res = 1 if live_out.splitlines()[-1:] == ["1"] else 0
        best = run_gpu_tool(gpu_bin, "--best-info").split()
        if len(best) == 2:
            gpu_type = best[0]
            vram_mb = int(best[1]) if best[1].isdigit() else 0

    return {
        "devices": gpu_list,
        "discrete_id": discrete_id,
        "integrated_id": integrated_id,
        "live_res": live_res,
        "type": gpu_type,
        "vram_mb": vram_mb,
    }


def is_low_spec(gpu, ram_gib):
    # integrated or software gpus, small vram or little system memory
    if gpu["type"] in ("integrated", "cpu", "other"):
        return True
    if gpu["type"] == "discrete" and 0 < gpu["vram_mb"] < 4096:
        return True
    return ram_gib < 12.0


def compute_compiler_flags(cpu, ram_gib, cc="gcc"):
    # test compiler support for host native flags
    cflags = ["-O3", "-fno-omit-frame-pointer"]
    arch_flags = "-march=native -mtune=native"

    # verify march=native with active compiler
    try:
        res = subprocess.run([cc, "-march=native", "-E", "-"], input=b"", stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if res.returncode != 0:
            if "AVX2" in cpu["features"]:
                arch_flags = "-march=x86-64-v3 -mtune=generic"
            elif "SSE4_2" in cpu["features"]:
                arch_flags = "-march=x86-64-v2 -mtune=generic"
            else:
                arch_flags = "-march=x86-64"
    except Exception:
        arch_flags = "-march=x86-64-v3"

    cflags.append(arch_flags)
    cflags.extend(["-ftree-vectorize", "-fno-math-errno", "-fno-trapping-math"])

    # determine parallel compile jobs based on ram and cpu cores
    max_jobs_by_ram = max(1, int(ram_gib / 1.5))
    jobs = min(cpu["threads"], max_jobs_by_ram)

    # determine lto suitability based on ram
    lto_default = "ON" if ram_gib >= 6.0 else "OFF"

    return {
        "cflags": " ".join(cflags),
        "arch_flags": arch_flags,
        "jobs": jobs,
        "lto": lto_default,
    }


def main():
    parser = argparse.ArgumentParser(description="Detect system hardware and compute compiler flags")
    parser.add_argument("--shell", action="store_true", help="output shell variable assignments")
    parser.add_argument("--quiet", action="store_true", help="suppress human readable output")
    parser.add_argument("--save-env", type=Path, default=None, help="save environment variables to file")
    parser.add_argument("--cc", default=os.getenv("CC", "gcc"), help="c compiler to test")
    args = parser.parse_args()

    cpu = get_cpu_info()
    ram = get_ram_info()
    gpu = get_gpu_info()
    flags = compute_compiler_flags(cpu, ram, cc=args.cc)

    primary_gpu = gpu["devices"][0] if gpu["devices"] else "Vulkan Device (auto)"
    low_spec = is_low_spec(gpu, ram)

    lines = [
        f"HW_CPU_MODEL='{cpu['model']}'",
        f"HW_CPU_VENDOR='{cpu['vendor']}'",
        f"HW_CPU_THREADS={cpu['threads']}",
        f"HW_CPU_FEATURES='{' '.join(cpu['features'])}'",
        f"HW_RAM_GIB={ram:.1f}",
        f"HW_BUILD_JOBS={flags['jobs']}",
        f"HW_LTO_DEFAULT='{flags['lto']}'",
        f"HW_ARCH_FLAGS='{flags['arch_flags']}'",
        f"HW_CFLAGS='{flags['cflags']}'",
        f"HW_PRIMARY_GPU='{primary_gpu}'",
        f"HW_DISCRETE_GPU_ID={gpu['discrete_id']}",
        f"HW_INTEGRATED_GPU_ID={gpu['integrated_id']}",
        f"HW_LIVE_RESOLUTION={gpu['live_res']}",
        f"HW_GPU_TYPE={gpu['type']}",
        f"HW_VRAM_MB={gpu['vram_mb']}",
        f"HW_LOW_SPEC={1 if low_spec else 0}",
    ]

    if args.save_env:
        args.save_env.parent.mkdir(parents=True, exist_ok=True)
        args.save_env.write_text("\n".join(lines) + "\n")

    if args.shell:
        for l in lines:
            print(l)
    elif not args.quiet:
        print("=" * 60)
        print("system hardware detection & compiler optimization profile")
        print("=" * 60)
        print(f"cpu:        {cpu['model']} ({cpu['threads']} threads) [{', '.join(cpu['features'])}]")
        print(f"ram:        {ram:.1f} GiB (allocated parallel jobs: {flags['jobs']})")
        print(f"gpu:        {primary_gpu}")
        print(f"gpu class:  {gpu['type']} ({gpu['vram_mb']} MiB local), low spec profile {'on' if low_spec else 'off'}")
        print(f"compiler:   {args.cc}")
        print(f"arch:       {flags['arch_flags']}")
        print(f"cflags:     {flags['cflags']}")
        print(f"lto:        {flags['lto']}")
        print("=" * 60)


if __name__ == "__main__":
    main()
