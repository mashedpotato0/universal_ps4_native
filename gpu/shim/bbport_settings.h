// SPDX-License-Identifier: GPL-2.0-or-later
// bbport: user settings changed at run time from the in-game menu (bbport_overlay.h) and kept
// in bbport.ini (BB_CONFIG overrides the path). Environment variables override the file at
// start. Readers load the atomics every frame; writers are the menu and Load().

#pragma once

#include <atomic>
#include <string>
#include <SDL3/SDL_scancode.h>
#include "../bbgpu.h"

namespace BbSettings {

enum Upscaler : int { UpscalerOff = 0, UpscalerFsr3 = 1, UpscalerFsr4 = 2, UpscalerFsr411 = 3,
                      UpscalerTaa = 4, UpscalerCount };
/// FSR 4 v07 or FSR 4.1.1: the same inputs, settings and placement in the frame.
inline bool IsFsr4(int upscaler) {
    return upscaler == UpscalerFsr4 || upscaler == UpscalerFsr411;
}
enum Preset : int { NativeAA = 0, Quality, Balanced, Performance, UltraPerformance, PresetCount };
enum DebugView : int { DebugNone = 0, DebugReactive = 1, DebugMotion = 2, DebugViewCount };

/// Game effects switched by the community patches at start (patches.py EFFECTS): ini key,
/// menu label, default (the game's own behaviour).
struct Effect {
    const char* key;
    const char* label;
    bool default_on;
};
inline constexpr Effect Effects[] = {
    {"effect_chromatic_aberration", "Chromatic aberration", false},
    {"effect_dof", "Depth of field (DoF)", false},
    {"effect_motion_blur", "Motion blur", false},
    {"effect_ssao", "SSAO", false},
    {"effect_game_aa", "In-game anti-aliasing", true},
    {"effect_dynamic_shadows", "Dynamic light shadows", false},
    {"effect_ssr", "SSR reflections (not in base game)", false},
    {"skip_intro", "Skip intro videos", false},
    {"debug_camera", "Free camera (Cross + L3)", false},
    {"debug_menu", "Debug menu (needs font files)", false},
};
inline constexpr int EffectCount = int(sizeof(Effects) / sizeof(Effects[0]));
/// Live output resolutions: the upscaler's output and the UI host targets.
inline constexpr int OutputWidths[] = {1280, 1920, 2560, 3840};
inline constexpr int OutputHeights[] = {720, 1080, 1440, 2160};
inline constexpr int OutputCount = 4;
inline constexpr int OutputDefault = 1; ///< 1920x1080, the game's own size

struct ActionDesc {
    int action;
    const char* key;
    const char* label;
    int default_code;
};

inline constexpr ActionDesc Actions[] = {
    {BB_ACTION_LS_UP, "key_ls_up", "Left Stick Up", SDL_SCANCODE_W},
    {BB_ACTION_LS_DOWN, "key_ls_down", "Left Stick Down", SDL_SCANCODE_S},
    {BB_ACTION_LS_LEFT, "key_ls_left", "Left Stick Left", SDL_SCANCODE_A},
    {BB_ACTION_LS_RIGHT, "key_ls_right", "Left Stick Right", SDL_SCANCODE_D},
    {BB_ACTION_CROSS, "key_cross", "Cross", SDL_SCANCODE_E},
    {BB_ACTION_CIRCLE, "key_circle", "Circle", SDL_SCANCODE_SPACE},
    {BB_ACTION_SQUARE, "key_square", "Square", SDL_SCANCODE_R},
    {BB_ACTION_TRIANGLE, "key_triangle", "Triangle", SDL_SCANCODE_F},
    {BB_ACTION_L1, "key_l1", "L1", SDL_SCANCODE_Q},
    {BB_ACTION_R1, "key_r1", "R1", BB_MOUSE_LEFT},
    {BB_ACTION_L2, "key_l2", "L2", BB_MOUSE_RIGHT},
    {BB_ACTION_R2, "key_r2", "R2", BB_MOUSE_X2},
    {BB_ACTION_L3, "key_l3", "L3", SDL_SCANCODE_Z},
    {BB_ACTION_R3, "key_r3", "R3", BB_MOUSE_MIDDLE},
    {BB_ACTION_OPTIONS, "key_options", "Options", SDL_SCANCODE_TAB},
    {BB_ACTION_TOUCHPAD, "key_touchpad", "Touchpad", SDL_SCANCODE_T},
    {BB_ACTION_DPAD_UP, "key_dpad_up", "D-Pad Up", SDL_SCANCODE_UP},
    {BB_ACTION_DPAD_DOWN, "key_dpad_down", "D-Pad Down", SDL_SCANCODE_DOWN},
    {BB_ACTION_DPAD_LEFT, "key_dpad_left", "D-Pad Left", SDL_SCANCODE_LEFT},
    {BB_ACTION_DPAD_RIGHT, "key_dpad_right", "D-Pad Right", SDL_SCANCODE_RIGHT},
};
inline constexpr int ActionCount = int(sizeof(Actions) / sizeof(Actions[0]));

std::string BindingName(int code);
int BindingFromName(const std::string& name);
void ResetDefaultBindings();

struct Values {
    std::atomic<int> upscaler{UpscalerFsr3};
    std::atomic<int> preset{Quality};
    std::atomic<bool> sharpen{true};
    std::atomic<float> sharpness{0.4f};
    std::atomic<bool> jitter{true};
    std::atomic<bool> reactive{false};
    std::atomic<bool> object_motion{true};
    std::atomic<float> reactive_scale{1.0f};
    std::atomic<float> reactive_threshold{0.2f};
    std::atomic<float> reactive_max{0.9f};
    std::atomic<int> debug_view{DebugNone};
    std::atomic<bool> show_fps{false};
    std::atomic<bool> uncap_fps{true};
    std::atomic<int> fps_limit{0};
    std::atomic<float> mouse_sensitivity{3.5f};
    std::atomic<int32_t> key_bindings[BB_ACTION_COUNT]{};
    // FSR 4 checks (menu): the provider's auto exposure, the jitter sign it is given.
    std::atomic<bool> fsr4_auto_exposure{true};
    std::atomic<bool> fsr4_invert_jitter{false};
    std::atomic<int> active_render_width{1920}, active_render_height{1080};
    /// Applied at start (patches.py); the menu shows when a restart is needed.
    std::atomic<bool> effects[EffectCount]{};
    std::atomic<int> model_lod{0}; ///< -2 highest .. 2 lowest, 0 the game's
    std::atomic<int> output_res{OutputDefault}; ///< index into OutputWidths
    /// Live resolution and preset changes (run.sh): 0 off by default (startup patch, fastest
    /// on the Steam Deck and older GPUs), -1 auto (strong discrete GPUs), 1 on. On restart.
    std::atomic<int> live_resolution{0};
    /// Why FSR 4 cannot run (assets, device features), or null. Set by the renderer.
    std::atomic<const char*> fsr4_problem{nullptr};
    std::atomic<bool> fsr4_supported{false}, fsr411_supported{false};

    /// Startup settings for the explicit BB_RENDER_RES compatibility patch only.
    int startup_preset = NativeAA;
    int startup_upscaler = UpscalerFsr3;
    bool startup_object_motion = true;
    bool startup_effects[EffectCount]{};
    int startup_model_lod = 0;
    int startup_output_res = OutputDefault;
    int startup_live_resolution = 0;
};

Values& Get();

/// Reads the file, then the environment overrides. Called once at start.
void Load();
/// Checks the loaded choice before the first frame; unsupported FSR 4 uses FSR 3.1.
void ConfigureUpscalerSupport(bool fsr4, bool fsr411);
/// Startup-patched scene dimensions cannot change until run.sh prepares a new image.
bool FixedRenderSession();
int RenderPreset();
bool ResolutionNeedsRestart();
/// Writes the file (menu changes).
void Save();

/// Render resolution divisor of a preset (1.0 native, 1.5 quality, ...).
float PresetScale(int preset);
const char* PresetName(int preset);
const char* UpscalerName(int upscaler);

} // namespace BbSettings
