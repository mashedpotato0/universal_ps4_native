#!/usr/bin/env bash
# cross compile translator for android arm64
set -euo pipefail
cd -- "$(dirname -- "$0")"

# find android ndk
NDK=""
SEARCH_DIRS=(
    "${ANDROID_NDK_ROOT:-}"
    "${ANDROID_NDK:-}"
    "${NDK_ROOT:-}"
    "$HOME/Unity/Hub/Editor/6000.6.5f1/Editor/Data/PlaybackEngines/AndroidPlayer/NDK"
    "$HOME/Android/Sdk/ndk"/*
    "$HOME/.android-sdk/ndk"/*
    "/opt/android-ndk"
)

for cand in "${SEARCH_DIRS[@]}"; do
    if [[ -n "$cand" && -d "$cand" && -f "$cand/ndk-build" ]]; then
        NDK="$cand"
        break
    fi
done

if [[ -z "$NDK" ]]; then
    echo "error: android ndk not found" >&2
    exit 1
fi

echo "using android ndk: $NDK"

LLVM_BIN="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin"
if [[ ! -d "$LLVM_BIN" ]]; then
    echo "error: llvm prebuilt toolchain missing in ndk" >&2
    exit 1
fi

# find highest available aarch64 clang
API_TARGET=""
for api in 35 34 33 32 31 30 29 28 27 26 24 21; do
    if [[ -x "$LLVM_BIN/aarch64-linux-android$api-clang" ]]; then
        API_TARGET="$api"
        break
    fi
done

if [[ -z "$API_TARGET" ]]; then
    echo "error: aarch64 clang not found in ndk" >&2
    exit 1
fi

CC="$LLVM_BIN/aarch64-linux-android$API_TARGET-clang"
AR="$LLVM_BIN/llvm-ar"
STRIP="$LLVM_BIN/llvm-strip"

echo "target compiler: $CC (api $API_TARGET)"

OUT_DIR="out/android/arm64-v8a"
mkdir -p "$OUT_DIR"

CFLAGS="-std=c11 -O3 -Wall -Wextra -Werror -fPIC -Isrc -I."

# compile translator objects
echo "building translator for android arm64-v8a..."
"$CC" $CFLAGS -c src/runtime_translator.c -o "$OUT_DIR/runtime_translator.o"

# compile runtime shims for android
RUNTIME_SOURCES=(
    src/runtime_memory.c
    src/runtime_time.c
    src/runtime_thread.c
    src/runtime_mutex.c
    src/runtime_rwlock.c
    src/runtime_sema.c
    src/runtime_file.c
    src/runtime_kernel.c
    src/runtime_pad.c
    src/runtime_audio.c
    src/runtime_ajm.c
    src/runtime_savedata.c
    src/runtime_content.c
    src/runtime_rtc.c
    src/runtime_services.c
)

RUNTIME_OBJS=()
for src in "${RUNTIME_SOURCES[@]}"; do
    obj="$OUT_DIR/$(basename "${src%.c}").o"
    "$CC" $CFLAGS -c "$src" -o "$obj"
    RUNTIME_OBJS+=("$obj")
done

# build shared library
echo "linking libps4_translator.so..."
"$CC" -shared -fPIC -Wl,-soname,libps4_translator.so \
    "$OUT_DIR/runtime_translator.o" "${RUNTIME_OBJS[@]}" \
    -lm -pthread -ldl -o "$OUT_DIR/libps4_translator.so"

# build standalone executable
echo "linking ps4-translator runner..."
"$CC" $CFLAGS src/translator_main.c "$OUT_DIR/runtime_translator.o" \
    -lm -pthread -o "$OUT_DIR/ps4-translator"

# build test runner
echo "linking translator-test..."
"$CC" $CFLAGS tests/test_translator.c "$OUT_DIR/runtime_translator.o" \
    -lm -pthread -o "$OUT_DIR/translator-test"

echo ""
echo "android arm64-v8a build complete:"
echo "  shared library: $OUT_DIR/libps4_translator.so"
echo "  cli runner:     $OUT_DIR/ps4-translator"
echo "  unit test:      $OUT_DIR/translator-test"
echo ""
echo "to run test on an android device:"
echo "  adb push $OUT_DIR/translator-test /data/local/tmp/"
echo "  adb shell chmod 755 /data/local/tmp/translator-test"
echo "  adb shell /data/local/tmp/translator-test"
