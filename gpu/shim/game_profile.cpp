// game profile implementation
#include "game_profile.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace BbProfile {

static Profile active_profile{};

static bool MatchesSerial(std::string_view serial, std::initializer_list<std::string_view> candidates) {
    for (const auto& candidate : candidates) {
        if (serial.find(candidate) != std::string_view::npos) {
            return true;
        }
    }
    return false;
}

void Initialize(std::string_view serial) {
    active_profile = Profile{};
    active_profile.serial = std::string(serial);

    if (MatchesSerial(serial, {"CUSA03173", "CUSA03014", "CUSA03023", "CUSA00207", "CUSA00208"})) {
        active_profile.type = GameType::Bloodborne;
        active_profile.name = "Bloodborne";
        active_profile.camera_motion = true;
        active_profile.scene_resolution_hack = true;
        active_profile.safe_compute_clear = false;
        active_profile.format_remap = false;
    } else if (MatchesSerial(serial, {"CUSA13801", "CUSA13800", "CUSA13802", "CUSA13803", "CUSA13804"})) {
        active_profile.type = GameType::Sekiro;
        active_profile.name = "Sekiro: Shadows Die Twice";
        active_profile.camera_motion = false;
        active_profile.scene_resolution_hack = false;
        active_profile.safe_compute_clear = true;
        active_profile.format_remap = true;
    } else if (MatchesSerial(serial, {"CUSA03388", "CUSA03365", "CUSA03366", "CUSA03367"})) {
        active_profile.type = GameType::DarkSouls3;
        active_profile.name = "Dark Souls III";
        active_profile.camera_motion = false;
        active_profile.scene_resolution_hack = false;
        active_profile.safe_compute_clear = true;
        active_profile.format_remap = true;
    } else {
        active_profile.type = GameType::Generic;
        active_profile.name = "Universal PS4";
        active_profile.camera_motion = false;
        active_profile.scene_resolution_hack = false;
        active_profile.safe_compute_clear = true;
        active_profile.format_remap = true;
    }

    std::printf("Profile: selected %s [%s] (camera_motion=%d, safe_clear=%d, format_remap=%d, copy=%d, clear=%d, cmask=%d)\n",
                active_profile.name.c_str(), active_profile.serial.c_str(),
                active_profile.camera_motion, active_profile.safe_compute_clear,
                active_profile.format_remap, active_profile.compute_image_copy,
                active_profile.compute_image_clear, active_profile.cmask_fast_clear);
}

const Profile& Get() {
    return active_profile;
}

void Configure(std::string_view key, std::string_view val) {
    const int int_val = std::atoi(val.data());
    if (key == "camera_motion") {
        active_profile.camera_motion = (int_val != 0);
    } else if (key == "scene_resolution_hack") {
        active_profile.scene_resolution_hack = (int_val != 0);
    } else if (key == "safe_compute_clear" || key == "compute_clear_safe") {
        active_profile.safe_compute_clear = (int_val != 0);
    } else if (key == "compute_image_copy") {
        active_profile.compute_image_copy = (int_val != 0);
    } else if (key == "compute_image_clear") {
        active_profile.compute_image_clear = (int_val != 0);
    } else if (key == "cmask_fast_clear") {
        active_profile.cmask_fast_clear = (int_val != 0);
    } else if (key == "format_remap" || key == "snorm_format_remap") {
        active_profile.format_remap = (int_val != 0);
    } else if (key == "low_spec_mode") {
        active_profile.low_spec_mode = (int_val != 0);
        if (active_profile.low_spec_mode) {
            if (active_profile.internal_res_scale == 100) {
                active_profile.internal_res_scale = 67;
            }
            active_profile.vram_budget_mb = 2048;
        }
    } else if (key == "internal_res_scale") {
        active_profile.internal_res_scale = std::clamp(int_val, 25, 200);
    } else if (key == "vram_budget_mb") {
        active_profile.vram_budget_mb = std::max(512, int_val);
    }
}

} // namespace BbProfile
