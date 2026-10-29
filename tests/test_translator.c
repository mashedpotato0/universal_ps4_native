#include "runtime_translator.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_decoder(void) {
    x64_insn_t insn;

    // nop
    uint8_t nop_code[] = {0x90};
    assert(x64_decode(nop_code, sizeof(nop_code), 0x1000, &insn) == 1);
    assert(insn.op == X64_OP_NOP);

    // mov rax, 42
    uint8_t mov_imm[] = {0x48, 0xC7, 0xC0, 0x2A, 0x00, 0x00, 0x00};
    assert(x64_decode(mov_imm, sizeof(mov_imm), 0x1000, &insn) == 7);
    assert(insn.op == X64_OP_MOV);
    assert(insn.dst_kind == X64_ARG_REG);
    assert(insn.dst_reg == X64_REG_RAX);
    assert(insn.src_kind == X64_ARG_IMM);
    assert(insn.imm == 42);

    // add rax, rbx
    uint8_t add_rr[] = {0x48, 0x01, 0xD8};
    assert(x64_decode(add_rr, sizeof(add_rr), 0x1000, &insn) == 3);
    assert(insn.op == X64_OP_ADD);
    assert(insn.dst_reg == X64_REG_RAX);
    assert(insn.src_reg == X64_REG_RBX);

    // sub rax, 10
    uint8_t sub_imm[] = {0x48, 0x83, 0xE8, 0x0A};
    assert(x64_decode(sub_imm, sizeof(sub_imm), 0x1000, &insn) == 4);
    assert(insn.op == X64_OP_SUB);
    assert(insn.dst_reg == X64_REG_RAX);
    assert(insn.imm == 10);

    // ret
    uint8_t ret_code[] = {0xC3};
    assert(x64_decode(ret_code, sizeof(ret_code), 0x1000, &insn) == 1);
    assert(insn.op == X64_OP_RET);

    // jmp rel8
    uint8_t jmp_rel8[] = {0xEB, 0x0A};
    assert(x64_decode(jmp_rel8, sizeof(jmp_rel8), 0x1000, &insn) == 2);
    assert(insn.op == X64_OP_JMP);
    assert(insn.imm == 0x1000 + 2 + 0x0A);

    // je rel8
    uint8_t je_rel8[] = {0x74, 0x05};
    assert(x64_decode(je_rel8, sizeof(je_rel8), 0x2000, &insn) == 2);
    assert(insn.op == X64_OP_JCC);
    assert(insn.cond == X64_COND_E);
    assert(insn.imm == 0x2000 + 2 + 0x05);

    // syscall
    uint8_t syscall_code[] = {0x0F, 0x05};
    assert(x64_decode(syscall_code, sizeof(syscall_code), 0x3000, &insn) == 2);
    assert(insn.op == X64_OP_SYSCALL);

    puts("pass: x86_64 decoder basic instructions");
}

static void test_interpreter_execution(void) {
    uint8_t code[128];
    size_t pos = 0;

    // mov rax, 100
    uint8_t insn1[] = {0x48, 0xC7, 0xC0, 0x64, 0x00, 0x00, 0x00};
    memcpy(code + pos, insn1, sizeof(insn1)); pos += sizeof(insn1);

    // mov rbx, 50
    uint8_t insn2[] = {0x48, 0xC7, 0xC3, 0x32, 0x00, 0x00, 0x00};
    memcpy(code + pos, insn2, sizeof(insn2)); pos += sizeof(insn2);

    // add rax rbx
    uint8_t insn3[] = {0x48, 0x01, 0xD8};
    memcpy(code + pos, insn3, sizeof(insn3)); pos += sizeof(insn3);

    // sub rax 25
    uint8_t insn4[] = {0x48, 0x83, 0xE8, 0x19};
    memcpy(code + pos, insn4, sizeof(insn4)); pos += sizeof(insn4);

    // syscall
    uint8_t insn5[] = {0x0F, 0x05};
    memcpy(code + pos, insn5, sizeof(insn5)); pos += sizeof(insn5);

    x64_cpu_t cpu;
    uint8_t stack[4096];
    x64_cpu_init(&cpu, (uint64_t)(uintptr_t)code, (uint64_t)(uintptr_t)(stack + sizeof(stack) - 64));

    int ran = x64_run(&cpu, 100);
    assert(ran == 5);
    assert(cpu.regs[X64_REG_RAX] == 125);
    assert(cpu.regs[X64_REG_RBX] == 50);
    assert(cpu.status == X64_STATUS_SYSCALL);

    puts("pass: x86_64 interpreter execution & register arithmetic");
}

static void test_branches(void) {
    uint8_t code[128];
    size_t pos = 0;

    // mov rax, 10
    uint8_t c1[] = {0x48, 0xC7, 0xC0, 0x0A, 0x00, 0x00, 0x00};
    memcpy(code + pos, c1, sizeof(c1)); pos += sizeof(c1);

    // cmp rax, 10
    uint8_t c2[] = {0x48, 0x83, 0xF8, 0x0A};
    memcpy(code + pos, c2, sizeof(c2)); pos += sizeof(c2);

    // je skip
    uint8_t c3[] = {0x74, 0x07};
    memcpy(code + pos, c3, sizeof(c3)); pos += sizeof(c3);

    // mov rax 99
    uint8_t c4[] = {0x48, 0xC7, 0xC0, 0x63, 0x00, 0x00, 0x00};
    memcpy(code + pos, c4, sizeof(c4)); pos += sizeof(c4);

    // syscall
    uint8_t c5[] = {0x0F, 0x05};
    memcpy(code + pos, c5, sizeof(c5)); pos += sizeof(c5);

    x64_cpu_t cpu;
    uint8_t stack[4096];
    x64_cpu_init(&cpu, (uint64_t)(uintptr_t)code, (uint64_t)(uintptr_t)(stack + sizeof(stack) - 64));

    x64_run(&cpu, 100);
    assert(cpu.regs[X64_REG_RAX] == 10);
    assert(cpu.status == X64_STATUS_SYSCALL);

    puts("pass: conditional branching and flag evaluation");
}

static void test_cache(void) {
    x64_cache_t *cache = x64_cache_create(256);
    assert(cache);

    uint8_t code[] = {0x90, 0x90, 0xC3};
    x64_cpu_t cpu;
    x64_cpu_init(&cpu, (uint64_t)(uintptr_t)code, 0);

    x64_block_t *block = x64_cache_translate(cache, &cpu, (uint64_t)(uintptr_t)code);
    assert(block);
    assert(block->insn_count == 3);
    assert(block->guest_bytes == 3);

    x64_block_t *found = x64_cache_lookup(cache, (uint64_t)(uintptr_t)code);
    assert(found == block);

    x64_cache_destroy(cache);
    puts("pass: basic block translation cache");
}

static void test_arm64_emitter(void) {
    uint8_t buf[16];
    memset(buf, 0, sizeof(buf));

    // mov x0, x1
    assert(arm64_emit_mov_reg(buf, 0, 1) == 4);
    uint32_t val = 0;
    memcpy(&val, buf, 4);
    assert(val == (0xAA0003E0 | (1 << 16) | 0));

    // add x0 x1 16
    assert(arm64_emit_add_imm(buf, 0, 1, 16) == 4);
    memcpy(&val, buf, 4);
    assert(val == (0x91000000 | (16 << 10) | (1 << 5) | 0));

    // ret
    assert(arm64_emit_ret(buf) == 4);
    memcpy(&val, buf, 4);
    assert(val == 0xD65F03C0);

    puts("pass: arm64 machine code emitter primitives");
}

int main(void) {
    test_decoder();
    test_interpreter_execution();
    test_branches();
    test_cache();
    test_arm64_emitter();
    puts("all translator tests passed");
    return 0;
}
