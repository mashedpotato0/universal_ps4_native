#include "runtime_translator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

int main(int argc, char **argv) {
    printf("ps4 translator for aarch64 android and linux\n");

    if (argc < 2) {
        printf("usage: %s <binary_file | --test>\n", argv[0]);
        return 0;
    }

    if (!strcmp(argv[1], "--test")) {
        printf("running self test...\n");
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
        printf("executed %d instructions, rax=%" PRIu64 ", status=%d\n", ran, cpu.regs[X64_REG_RAX], (int)cpu.status);
        if (cpu.regs[X64_REG_RAX] == 10 && cpu.status == X64_STATUS_SYSCALL) {
            printf("translator self test passed\n");
            return 0;
        } else {
            fprintf(stderr, "translator self test failed\n");
            return 1;
        }
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "error: cannot open input file %s\n", argv[1]);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz <= 0 || sz > 128 * 1024 * 1024) {
        fprintf(stderr, "error: invalid file size %ld\n", sz);
        fclose(f);
        return 1;
    }

    uint8_t *code = malloc((size_t)sz);
    if (!code) {
        fprintf(stderr, "error: allocation failed\n");
        fclose(f);
        return 1;
    }

    if (fread(code, 1, (size_t)sz, f) != (size_t)sz) {
        fprintf(stderr, "error: read failed\n");
        free(code);
        fclose(f);
        return 1;
    }
    fclose(f);

    printf("loaded %ld bytes into translator\n", sz);

    x64_cache_t *cache = x64_cache_create(4096);
    x64_cpu_t cpu;
    size_t stack_sz = 8 * 1024 * 1024;
    uint8_t *stack = malloc(stack_sz);
    x64_cpu_init(&cpu, (uint64_t)(uintptr_t)code, (uint64_t)(uintptr_t)(stack + stack_sz - 64));

    printf("starting execution at 0x%" PRIx64 "...\n", cpu.rip);
    x64_run(&cpu, 10000);

    runtime_translator_report();
    x64_cache_destroy(cache);
    free(stack);
    free(code);
    return 0;
}
