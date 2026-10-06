# shared launcher environment sourced by every game launcher
# expects HERE to point at the repo root
# low spec mode turns on for integrated gpus small vram or under 12 gib ram
# force with BB_LOW_SPEC=1 or disable with BB_LOW_SPEC=0

# default config from template on fresh clones
export BB_CONFIG="${BB_CONFIG:-$HERE/bbport.ini}"
if [[ ! -f "$BB_CONFIG" && -f "$HERE/bbport.ini.example" ]]; then
    cp "$HERE/bbport.ini.example" "$BB_CONFIG"
fi

# hardware profile written by build.sh or created here on first launch
if [[ ! -f "$HERE/out/hardware_profile.env" && -f "$HERE/scripts/detect_hardware.py" ]]; then
    python3 "$HERE/scripts/detect_hardware.py" --quiet --save-env "$HERE/out/hardware_profile.env" >/dev/null 2>&1 || true
fi
if [[ -f "$HERE/out/hardware_profile.env" ]]; then
    # shellcheck disable=SC1091
    source "$HERE/out/hardware_profile.env"
fi

# prefer a discrete gpu when present otherwise select integrated gpu if available
if [[ -z "${BB_GPU_ID:-}" ]]; then
    if [[ "${HW_DISCRETE_GPU_ID:--1}" != "-1" ]]; then
        export BB_GPU_ID="$HW_DISCRETE_GPU_ID"
    elif [[ "${HW_INTEGRATED_GPU_ID:--1}" != "-1" ]]; then
        export BB_GPU_ID="$HW_INTEGRATED_GPU_ID"
    fi
fi

export BB_LOW_SPEC="${BB_LOW_SPEC:-${HW_LOW_SPEC:-0}}"
export SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS=1
export BB_FRAMES_AHEAD="${BB_FRAMES_AHEAD:-1}"

if [[ "$BB_LOW_SPEC" == "1" ]]; then
    # shared memory gpus stall on extra queued frames so present at vsync
    export BB_PRESENT_MODE="${BB_PRESENT_MODE:-Fifo}"
    # cap at 60 so the igpu is not pushed past what the game was built for
    export BB_FPS="${BB_FPS:-60}"
    export BB_FPS_LIMIT="${BB_FPS_LIMIT:-60}"
    export BB_NO_CAP_FPS="${BB_NO_CAP_FPS:-0}"
    export BB_VBLANK_HZ="${BB_VBLANK_HZ:-60}"
    # retail ps4 direct memory size keeps system ram free for the igpu
    if awk "BEGIN{exit !(${HW_RAM_GIB:-16} < 12)}"; then
        export BB_DMEM_MB="${BB_DMEM_MB:-5056}"
    fi
    # 0 lets the texture cache follow the driver memory budget
    export BB_GC_BUDGET_MB="${BB_GC_BUDGET_MB:-0}"
    export BB_UPSCALER="${BB_UPSCALER:-off}"
    export BB_READBACKS="${BB_READBACKS:-1}"
    echo "Launcher: low spec profile (${HW_GPU_TYPE:-unknown} gpu, ${HW_VRAM_MB:-0} MiB, ${HW_RAM_GIB:-?} GiB ram)"
else
    export BB_NO_CAP_FPS="${BB_NO_CAP_FPS:-1}"
    export BB_FPS_LIMIT="${BB_FPS_LIMIT:-0}"
    export BB_VBLANK_HZ="${BB_VBLANK_HZ:-0}"
    export BB_FPS="${BB_FPS:-uncap}"
    export BB_DMEM_MB="${BB_DMEM_MB:-8192}"
    export BB_PRESENT_MODE="${BB_PRESENT_MODE:-Mailbox}"
fi
