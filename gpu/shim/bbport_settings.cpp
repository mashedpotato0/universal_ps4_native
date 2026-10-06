// SPDX-License-Identifier: GPL-2.0-or-later
#include "bbport_settings.h"
#include "game_profile.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <SDL3/SDL.h>

namespace BbSettings {

namespace {

const char* Path() {
    const char* env = std::getenv("BB_CONFIG");
    return env && env[0] ? env : "bbport.ini";
}

float Clamp(float v, float lo, float hi) {
    return std::clamp(v, lo, hi);
}

void Set(Values& v, const std::string& key, const std::string& value) {
    const float f = float(std::atof(value.c_str()));
    const int i = std::atoi(value.c_str());
    if (key == "upscaler") {
        for (int u = 0; u < UpscalerCount; ++u) {
            if (value == UpscalerName(u)) {
                v.upscaler = u;
            }
        }
    } else if (key == "preset") {
        v.preset = std::clamp(i, 0, PresetCount - 1);
    } else if (key == "sharpen") {
        v.sharpen = i != 0;
    } else if (key == "sharpness") {
        v.sharpness = Clamp(f, 0.0f, 2.0f);
    } else if (key == "jitter") {
        v.jitter = i != 0;
    } else if (key == "reactive") {
        v.reactive = i != 0;
    } else if (key == "object_motion") {
        v.object_motion = i != 0;
    } else if (key == "reactive_scale") {
        v.reactive_scale = Clamp(f, 0.0f, 16.0f);
    } else if (key == "reactive_threshold") {
        v.reactive_threshold = Clamp(f, 0.0f, 1.0f);
    } else if (key == "reactive_max") {
        v.reactive_max = Clamp(f, 0.0f, 1.0f);
    } else if (key == "debug_view") {
        v.debug_view = std::clamp(i, 0, DebugViewCount - 1);
    } else if (key == "show_fps") {
        v.show_fps = i != 0;
    } else if (key == "uncap_fps") {
        v.uncap_fps = i != 0;
    } else if (key == "fps_limit") {
        v.fps_limit = std::max(0, i);
    } else if (key == "fsr4_auto_exposure") {
        v.fsr4_auto_exposure = i != 0;
    } else if (key == "fsr4_invert_jitter") {
        v.fsr4_invert_jitter = i != 0;
    } else if (key == "model_lod") {
        v.model_lod = std::clamp(i, -2, 2);
    } else if (key == "live_resolution") {
        v.live_resolution = value == "auto" ? -1 : std::clamp(i, 0, 1);
    } else if (key == "output_res") {
        for (int r = 0; r < OutputCount; ++r) {
            if (value == std::to_string(OutputWidths[r]) + "x" + std::to_string(OutputHeights[r])) {
                v.output_res = r;
            }
        }
    } else if (key == "mouse_sensitivity") {
        v.mouse_sensitivity = Clamp(f, 0.5f, 20.0f);
    } else {
        bool handled = false;
        for (int a = 0; a < ActionCount; ++a) {
            if (key == Actions[a].key) {
                int code = BindingFromName(value);
                if (code > 0) v.key_bindings[Actions[a].action] = code;
                handled = true;
                break;
            }
        }
        if (!handled) {
            static const struct { const char* old_key; int action; } compat[] = {
                {"key_forward", BB_ACTION_LS_UP},
                {"key_backward", BB_ACTION_LS_DOWN},
                {"key_left", BB_ACTION_LS_LEFT},
                {"key_right", BB_ACTION_LS_RIGHT},
                {"key_interact", BB_ACTION_CROSS},
                {"key_dodge", BB_ACTION_CIRCLE},
                {"key_use_item", BB_ACTION_SQUARE},
                {"key_switch_mode", BB_ACTION_TRIANGLE},
                {"key_trick", BB_ACTION_L1},
                {"key_light_atk", BB_ACTION_R1},
                {"key_heavy_atk", BB_ACTION_R2},
                {"key_gun", BB_ACTION_L2},
                {"key_lock_on", BB_ACTION_R3},
                {"key_gesture", BB_ACTION_L3},
                {"key_menu", BB_ACTION_OPTIONS},
                {"key_up", BB_ACTION_DPAD_UP},
                {"key_down", BB_ACTION_DPAD_DOWN},
                {"key_dleft", BB_ACTION_DPAD_LEFT},
                {"key_dright", BB_ACTION_DPAD_RIGHT},
            };
            for (const auto& c : compat) {
                if (key == c.old_key) {
                    int code = BindingFromName(value);
                    if (code > 0) v.key_bindings[c.action] = code;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled) {
            for (int e = 0; e < EffectCount; ++e) {
                if (key == Effects[e].key) {
                    v.effects[e] = i != 0;
                }
            }
        }
    }
}

} // namespace

Values& Get() {
    static Values values;
    return values;
}

void Load() {
    auto& v = Get();
    ResetDefaultBindings();
    for (int e = 0; e < EffectCount; ++e) {
        v.effects[e] = Effects[e].default_on;
    }
    const auto& profile = BbProfile::Get();
    bool in_active_section = true;
    if (FILE* file = std::fopen(Path(), "r")) {
        char line[256];
        while (std::fgets(line, sizeof(line), file)) {
            std::string text{line};
            const auto first = text.find_first_not_of(" \t");
            if (first == std::string::npos || text[first] == '#') {
                continue;
            }
            text.erase(0, first);
            text.erase(text.find_last_not_of(" \t\r\n") + 1);
            if (text.front() == '[' && text.back() == ']') {
                std::string section = text.substr(1, text.size() - 2);
                in_active_section = (section == "General" || section == "general" ||
                                     section == profile.serial ||
                                     (!profile.serial.empty() && section.find(profile.serial) != std::string::npos));
                continue;
            }
            if (!in_active_section) {
                continue;
            }
            const auto eq = text.find('=');
            if (eq == std::string::npos) {
                continue;
            }
            std::string key = text.substr(0, eq);
            std::string val = text.substr(eq + 1);
            key.erase(key.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            BbProfile::Configure(key, val);
            Set(v, key, val);
        }
        std::fclose(file);
        std::printf("Settings: %s (profile [%s])\n", Path(), profile.serial.c_str());
    }
    // Environment overrides (scripts, A/B tests).
    if (const char* env = std::getenv("BB_UPSCALER")) {
        v.upscaler = UpscalerOff;
        for (int u = 0; u < UpscalerCount; ++u) {
            if (std::strcmp(env, UpscalerName(u)) == 0) v.upscaler = u;
        }
    }
    const std::pair<const char*, const char*> env_keys[] = {
        {"BB_FSR_SHARPNESS", "sharpness"},        {"BB_JITTER", "jitter"},
        {"BB_REACTIVE", "reactive"},              {"BB_REACTIVE_SCALE", "reactive_scale"},
        {"BB_REACTIVE_THRESHOLD", "reactive_threshold"}, {"BB_REACTIVE_MAX", "reactive_max"},
        {"BB_UPSCALE_PRESET", "preset"},            {"BB_OBJECT_MOTION", "object_motion"},
    };
    for (const auto& [env, key] : env_keys) {
        if (const char* value = std::getenv(env)) {
            Set(v, key, value);
        }
    }
    if (const char* env = std::getenv("BB_LOW_SPEC")) {
        if (env[0] == '1' || env[0] == 'y' || env[0] == 't') {
            BbProfile::Configure("low_spec_mode", "1");
        }
    }
    if (const char* env = std::getenv("BB_NO_CAP_FPS")) {
        v.uncap_fps = (env[0] == '1' || env[0] == 'y' || env[0] == 't');
        if (v.uncap_fps) v.fps_limit = 0;
    }
    if (const char* env = std::getenv("BB_FPS_LIMIT")) {
        int val = std::atoi(env);
        if (val == 0) {
            v.uncap_fps = true;
            v.fps_limit = 0;
        } else if (val > 0) {
            v.uncap_fps = false;
            v.fps_limit = val;
        }
    }
    v.startup_preset = v.preset;
    v.startup_upscaler = v.upscaler;
    v.startup_object_motion = v.object_motion;
    for (int e = 0; e < EffectCount; ++e) {
        v.startup_effects[e] = v.effects[e];
    }
    v.startup_model_lod = v.model_lod;
    v.startup_output_res = v.output_res;
    v.startup_live_resolution = v.live_resolution;
}

void ConfigureUpscalerSupport(bool fsr4, bool fsr411) {
    auto& v = Get();
    v.fsr4_supported = fsr4;
    v.fsr411_supported = fsr4 && fsr411;
    const int requested = v.upscaler;
    if ((requested == UpscalerFsr4 && !v.fsr4_supported) ||
        (requested == UpscalerFsr411 && !v.fsr411_supported)) {
        v.fsr4_problem = "GPU does not support the selected FSR 4 shaders; using FSR 3.1";
        std::printf("Upscaler: %s unsupported on this GPU; falling back to FSR 3.1 before the first frame\n",
                    UpscalerName(requested));
        v.upscaler = UpscalerFsr3;
    }
}

bool FixedRenderSession() {
    const char* size = std::getenv("BB_RENDER_RES");
    return size && size[0];
}

int RenderPreset() {
    const auto& v = Get();
    return FixedRenderSession() ? v.startup_preset :
        v.upscaler == UpscalerTaa ? NativeAA : v.preset.load();
}

bool ResolutionNeedsRestart() {
    const auto& v = Get();
    // TAA needs the live path (native guest targets): run.sh selects it on restart.
    return FixedRenderSession() &&
        (v.preset != v.startup_preset || v.output_res != v.startup_output_res ||
         (v.upscaler == UpscalerOff) != (v.startup_upscaler == UpscalerOff) ||
         (v.upscaler == UpscalerTaa) != (v.startup_upscaler == UpscalerTaa));
}

void Save() {
    const auto& v = Get();
    FILE* file = std::fopen(Path(), "w");
    if (!file) {
        std::printf("Settings: cannot write %s\n", Path());
        return;
    }
    std::fprintf(file,
                 "# bbport settings (in-game menu: Insert / L3+R3)\n"
                 "upscaler=%s\npreset=%d\nsharpen=%d\nsharpness=%.2f\njitter=%d\n"
                 "reactive=%d\nobject_motion=%d\nreactive_scale=%.2f\nreactive_threshold=%.2f\nreactive_max=%.2f\n"
                 "debug_view=%d\nshow_fps=%d\nfsr4_auto_exposure=%d\nfsr4_invert_jitter=%d\n",
                 UpscalerName(v.upscaler), v.preset.load(), int(v.sharpen.load()),
                 v.sharpness.load(), int(v.jitter.load()), int(v.reactive.load()),
                 int(v.object_motion.load()),
                 v.reactive_scale.load(), v.reactive_threshold.load(), v.reactive_max.load(),
                 v.debug_view.load(), int(v.show_fps.load()),
                 int(v.fsr4_auto_exposure.load()), int(v.fsr4_invert_jitter.load()));
    // Read by patches.py at start.
    for (int e = 0; e < EffectCount; ++e) {
        std::fprintf(file, "%s=%d\n", Effects[e].key, int(v.effects[e].load()));
    }
    std::fprintf(file, "model_lod=%d\noutput_res=%dx%d\n", v.model_lod.load(),
                 OutputWidths[v.output_res], OutputHeights[v.output_res]);
    std::fprintf(file, "uncap_fps=%d\nfps_limit=%d\n", int(v.uncap_fps.load()), v.fps_limit.load());
    std::fprintf(file, "mouse_sensitivity=%.2f\n", v.mouse_sensitivity.load());
    for (int a = 0; a < ActionCount; ++a) {
        std::fprintf(file, "%s=%s\n", Actions[a].key,
                     BindingName(v.key_bindings[Actions[a].action].load()).c_str());
    }
    // Read by run.sh at start.
    std::fprintf(file, "live_resolution=%s\n", v.live_resolution < 0 ? "auto"
                                                  : v.live_resolution ? "1" : "0");
    std::fclose(file);
}

float PresetScale(int preset) {
    static constexpr float scales[PresetCount] = {1.0f, 1.5f, 1.7f, 2.0f, 3.0f};
    return scales[std::clamp(preset, 0, PresetCount - 1)];
}

const char* PresetName(int preset) {
    static constexpr const char* names[PresetCount] = {"Native AA", "Quality", "Balanced",
                                                       "Performance", "Ultra Performance"};
    return names[std::clamp(preset, 0, PresetCount - 1)];
}

const char* UpscalerName(int upscaler) {
    static constexpr const char* names[UpscalerCount] = {"off", "fsr3", "fsr4", "fsr411", "taa"};
    return names[std::clamp(upscaler, 0, UpscalerCount - 1)];
}

std::string BindingName(int code) {
    if (code == BB_MOUSE_LEFT) return "Mouse Left";
    if (code == BB_MOUSE_RIGHT) return "Mouse Right";
    if (code == BB_MOUSE_MIDDLE) return "Mouse Middle";
    if (code == BB_MOUSE_X1) return "Mouse 4";
    if (code == BB_MOUSE_X2) return "Mouse 5";
    if (code > 0 && code < 512) {
        const char* name = SDL_GetScancodeName((SDL_Scancode)code);
        if (name && name[0]) return name;
    }
    return "None";
}

int BindingFromName(const std::string& name) {
    if (name == "Mouse Left" || name == "LMB") return BB_MOUSE_LEFT;
    if (name == "Mouse Right" || name == "RMB") return BB_MOUSE_RIGHT;
    if (name == "Mouse Middle" || name == "MMB") return BB_MOUSE_MIDDLE;
    if (name == "Mouse 4" || name == "Mouse X1") return BB_MOUSE_X1;
    if (name == "Mouse 5" || name == "Mouse X2") return BB_MOUSE_X2;
    SDL_Scancode sc = SDL_GetScancodeFromName(name.c_str());
    return sc != SDL_SCANCODE_UNKNOWN ? sc : 0;
}

void ResetDefaultBindings() {
    auto& v = Get();
    v.mouse_sensitivity = 3.5f;
    for (int a = 0; a < ActionCount; ++a) {
        v.key_bindings[Actions[a].action] = Actions[a].default_code;
    }
}

} // namespace BbSettings

extern "C" float bbgpu_get_mouse_sensitivity(void) {
    return BbSettings::Get().mouse_sensitivity.load();
}

extern "C" int32_t bbgpu_get_input_binding(int32_t action) {
    if (action < 0 || action >= BB_ACTION_COUNT) return 0;
    return BbSettings::Get().key_bindings[action].load();
}
