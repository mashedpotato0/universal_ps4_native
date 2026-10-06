#!/usr/bin/env bash
# native ps4 launcher wrapper
set -euo pipefail
HERE="$(cd -- "$(dirname -- "$0")" && pwd)"

if [[ -n "${BB_PREBUILT:-}" || -n "${BB_PROBE:-}" ]]; then
    # test mode for legacy test harness
    data=${BB_DATA_DIR:-.}
    out=$data/out
    mkdir -p "$out"
    PYTHON=${PYTHON:-$(command -v python3)}
    game=${BB_GAME_DIR:-../CUSA03173}
    original_game=$game
    mod_game=""
    if [[ -f scripts/mods.py ]]; then
        game=$("$PYTHON" scripts/mods.py "$game" --out "$out" \
            --mods-dir "${BB_MODS_DIR:-$data/mods}" --config "${BB_MODS_CONFIG:-$data/mods.json}" \
            --enabled "${BB_MODS_ENABLED:-1}")
        if [[ $game != "$(realpath "$original_game")" ]]; then
            mod_game=$game
            trap '"$PYTHON" -c '\''import shutil,sys; shutil.rmtree(sys.argv[1])'\'' "$mod_game"' EXIT
        fi
    fi
    export BB_CONFIG=${BB_CONFIG:-$data/bbport.ini}
    if [[ ${BB_AUTO_RENDER_RES:-} == 1 ]]; then
        unset BB_RENDER_RES BB_OUTPUT_RES BB_AUTO_RENDER_RES
    fi
    if [[ -z ${BB_RENDER_RES:-} && -f scripts/patches.py ]]; then
        read -r scaled_render scaled_output < <("$PYTHON" scripts/patches.py --print-scaled --settings "$BB_CONFIG") || true
    fi
    live=0
    if [[ -n ${scaled_output:-} ]]; then
        live=${BB_LIVE_RES:-}
        if [[ -z $live && -f $BB_CONFIG ]]; then
            while IFS= read -r line || [[ -n $line ]]; do
                [[ $line =~ ^live_resolution=([01]|auto)$ ]] && live=${BASH_REMATCH[1]}
            done < "$BB_CONFIG"
        fi
        if [[ $live == auto ]]; then
            if [[ -n ${BB_PROBE:-} ]]; then caps=$(dirname -- "$BB_PROBE")/bb-gpu-capabilities
            elif [[ -n ${BB_PREBUILT:-} ]]; then caps=bin/bb-gpu-capabilities
            else caps=out/bb-gpu-capabilities; fi
            live=$("$caps" --live-resolution 2>/dev/null) || live=0
        fi
        [[ $live == 1 ]] || live=0
    fi
    if [[ $live != 1 && -n ${scaled_output:-} ]]; then
        export BB_RENDER_RES=$scaled_render BB_OUTPUT_RES=$scaled_output BB_AUTO_RENDER_RES=1
    fi
    if [[ -f scripts/patches.py && -d "${BB_PATCHES_DIR:-$HERE/patches}" ]]; then
        "$PYTHON" scripts/patches.py --out "$out" --fps "${BB_FPS:-uncap}" \
            --settings "$BB_CONFIG" --game-dir "$game" --render-res "${BB_RENDER_RES:-}" --output-res "${BB_OUTPUT_RES:-}" \
            --patches-dir "${BB_PATCHES_DIR:-$HERE/patches}" || true
    fi
    probe=${BB_PROBE:-out/ps4-runtime}
    probe_args=(--app0 "$game" "$@")
    if [[ -n ${mod_game:-} ]]; then
        "$probe" "${probe_args[@]}" &
        mod_pid=$!
        trap 'kill -TERM "$mod_pid" 2>/dev/null || true' TERM INT
        mod_status=0
        wait "$mod_pid" || mod_status=$?
        if kill -0 "$mod_pid" 2>/dev/null; then wait "$mod_pid" || mod_status=$?; fi
        exit "$mod_status"
    fi
    exec "$probe" "${probe_args[@]}"
fi

exec "$HERE/play" "$@"
