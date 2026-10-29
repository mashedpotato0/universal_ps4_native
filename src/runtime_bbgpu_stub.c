#include <stdint.h>
#include <stddef.h>
#include "gpu/bbgpu.h"

// bbgpu fallback stubs
uintptr_t bbgpu_resolve(const char *scoped_nid) {
    (void)scoped_nid;
    return 0;
}

int bbgpu_overlay_captures_input(void) {
    return 0;
}

int32_t bbgpu_get_input_binding(int32_t action) {
    return action;
}

float bbgpu_get_mouse_sensitivity(void) {
    return 1.0f;
}

int bbgpu_text_input_begin(const char *initial_utf8, const char *prompt_utf8) {
    (void)initial_utf8;
    (void)prompt_utf8;
    return 0;
}

int bbgpu_text_input_poll(char *out_utf8, uint64_t size) {
    (void)out_utf8;
    (void)size;
    return 0;
}

void bbgpu_register_kernel(void) {
}

int bbgpu_init(const BbGpuConfig *config) {
    (void)config;
    return 0;
}

int bbgpu_handle_fault(void *ucontext, void *address) {
    (void)ucontext;
    (void)address;
    return 0;
}

void bbgpu_dump_guest_writes(void *ucontext) {
    (void)ucontext;
}

unsigned bbgpu_symbol_count(void) {
    return 0;
}
