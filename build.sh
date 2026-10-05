#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"

mkdir -p out bin
CC=${CC:-gcc}

read -r -a includes <<< "$(pkg-config --cflags vulkan sdl3)"
read -r -a libraries <<< "$(pkg-config --libs vulkan sdl3)"

# compile atrac9 if needed
atrac9=(third_party/LibAtrac9/C/src/*.c)
if [[ ! -f out/libatrac9.a || -n $(find third_party/LibAtrac9/C/src -newer out/libatrac9.a -name '*.c' 2>/dev/null) ]]; then
    rm -rf out/atrac9 && mkdir -p out/atrac9
    for source in "${atrac9[@]}"; do
        "$CC" -std=c99 -O3 -march=native -mtune=native -g -w -c "$source" -o "out/atrac9/$(basename "${source%.c}").o"
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
"$CC" -std=c11 -O3 -march=native -mtune=native -g -Wall -Wextra -Werror -pthread -no-pie \
    "${includes[@]}" -I. -Isrc src/probe.c "${runtime[@]}" src/vulkan_smoke.c \
    out/libatrac9.a -lm "${gpu[@]}" "${libraries[@]}" -o out/ps4-runtime

# gpu capabilities check
"$CC" -std=c11 -O2 -Wall -Wextra -Werror tools/gpu_capabilities.c "${libraries[@]}" -o out/ps4-gpu-capabilities

echo "universal ps4 runtime build successful: out/ps4-runtime"
