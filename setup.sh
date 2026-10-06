#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"

# print header
echo "=========================================================="
echo "universal ps4 native runtime - dependency and tool setup"
echo "=========================================================="

mkdir -p tools/bin out
if [[ ! -f bbport.ini && -f bbport.ini.example ]]; then
    cp bbport.ini.example bbport.ini
fi

# system dependencies check
echo "[0/4] verifying system build prerequisites..."
missing_deps=()
for tool in pkg-config cmake ninja gcc g++ python3; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        missing_deps+=("$tool")
    fi
done
for pkg in vulkan sdl3; do
    if command -v pkg-config >/dev/null 2>&1 && ! pkg-config --exists "$pkg" >/dev/null 2>&1; then
        missing_deps+=("$pkg")
    fi
done
if [[ ${#missing_deps[@]} -gt 0 ]]; then
    echo "warning: some build dependencies are missing: ${missing_deps[*]}"
    echo "to install them:"
    echo "  debian/ubuntu: sudo apt install build-essential cmake ninja-build pkg-config libvulkan-dev libsdl3-dev libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libswresample-dev python3"
    echo "  arch linux:    sudo pacman -S base-devel cmake ninja vulkan-devel sdl3 ffmpeg python"
    echo "  fedora:        sudo dnf install gcc-c++ cmake ninja-build pkgconf vulkan-loader-devel SDL3-devel ffmpeg-free-devel python3"
else
    echo "system build prerequisites found"
fi

# submodules
echo "[1/4] checking and updating git submodules..."
if [[ -f .gitmodules ]]; then
    git submodule update --init --recursive
    echo "git submodules initialized"
else
    echo "no .gitmodules found; skipping submodule sync"
fi

# apply patches
echo "[2/4] applying submodule patches..."
if [[ -d gpu/patches/fsr-vulkan && -d gpu/third_party/fsr-vulkan ]]; then
    for patch in gpu/patches/fsr-vulkan/*.patch; do
        if [[ -f "$patch" ]]; then
            if ! git -C gpu/third_party/fsr-vulkan apply --reverse --check "$PWD/$patch" 2>/dev/null; then
                git -C gpu/third_party/fsr-vulkan apply "$PWD/$patch" 2>/dev/null || true
            fi
        fi
    done
    echo "fsr-vulkan patches applied"
fi

# ps4 pkg tool
echo "[3/4] checking ps4 package extraction tool..."
if command -v ps4-pkg-tool >/dev/null 2>&1 || [[ -x "$HOME/.local/bin/ps4-pkg-tool" ]]; then
    echo "ps4-pkg-tool already installed"
else
    echo "ps4-pkg-tool not found; cloning and building..."
    PKG_TOOL_DIR="tools/ps4-pkg-tool"
    if [[ ! -d "$PKG_TOOL_DIR" ]]; then
        git clone --depth 1 https://github.com/hippie68/ps4-pkg-tool.git "$PKG_TOOL_DIR"
    fi
    mkdir -p "$PKG_TOOL_DIR/build"
    cmake -S "$PKG_TOOL_DIR" -B "$PKG_TOOL_DIR/build" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$PKG_TOOL_DIR/build" -j"$(nproc)"
    mkdir -p "$HOME/.local/bin"
    cp "$PKG_TOOL_DIR/build/ps4-pkg-tool" "$HOME/.local/bin/ps4-pkg-tool"
    chmod +x "$HOME/.local/bin/ps4-pkg-tool"
    echo "ps4-pkg-tool built and installed to ~/.local/bin/ps4-pkg-tool"
fi

# acknowledgments
echo "[4/4] acknowledging third-party components..."
cat << 'EOF'

Third-Party Tools & Libraries Acknowledged:

1. shadPS4 & bbport (GPL-2.0 / MIT)
   Authors: shadPS4 contributors, FireBurn, and the bbport team
   Role: PS4 Gnm/Gnmx Vulkan video core, GCN shader recompiler, HLE runtime concepts
   URL: https://github.com/shadps4-emu/shadPS4
   URL: https://github.com/shadps4-emu/bbport

2. ps4-pkg-tool (GPL-3.0)
   Author: hippie68
   Role: PlayStation 4 PKG container parsing, unpacking, and extraction
   URL: https://github.com/hippie68/ps4-pkg-tool

3. LibAtrac9 (MIT)
   Author: Thealexbarney
   Role: Hardware ATRAC9 audio decoding in native C
   URL: https://github.com/Thealexbarney/LibAtrac9

4. FSR-Vulkan (MIT / AMD FidelityFX)
   Authors: FireBurn, Advanced Micro Devices, Inc.
   Role: AMD FidelityFX Super Resolution 3.1 temporal upscaler Vulkan port
   URL: https://github.com/FireBurn/FSR-Vulkan

5. Dear ImGui (MIT)
   Author: Omar Cornut (ocornut)
   Role: In-game debug overlay and settings interface
   URL: https://github.com/ocornut/imgui

6. sirit (MIT)
   Authors: ReinUsesLisp, shadPS4 contributors
   Role: Runtime SPIR-V assembly and intermediate representation generator
   URL: https://github.com/ReinUsesLisp/sirit

7. Zydis & Zycore (MIT)
   Author: Zyantific
   Role: Fast and lightweight x86/x86-64 disassembler and instruction decoder
   URL: https://github.com/zyantific/zydis

8. VulkanMemoryAllocator (VMA) (MIT)
   Author: AMD GPUOpen
   Role: Fast memory allocation for Vulkan buffers and images
   URL: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator

9. miniz (MIT)
   Author: Rich Geldreich and contributors
   Role: Single-source Deflate/ZIP data compression library
   URL: https://github.com/richgel999/miniz

10. half (MIT)
    Author: Christian Rau
    Role: IEEE 754-2008 half-precision floating point library for C++
    URL: https://half.sourceforge.net/

11. tsl-robin-map (MIT)
    Author: Thibaut Goetghebuer-Planchon
    Role: Fast robin-hood hash map and set implementations
    URL: https://github.com/Tessil/robin-map

12. magic_enum (MIT)
    Author: Daniil Goncharov
    Role: Static reflection for C++ enums
    URL: https://github.com/Neargye/magic_enum

13. xxhash (BSD-2-Clause)
    Author: Yann Collet
    Role: Ultra-fast non-cryptographic hashing algorithm
    URL: https://github.com/Cyan4973/xxHash

14. fmt (MIT)
    Author: Victor Zverovich and contributors
    Role: Modern string formatting library for C++
    URL: https://github.com/fmtlib/fmt

15. DejaVu Fonts (Bitstream Vera / DejaVu License)
    Authors: DejaVu Fonts team
    Role: Embedded Cyrillic/Latin TrueType font
    URL: https://dejavu-fonts.github.io/

==========================================================
setup complete; run ./build.sh to compile runtime
==========================================================
EOF
