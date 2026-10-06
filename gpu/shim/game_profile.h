// game profile definitions
#pragma once

#include <string>
#include <string_view>
#include "video_core/amdgpu/pixel_format.h"

namespace BbProfile {

enum class GameType {
    Generic,
    Bloodborne,
    Sekiro,
    DarkSouls3,
    EldenRing,
};

struct Profile {
    GameType type = GameType::Generic;
    std::string serial;
    std::string name;

    // profile toggles
    bool camera_motion = false;
    bool scene_resolution_hack = false;
    bool safe_compute_clear = true;
    bool compute_image_copy = true;
    bool compute_image_clear = true;
    bool cmask_fast_clear = true;
    bool format_remap = true;
    bool low_spec_mode = false;
    int internal_res_scale = 100;
    int vram_budget_mb = 4096;
};

// retrieves active game profile
const Profile& Get();

// initializes profile for serial
void Initialize(std::string_view serial);

// updates profile options from config
void Configure(std::string_view key, std::string_view val);

} // namespace BbProfile
