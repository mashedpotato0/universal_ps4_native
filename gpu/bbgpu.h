/* C interface of the GPU library (gpu/): shadPS4's Liverpool/Vulkan video core,
 * GnmDriver, VideoOut and kernel event queues, adapted to the native loader. */
#ifndef BBGPU_H
#define BBGPU_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    const char *title;          /* window title */
    const char *serial;         /* CUSA id, names the pipeline cache */
    const char *user_dir;       /* pipeline cache/logs directory */
    uint32_t sdk_version;       /* from the eboot's procparam */
    uint32_t psf_attributes;    /* param.sfo ATTRIBUTE */
    int32_t width, height;      /* initial window size */
} BbGpuConfig;
/* Registers kernel event queues (needed with or without graphics). */
void bbgpu_register_kernel(void);
/* Creates window, Vulkan device, presenter and GPU command processor. */
int bbgpu_init(const BbGpuConfig *config);
/* Function for an imported NID ("NID#lib#mod"), or 0 when the GPU library does not provide it. */
uintptr_t bbgpu_resolve(const char *scoped_nid);
/* Called first by the loader's SIGSEGV handler: 1 when a GPU page-tracking fault was handled. */
int bbgpu_handle_fault(void *ucontext, void *address);
/* BB_WRITE_LOG=1: prints the logged GPU-side writes to guest memory near the fault. */
void bbgpu_dump_guest_writes(void *ucontext);
/* Keyboard text entry through the game window (IME dialog). begin returns 0 when
 * no window exists; poll returns 0 typing, 1 confirmed, 2 cancelled (UTF-8 text). */
int bbgpu_text_input_begin(const char *initial_utf8, const char *prompt_utf8);
int bbgpu_text_input_poll(char *out_utf8, uint64_t size);
/* 1 while the in-game settings menu is open: the game's pad input is held neutral. */
int bbgpu_overlay_captures_input(void);
/* Number of symbols registered by the vendored libraries (diagnostics). */
unsigned bbgpu_symbol_count(void);

enum BbAction {
    BB_ACTION_FORWARD = 0,
    BB_ACTION_BACKWARD,
    BB_ACTION_LEFT,
    BB_ACTION_RIGHT,
    BB_ACTION_INTERACT,
    BB_ACTION_DODGE,
    BB_ACTION_USE_ITEM,
    BB_ACTION_SWITCH_MODE,
    BB_ACTION_TRICK,
    BB_ACTION_LIGHT_ATK,
    BB_ACTION_HEAVY_ATK,
    BB_ACTION_GUN,
    BB_ACTION_LOCK_ON,
    BB_ACTION_GESTURE,
    BB_ACTION_MENU,
    BB_ACTION_UP,
    BB_ACTION_DOWN,
    BB_ACTION_DLEFT,
    BB_ACTION_DRIGHT,
    BB_ACTION_COUNT
};

#define BB_MOUSE_BASE 1000
#define BB_MOUSE_LEFT 1001
#define BB_MOUSE_RIGHT 1002
#define BB_MOUSE_MIDDLE 1003
#define BB_MOUSE_X1 1004
#define BB_MOUSE_X2 1005

/* input settings */
float bbgpu_get_mouse_sensitivity(void);
int32_t bbgpu_get_input_binding(int32_t action);

#ifdef __cplusplus
}
#endif
#endif
