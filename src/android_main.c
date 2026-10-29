#include <android/native_activity.h>
#include <android/log.h>
#include "runtime_translator.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#define LOG_TAG "PS4Translator"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window) {
    (void)activity;
    (void)window;
    LOGI("native window created: %p", window);
}

static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window) {
    (void)activity;
    (void)window;
    LOGI("native window destroyed: %p", window);
}

static void onDestroy(ANativeActivity* activity) {
    (void)activity;
    LOGI("native activity destroyed");
}

void ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize) {
    (void)savedState;
    (void)savedStateSize;
    LOGI("ANativeActivity_onCreate: starting ps4 translator runtime");

    if (activity && activity->callbacks) {
        activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
        activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
        activity->callbacks->onDestroy = onDestroy;
    }

    // run translator test
    uint8_t test_code[] = {
        0x48, 0xC7, 0xC0, 0x07, 0x00, 0x00, 0x00, // mov rax 7
        0x48, 0xC7, 0xC3, 0x03, 0x00, 0x00, 0x00, // mov rbx 3
        0x48, 0x01, 0xD8,                         // add rax rbx
        0x0F, 0x05                                // syscall
    };
    x64_cpu_t cpu;
    uint8_t stack[4096];
    x64_cpu_init(&cpu, (uint64_t)(uintptr_t)test_code, (uint64_t)(uintptr_t)(stack + sizeof(stack) - 64));
    int ran = x64_run(&cpu, 100);
    LOGI("translator test: ran %d insns, rax=%" PRIu64 ", status=%d", ran, cpu.regs[X64_REG_RAX], (int)cpu.status);
}
