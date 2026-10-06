#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"

mkdir -p out bin
if [[ ! -f bbport.ini && -f bbport.ini.example ]]; then
    cp bbport.ini.example bbport.ini
fi
CC=${CC:-gcc}
CXX=${CXX:-g++}

# check build tools
if ! command -v pkg-config >/dev/null 2>&1 || ! pkg-config --exists vulkan sdl3; then
    echo "error: vulkan and sdl3 development libraries are required" >&2
    exit 1
fi
if ! command -v cmake >/dev/null 2>&1 || ! command -v ninja >/dev/null 2>&1; then
    echo "error: cmake and ninja are required" >&2
    exit 1
fi

read -r -a includes <<< "$(pkg-config --cflags vulkan sdl3)"
read -r -a libraries <<< "$(pkg-config --libs vulkan sdl3)"

# build gpu capabilities tool early for hardware detection
if [[ ! -x out/ps4-gpu-capabilities || tools/gpu_capabilities.c -nt out/ps4-gpu-capabilities ]]; then
    "$CC" -std=c11 -O2 -Wall -Wextra tools/gpu_capabilities.c "${libraries[@]}" -o out/ps4-gpu-capabilities 2>/dev/null || true
fi

# detect system hardware and compute optimal compiler flags
if [[ -f scripts/detect_hardware.py ]]; then
    python3 scripts/detect_hardware.py --save-env out/hardware_profile.env
    if [[ -f out/hardware_profile.env ]]; then
        # shellcheck disable=SC1091
        source out/hardware_profile.env
    fi
fi

# host cpu flags for intel and amd
ARCH_FLAGS="${HW_ARCH_FLAGS:-"-march=native -mtune=native"}"
if ! "$CC" $ARCH_FLAGS -E - </dev/null >/dev/null 2>&1; then
    ARCH_FLAGS="-march=x86-64-v3 -mtune=generic"
fi
if [[ -n "${BB_ARCH:-}" ]]; then
    ARCH_FLAGS="-march=$BB_ARCH"
fi

BUILD_JOBS="${HW_BUILD_JOBS:-$(nproc 2>/dev/null || echo 4)}"

# update submodules and patches if needed
if [[ -f .gitmodules && (! -f gpu/third_party/imgui/imgui.h || ! -f gpu/third_party/fsr-vulkan/CMakeLists.txt) ]]; then
    git submodule update --init --recursive
fi
if [[ -d gpu/patches/fsr-vulkan && -d gpu/third_party/fsr-vulkan ]]; then
    for patch in gpu/patches/fsr-vulkan/*.patch; do
        if [[ -f "$patch" ]] && ! git -C gpu/third_party/fsr-vulkan apply --reverse --check "$PWD/$patch" 2>/dev/null; then
            git -C gpu/third_party/fsr-vulkan apply "$PWD/$patch" 2>/dev/null || true
        fi
    done
fi

# build gpu library with lto by default
gpu_so="out/gpu/libbbgpu.so"
needs_gpu_build=0
if [[ ! -f "$gpu_so" ]]; then
    needs_gpu_build=1
elif [[ -n $(find gpu/shadps4 gpu/shim gpu/CMakeLists.txt \( -name '*.cpp' -o -name '*.h' -o -name 'CMakeLists.txt' \) -newer "$gpu_so" 2>/dev/null | head -1) ]]; then
    needs_gpu_build=1
fi

if [[ $needs_gpu_build -eq 1 ]]; then
    echo "compiling ps4 gpu library with lto..."
    cmake_args=()
    if [[ -d "$HOME/.local" ]]; then
        cmake_args+=("-DCMAKE_PREFIX_PATH=$HOME/.local")
    fi
    lto_opt="${BB_LTO:-${HW_LTO_DEFAULT:-ON}}"
    mkdir -p out/gpu
    if cmake -S gpu -B out/gpu -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBB_ARCH="$ARCH_FLAGS" -DBB_LTO="$lto_opt" "${cmake_args[@]}" >/dev/null 2>&1 && \
       ninja -C out/gpu -j"$BUILD_JOBS" bbgpu > out/gpu-build.log 2>&1; then
        echo "ps4 gpu library built with lto $lto_opt"
    else
        echo "lto build unavailable; falling back to standard build..."
        cmake -S gpu -B out/gpu -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBB_ARCH="$ARCH_FLAGS" -DBB_LTO=OFF "${cmake_args[@]}" >/dev/null 2>&1
        ninja -C out/gpu -j"$BUILD_JOBS" bbgpu > out/gpu-build.log 2>&1
        echo "ps4 gpu library built with lto off"
    fi
fi

# compile atrac9 if needed
atrac9=(third_party/LibAtrac9/C/src/*.c)
if [[ ! -f out/libatrac9.a || -n $(find third_party/LibAtrac9/C/src -newer out/libatrac9.a -name '*.c' 2>/dev/null) ]]; then
    rm -rf out/atrac9 && mkdir -p out/atrac9
    for source in "${atrac9[@]}"; do
        "$CC" -std=c99 -O3 $ARCH_FLAGS -g -w -c "$source" -o "out/atrac9/$(basename "${source%.c}").o"
    done
    ar rcs out/libatrac9.a out/atrac9/*.o
fi

gpu=(-Lout/gpu -lbbgpu -Wl,-rpath,'$ORIGIN/gpu' -Wl,-rpath,"$PWD/out/gpu" -rdynamic)
if [[ -d "$HOME/.local/lib" ]]; then
    gpu+=(-L"$HOME/.local/lib" -Wl,-rpath,"$HOME/.local/lib")
fi

runtime=(src/runtime*.c)

# compile ps4 native runtime executable
echo "compiling universal ps4 native runtime..."
"$CC" -std=c11 ${HW_CFLAGS:-"-O3 $ARCH_FLAGS -ftree-vectorize"} -g -Wall -Wextra -Werror -pthread -no-pie \
    "${includes[@]}" -I. -Isrc src/probe.c "${runtime[@]}" src/vulkan_smoke.c \
    out/libatrac9.a -lm "${gpu[@]}" "${libraries[@]}" -o out/ps4-runtime

# gpu capabilities check
"$CC" -std=c11 -O2 -Wall -Wextra -Werror tools/gpu_capabilities.c "${libraries[@]}" -o out/ps4-gpu-capabilities

echo "universal ps4 runtime build successful: out/ps4-runtime"
