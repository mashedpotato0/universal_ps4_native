#include "runtime_translator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <sched.h>

#define RFLAGS_CF (1ULL << 0)
#define RFLAGS_PF (1ULL << 2)
#define RFLAGS_AF (1ULL << 4)
#define RFLAGS_ZF (1ULL << 6)
#define RFLAGS_SF (1ULL << 7)
#define RFLAGS_OF (1ULL << 11)

static uint64_t total_blocks_translated = 0;
static uint64_t total_insns_interpreted = 0;

void x64_cpu_init(x64_cpu_t *cpu, uint64_t entry, uint64_t stack_top) {
    if (!cpu) return;
    memset(cpu, 0, sizeof(*cpu));
    cpu->rip = entry;
    cpu->regs[X64_REG_RSP] = stack_top;
    cpu->rflags = 0x2;
    cpu->status = X64_STATUS_READY;
}

void x64_cpu_set_tls(x64_cpu_t *cpu, uint64_t fs_base, uint64_t gs_base) {
    if (!cpu) return;
    cpu->fs_base = fs_base;
    cpu->gs_base = gs_base;
}

static inline int parity8(uint8_t v) {
    v ^= v >> 4;
    v ^= v >> 2;
    v ^= v >> 1;
    return (~v) & 1;
}

static inline uint64_t mask_by_size(uint8_t size) {
    if (size == 1) return 0xFFULL;
    if (size == 2) return 0xFFFFULL;
    if (size == 4) return 0xFFFFFFFFULL;
    return 0xFFFFFFFFFFFFFFFFULL;
}

static inline uint64_t sign_bit(uint8_t size) {
    if (size == 1) return 0x80ULL;
    if (size == 2) return 0x8000ULL;
    if (size == 4) return 0x80000000ULL;
    return 0x8000000000000000ULL;
}

static inline __attribute__((unused)) int64_t sign_extend(uint64_t val, uint8_t size) {
    if (size == 1) return (int8_t)val;
    if (size == 2) return (int16_t)val;
    if (size == 4) return (int32_t)val;
    return (int64_t)val;
}

static void update_flags_logic(x64_cpu_t *cpu, uint64_t res, uint8_t size) {
    uint64_t m = mask_by_size(size);
    res &= m;
    cpu->rflags &= ~(RFLAGS_CF | RFLAGS_OF | RFLAGS_AF | RFLAGS_ZF | RFLAGS_SF | RFLAGS_PF);
    if (res == 0) cpu->rflags |= RFLAGS_ZF;
    if (res & sign_bit(size)) cpu->rflags |= RFLAGS_SF;
    if (parity8((uint8_t)res)) cpu->rflags |= RFLAGS_PF;
}

static void update_flags_add(x64_cpu_t *cpu, uint64_t a, uint64_t b, uint64_t res, uint8_t size) {
    uint64_t m = mask_by_size(size);
    uint64_t s = sign_bit(size);
    a &= m;
    b &= m;
    res &= m;
    cpu->rflags &= ~(RFLAGS_CF | RFLAGS_OF | RFLAGS_AF | RFLAGS_ZF | RFLAGS_SF | RFLAGS_PF);
    if (res == 0) cpu->rflags |= RFLAGS_ZF;
    if (res & s) cpu->rflags |= RFLAGS_SF;
    if (parity8((uint8_t)res)) cpu->rflags |= RFLAGS_PF;
    if (res < a) cpu->rflags |= RFLAGS_CF;
    if (((a ^ res) & (b ^ res) & s) != 0) cpu->rflags |= RFLAGS_OF;
}

static void update_flags_sub(x64_cpu_t *cpu, uint64_t a, uint64_t b, uint64_t res, uint8_t size) {
    uint64_t m = mask_by_size(size);
    uint64_t s = sign_bit(size);
    a &= m;
    b &= m;
    res &= m;
    cpu->rflags &= ~(RFLAGS_CF | RFLAGS_OF | RFLAGS_AF | RFLAGS_ZF | RFLAGS_SF | RFLAGS_PF);
    if (res == 0) cpu->rflags |= RFLAGS_ZF;
    if (res & s) cpu->rflags |= RFLAGS_SF;
    if (parity8((uint8_t)res)) cpu->rflags |= RFLAGS_PF;
    if (a < b) cpu->rflags |= RFLAGS_CF;
    if (((a ^ b) & (a ^ res) & s) != 0) cpu->rflags |= RFLAGS_OF;
}

int x64_decode(const uint8_t *code, size_t max_len, uint64_t rip, x64_insn_t *insn) {
    if (!code || max_len == 0 || !insn) return 0;
    memset(insn, 0, sizeof(*insn));

    size_t idx = 0;
    uint8_t rex = 0;
    uint8_t op_override = 0;
    uint8_t seg = 0;
    uint8_t has_lock = 0;
    uint8_t has_rep = 0;
    uint8_t has_repne = 0;

    // decode prefixes
    while (idx < max_len && idx < 15) {
        uint8_t b = code[idx];
        if (b == 0x66) { op_override = 1; idx++; }
        else if (b == 0xF0) { has_lock = 1; idx++; }
        else if (b == 0xF3) { has_rep = 1; idx++; }
        else if (b == 0xF2) { has_repne = 1; idx++; }
        else if (b == 0x64) { seg = 0x64; idx++; } // fs segment
        else if (b == 0x65) { seg = 0x65; idx++; } // gs segment
        else if ((b & 0xF0) == 0x40) { rex = b; idx++; }
        else break;
    }

    if (idx >= max_len) return 0;
    insn->segment_override = seg;
    insn->has_lock = has_lock;

    uint8_t rex_w = (rex >> 3) & 1;
    uint8_t rex_r = (rex >> 2) & 1;
    uint8_t rex_x = (rex >> 1) & 1;
    uint8_t rex_b = rex & 1;

    uint8_t op_size = rex_w ? 8 : (op_override ? 2 : 4);
    insn->op_size = op_size;

    uint8_t op = code[idx++];

    // nop and pause
    if (op == 0x90) {
        insn->op = has_rep ? X64_OP_PAUSE : X64_OP_NOP;
        insn->len = (uint8_t)idx;
        return (int)insn->len;
    }

    // ret
    if (op == 0xC3) {
        insn->op = X64_OP_RET;
        insn->len = (uint8_t)idx;
        return (int)insn->len;
    }

    // push reg
    if ((op & 0xF8) == 0x50) {
        insn->op = X64_OP_PUSH;
        insn->op_size = 8;
        insn->src_kind = X64_ARG_REG;
        insn->src_reg = (x64_reg_t)((op & 7) | (rex_b << 3));
        insn->len = (uint8_t)idx;
        return (int)insn->len;
    }

    // pop reg
    if ((op & 0xF8) == 0x58) {
        insn->op = X64_OP_POP;
        insn->op_size = 8;
        insn->dst_kind = X64_ARG_REG;
        insn->dst_reg = (x64_reg_t)((op & 7) | (rex_b << 3));
        insn->len = (uint8_t)idx;
        return (int)insn->len;
    }

    // mov reg imm
    if ((op & 0xF8) == 0xB8) {
        insn->op = X64_OP_MOV;
        insn->dst_kind = X64_ARG_REG;
        insn->dst_reg = (x64_reg_t)((op & 7) | (rex_b << 3));
        insn->src_kind = X64_ARG_IMM;
        if (rex_w) {
            if (idx + 8 > max_len) return 0;
            uint64_t v = 0;
            memcpy(&v, code + idx, 8);
            insn->imm = v;
            idx += 8;
        } else {
            if (idx + 4 > max_len) return 0;
            uint32_t v = 0;
            memcpy(&v, code + idx, 4);
            insn->imm = v;
            idx += 4;
        }
        insn->len = (uint8_t)idx;
        return (int)insn->len;
    }

    // jmp rel8
    if (op == 0xEB) {
        if (idx >= max_len) return 0;
        int8_t rel = (int8_t)code[idx++];
        insn->op = X64_OP_JMP;
        insn->dst_kind = X64_ARG_IMM;
        insn->imm = (uint64_t)((int64_t)rip + (int64_t)idx + rel);
        insn->len = (uint8_t)idx;
        return (int)insn->len;
    }

    // jmp rel32
    if (op == 0xE9) {
        if (idx + 4 > max_len) return 0;
        int32_t rel = 0;
        memcpy(&rel, code + idx, 4);
        idx += 4;
        insn->op = X64_OP_JMP;
        insn->dst_kind = X64_ARG_IMM;
        insn->imm = (uint64_t)((int64_t)rip + (int64_t)idx + rel);
        insn->len = (uint8_t)idx;
        return (int)insn->len;
    }

    // call rel32
    if (op == 0xE8) {
        if (idx + 4 > max_len) return 0;
        int32_t rel = 0;
        memcpy(&rel, code + idx, 4);
        idx += 4;
        insn->op = X64_OP_CALL;
        insn->dst_kind = X64_ARG_IMM;
        insn->imm = (uint64_t)((int64_t)rip + (int64_t)idx + rel);
        insn->len = (uint8_t)idx;
        return (int)insn->len;
    }

    // jcc rel8
    if ((op & 0xF0) == 0x70) {
        if (idx >= max_len) return 0;
        int8_t rel = (int8_t)code[idx++];
        insn->op = X64_OP_JCC;
        insn->cond = (x64_cond_t)(op & 0xF);
        insn->dst_kind = X64_ARG_IMM;
        insn->imm = (uint64_t)((int64_t)rip + (int64_t)idx + rel);
        insn->len = (uint8_t)idx;
        return (int)insn->len;
    }

    uint8_t is_two_byte = 0;
    uint8_t op2 = 0;

    // two-byte opcodes
    if (op == 0x0F) {
        if (idx >= max_len) return 0;
        op2 = code[idx++];

        // syscall
        if (op2 == 0x05) {
            insn->op = X64_OP_SYSCALL;
            insn->len = (uint8_t)idx;
            return (int)insn->len;
        }

        // ud2
        if (op2 == 0x0B) {
            insn->op = X64_OP_UD2;
            insn->len = (uint8_t)idx;
            return (int)insn->len;
        }

        // jcc rel32
        if ((op2 & 0xF0) == 0x80) {
            if (idx + 4 > max_len) return 0;
            int32_t rel = 0;
            memcpy(&rel, code + idx, 4);
            idx += 4;
            insn->op = X64_OP_JCC;
            insn->cond = (x64_cond_t)(op2 & 0xF);
            insn->dst_kind = X64_ARG_IMM;
            insn->imm = (uint64_t)((int64_t)rip + (int64_t)idx + rel);
            insn->len = (uint8_t)idx;
            return (int)insn->len;
        }

        is_two_byte = 1;
    }

    // decode modrm
    if (idx >= max_len) return 0;
    uint8_t modrm = code[idx++];
    uint8_t mod = (modrm >> 6) & 3;
    uint8_t reg = ((modrm >> 3) & 7) | (rex_r << 3);
    uint8_t rm = (modrm & 7) | (rex_b << 3);

    insn->dst_reg = (x64_reg_t)reg;
    insn->src_reg = (x64_reg_t)rm;

    // decode modrm addressing
    if (mod != 3) {
        insn->mem_base = (x64_reg_t)rm;
        if ((rm & 7) == 4) { // sib byte
            if (idx >= max_len) return 0;
            uint8_t sib = code[idx++];
            uint8_t scale = 1 << ((sib >> 6) & 3);
            uint8_t index = ((sib >> 3) & 7) | (rex_x << 3);
            uint8_t base = (sib & 7) | (rex_b << 3);
            insn->mem_scale = scale;
            insn->mem_base = (x64_reg_t)base;
            if (index != 4) {
                insn->has_index = 1;
                insn->mem_index = (x64_reg_t)index;
            }
            if (mod == 0 && (base & 7) == 5) {
                if (idx + 4 > max_len) return 0;
                int32_t disp = 0;
                memcpy(&disp, code + idx, 4);
                idx += 4;
                insn->mem_disp = disp;
                insn->mem_base = (x64_reg_t)0xFF;
            }
        } else if (mod == 0 && (rm & 7) == 5) { // rip relative
            if (idx + 4 > max_len) return 0;
            int32_t disp = 0;
            memcpy(&disp, code + idx, 4);
            idx += 4;
            insn->is_rip_rel = 1;
            insn->mem_disp = disp;
        }

        if (mod == 1) {
            if (idx >= max_len) return 0;
            insn->mem_disp = (int8_t)code[idx++];
        } else if (mod == 2) {
            if (idx + 4 > max_len) return 0;
            int32_t disp = 0;
            memcpy(&disp, code + idx, 4);
            idx += 4;
            insn->mem_disp = disp;
        }
    }

    // decode opcodes using modrm
    if (is_two_byte) {
        // vector and sse opcodes
        if (op2 == 0x57) { // xorps or pxor
            insn->op = op_override ? X64_OP_PXOR : X64_OP_XORPS;
            insn->is_vec = 1;
            insn->dst_kind = X64_ARG_REG;
            insn->dst_reg = (x64_reg_t)reg;
            insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->src_reg = (x64_reg_t)rm;
        } else if (op2 == 0x54) { // andps
            insn->op = X64_OP_ANDPS;
            insn->is_vec = 1;
            insn->dst_kind = X64_ARG_REG;
            insn->dst_reg = (x64_reg_t)reg;
            insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->src_reg = (x64_reg_t)rm;
        } else if (op2 == 0x56) { // orps
            insn->op = X64_OP_ORPS;
            insn->is_vec = 1;
            insn->dst_kind = X64_ARG_REG;
            insn->dst_reg = (x64_reg_t)reg;
            insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->src_reg = (x64_reg_t)rm;
        } else if (op2 == 0xEF) { // pxor
            insn->op = X64_OP_PXOR;
            insn->is_vec = 1;
            insn->dst_kind = X64_ARG_REG;
            insn->dst_reg = (x64_reg_t)reg;
            insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->src_reg = (x64_reg_t)rm;
        } else if (op2 == 0x58) { // addps/addss/addsd
            if (has_rep) insn->op = X64_OP_ADDSS;
            else if (has_repne) insn->op = X64_OP_ADDSD;
            else insn->op = X64_OP_ADDPS;
            insn->is_vec = 1;
            insn->dst_kind = X64_ARG_REG;
            insn->dst_reg = (x64_reg_t)reg;
            insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->src_reg = (x64_reg_t)rm;
        } else if (op2 == 0x5C) { // subps/subss/subsd
            if (has_rep) insn->op = X64_OP_SUBSS;
            else if (has_repne) insn->op = X64_OP_SUBSD;
            else insn->op = X64_OP_SUBPS;
            insn->is_vec = 1;
            insn->dst_kind = X64_ARG_REG;
            insn->dst_reg = (x64_reg_t)reg;
            insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->src_reg = (x64_reg_t)rm;
        } else if (op2 == 0x59) { // mulps/mulss/mulsd
            if (has_rep) insn->op = X64_OP_MULSS;
            else if (has_repne) insn->op = X64_OP_MULSD;
            else insn->op = X64_OP_MULPS;
            insn->is_vec = 1;
            insn->dst_kind = X64_ARG_REG;
            insn->dst_reg = (x64_reg_t)reg;
            insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->src_reg = (x64_reg_t)rm;
        } else if (op2 == 0x5E) { // divps/divss/divsd
            if (has_rep) insn->op = X64_OP_DIVSS;
            else if (has_repne) insn->op = X64_OP_DIVSD;
            else insn->op = X64_OP_DIVPS;
            insn->is_vec = 1;
            insn->dst_kind = X64_ARG_REG;
            insn->dst_reg = (x64_reg_t)reg;
            insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->src_reg = (x64_reg_t)rm;
        } else if (op2 == 0x10) { // movups/movss/movsd
            if (has_rep) insn->op = X64_OP_MOVSS;
            else if (has_repne) insn->op = X64_OP_MOVSD;
            else insn->op = X64_OP_MOVUPS;
            insn->is_vec = 1;
            insn->dst_kind = X64_ARG_REG;
            insn->dst_reg = (x64_reg_t)reg;
            insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->src_reg = (x64_reg_t)rm;
        } else if (op2 == 0x11) { // store movups/movss/movsd
            if (has_rep) insn->op = X64_OP_MOVSS;
            else if (has_repne) insn->op = X64_OP_MOVSD;
            else insn->op = X64_OP_MOVUPS;
            insn->is_vec = 1;
            insn->dst_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->dst_reg = (x64_reg_t)rm;
            insn->src_kind = X64_ARG_REG;
            insn->src_reg = (x64_reg_t)reg;
        } else if (op2 == 0x28) { // movaps
            insn->op = X64_OP_MOVAPS;
            insn->is_vec = 1;
            insn->dst_kind = X64_ARG_REG;
            insn->dst_reg = (x64_reg_t)reg;
            insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->src_reg = (x64_reg_t)rm;
        } else if (op2 == 0x29) { // movaps store
            insn->op = X64_OP_MOVAPS;
            insn->is_vec = 1;
            insn->dst_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->dst_reg = (x64_reg_t)rm;
            insn->src_kind = X64_ARG_REG;
            insn->src_reg = (x64_reg_t)reg;
        } else if (op2 == 0x2E) { // ucomiss/ucomisd
            insn->op = op_override ? X64_OP_UCOMISD : X64_OP_UCOMISS;
            insn->is_vec = 1;
            insn->dst_kind = X64_ARG_REG;
            insn->dst_reg = (x64_reg_t)reg;
            insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->src_reg = (x64_reg_t)rm;
        } else if (op2 == 0x2F) { // comiss/comisd
            insn->op = op_override ? X64_OP_COMISD : X64_OP_COMISS;
            insn->is_vec = 1;
            insn->dst_kind = X64_ARG_REG;
            insn->dst_reg = (x64_reg_t)reg;
            insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->src_reg = (x64_reg_t)rm;
        } else if (op2 == 0x2A) { // cvtsi2ss/cvtsi2sd
            insn->op = has_repne ? X64_OP_CVTSI2SD : X64_OP_CVTSI2SS;
            insn->is_vec = 1;
            insn->dst_kind = X64_ARG_REG;
            insn->dst_reg = (x64_reg_t)reg;
            insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->src_reg = (x64_reg_t)rm;
        } else if (op2 == 0x2C) { // cvttss2si/cvttsd2si
            insn->op = has_repne ? X64_OP_CVTTSD2SI : X64_OP_CVTTSS2SI;
            insn->dst_kind = X64_ARG_REG;
            insn->dst_reg = (x64_reg_t)reg;
            insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->src_reg = (x64_reg_t)rm;
        } else if (op2 == 0xAE) { // fences
            uint8_t grp = (modrm >> 3) & 7;
            if (grp == 5) insn->op = X64_OP_LFENCE;
            else if (grp == 6) insn->op = X64_OP_MFENCE;
            else if (grp == 7) insn->op = X64_OP_SFENCE;
            else return 0;
        } else if (op2 == 0xB0 || op2 == 0xB1) { // cmpxchg
            insn->op = X64_OP_CMPXCHG;
            insn->op_size = (op2 == 0xB0) ? 1 : (rex_w ? 8 : (op_override ? 2 : 4));
            insn->dst_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->dst_reg = (x64_reg_t)rm;
            insn->src_kind = X64_ARG_REG;
            insn->src_reg = (x64_reg_t)reg;
        } else if (op2 == 0xC0 || op2 == 0xC1) { // xadd
            insn->op = X64_OP_XADD;
            insn->op_size = (op2 == 0xC0) ? 1 : (rex_w ? 8 : (op_override ? 2 : 4));
            insn->dst_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->dst_reg = (x64_reg_t)rm;
            insn->src_kind = X64_ARG_REG;
            insn->src_reg = (x64_reg_t)reg;
        } else {
            return 0;
        }
    } else if (op == 0x86 || op == 0x87) { // xchg
        insn->op = X64_OP_XCHG;
        insn->op_size = (op == 0x86) ? 1 : (rex_w ? 8 : (op_override ? 2 : 4));
        insn->dst_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
        insn->dst_reg = (x64_reg_t)rm;
        insn->src_kind = X64_ARG_REG;
        insn->src_reg = (x64_reg_t)reg;
    } else if (op == 0x89) { // mov rm, r
        insn->op = X64_OP_MOV;
        insn->dst_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
        insn->dst_reg = (x64_reg_t)rm;
        insn->src_kind = X64_ARG_REG;
        insn->src_reg = (x64_reg_t)reg;
    } else if (op == 0x8B) { // mov r, rm
        insn->op = X64_OP_MOV;
        insn->dst_kind = X64_ARG_REG;
        insn->dst_reg = (x64_reg_t)reg;
        insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
        insn->src_reg = (x64_reg_t)rm;
    } else if (op == 0x8D) { // lea r, m
        insn->op = X64_OP_LEA;
        insn->dst_kind = X64_ARG_REG;
        insn->dst_reg = (x64_reg_t)reg;
        insn->src_kind = X64_ARG_MEM;
    } else if (op == 0x01) { // add rm, r
        insn->op = X64_OP_ADD;
        insn->dst_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
        insn->dst_reg = (x64_reg_t)rm;
        insn->src_kind = X64_ARG_REG;
        insn->src_reg = (x64_reg_t)reg;
    } else if (op == 0x03) { // add r, rm
        insn->op = X64_OP_ADD;
        insn->dst_kind = X64_ARG_REG;
        insn->dst_reg = (x64_reg_t)reg;
        insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
        insn->src_reg = (x64_reg_t)rm;
    } else if (op == 0x29) { // sub rm, r
        insn->op = X64_OP_SUB;
        insn->dst_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
        insn->dst_reg = (x64_reg_t)rm;
        insn->src_kind = X64_ARG_REG;
        insn->src_reg = (x64_reg_t)reg;
    } else if (op == 0x2B) { // sub r, rm
        insn->op = X64_OP_SUB;
        insn->dst_kind = X64_ARG_REG;
        insn->dst_reg = (x64_reg_t)reg;
        insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
        insn->src_reg = (x64_reg_t)rm;
    } else if (op == 0x39) { // cmp rm, r
        insn->op = X64_OP_CMP;
        insn->dst_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
        insn->dst_reg = (x64_reg_t)rm;
        insn->src_kind = X64_ARG_REG;
        insn->src_reg = (x64_reg_t)reg;
    } else if (op == 0x3B) { // cmp r, rm
        insn->op = X64_OP_CMP;
        insn->dst_kind = X64_ARG_REG;
        insn->dst_reg = (x64_reg_t)reg;
        insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
        insn->src_reg = (x64_reg_t)rm;
    } else if (op == 0x31) { // xor rm, r
        insn->op = X64_OP_XOR;
        insn->dst_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
        insn->dst_reg = (x64_reg_t)rm;
        insn->src_kind = X64_ARG_REG;
        insn->src_reg = (x64_reg_t)reg;
    } else if (op == 0x85) { // test rm, r
        insn->op = X64_OP_TEST;
        insn->dst_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
        insn->dst_reg = (x64_reg_t)rm;
        insn->src_kind = X64_ARG_REG;
        insn->src_reg = (x64_reg_t)reg;
    } else if (op == 0xC7 && ((modrm >> 3) & 7) == 0) { // mov rm, imm32
        insn->op = X64_OP_MOV;
        insn->dst_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
        insn->dst_reg = (x64_reg_t)rm;
        insn->src_kind = X64_ARG_IMM;
        if (idx + 4 > max_len) return 0;
        int32_t imm32 = 0;
        memcpy(&imm32, code + idx, 4);
        idx += 4;
        insn->imm = (uint64_t)(int64_t)imm32;
    } else if (op == 0x83 || op == 0x81) { // group 1
        uint8_t grp = (modrm >> 3) & 7;
        insn->dst_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
        insn->dst_reg = (x64_reg_t)rm;
        insn->src_kind = X64_ARG_IMM;

        if (op == 0x83) {
            if (idx >= max_len) return 0;
            insn->imm = (uint64_t)(int64_t)(int8_t)code[idx++];
        } else {
            if (idx + 4 > max_len) return 0;
            int32_t imm32 = 0;
            memcpy(&imm32, code + idx, 4);
            idx += 4;
            insn->imm = (uint64_t)(int64_t)imm32;
        }

        if (grp == 0) insn->op = X64_OP_ADD;
        else if (grp == 1) insn->op = X64_OP_OR;
        else if (grp == 4) insn->op = X64_OP_AND;
        else if (grp == 5) insn->op = X64_OP_SUB;
        else if (grp == 6) insn->op = X64_OP_XOR;
        else if (grp == 7) insn->op = X64_OP_CMP;
        else return 0;
    } else if (op == 0xFF) { // group 5
        uint8_t grp = (modrm >> 3) & 7;
        if (grp == 2) { // call rm
            insn->op = X64_OP_CALL;
            insn->dst_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->dst_reg = (x64_reg_t)rm;
        } else if (grp == 4) { // jmp rm
            insn->op = X64_OP_JMP;
            insn->dst_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->dst_reg = (x64_reg_t)rm;
        } else if (grp == 6) { // push rm
            insn->op = X64_OP_PUSH;
            insn->op_size = 8;
            insn->src_kind = (mod == 3) ? X64_ARG_REG : X64_ARG_MEM;
            insn->src_reg = (x64_reg_t)rm;
        } else {
            return 0;
        }
    } else {
        return 0;
    }

    insn->len = (uint8_t)idx;
    return (int)insn->len;
}

static uint64_t resolve_mem_addr(x64_cpu_t *cpu, const x64_insn_t *insn) {
    if (insn->is_rip_rel) {
        return (uint64_t)((int64_t)cpu->rip + (int64_t)insn->len + insn->mem_disp);
    }
    uint64_t addr = 0;
    if (insn->segment_override == 0x64) addr += cpu->fs_base;
    else if (insn->segment_override == 0x65) addr += cpu->gs_base;

    if (insn->mem_base != (x64_reg_t)0xFF) {
        addr += cpu->regs[insn->mem_base];
    }
    if (insn->has_index) {
        addr += cpu->regs[insn->mem_index] * insn->mem_scale;
    }
    addr += (uint64_t)(int64_t)insn->mem_disp;
    return addr;
}

static uint64_t read_mem(uint64_t addr, uint8_t size) {
    uint64_t v = 0;
    if (size == 1) v = *(uint8_t *)(uintptr_t)addr;
    else if (size == 2) v = *(uint16_t *)(uintptr_t)addr;
    else if (size == 4) v = *(uint32_t *)(uintptr_t)addr;
    else if (size == 8) v = *(uint64_t *)(uintptr_t)addr;
    return v;
}

static void write_mem(uint64_t addr, uint64_t val, uint8_t size) {
    if (size == 1) *(uint8_t *)(uintptr_t)addr = (uint8_t)val;
    else if (size == 2) *(uint16_t *)(uintptr_t)addr = (uint16_t)val;
    else if (size == 4) *(uint32_t *)(uintptr_t)addr = (uint32_t)val;
    else if (size == 8) *(uint64_t *)(uintptr_t)addr = val;
}

static inline void read_mem128(uint64_t addr, x64_vec128_t *vec) {
    memcpy(vec, (const void *)(uintptr_t)addr, 16);
}

static inline void write_mem128(uint64_t addr, const x64_vec128_t *vec) {
    memcpy((void *)(uintptr_t)addr, vec, 16);
}

static int eval_cond(uint64_t rflags, x64_cond_t cond) {
    int cf = (rflags & RFLAGS_CF) != 0;
    int zf = (rflags & RFLAGS_ZF) != 0;
    int sf = (rflags & RFLAGS_SF) != 0;
    int of = (rflags & RFLAGS_OF) != 0;
    int pf = (rflags & RFLAGS_PF) != 0;

    switch (cond) {
        case X64_COND_O:  return of;
        case X64_COND_NO: return !of;
        case X64_COND_B:  return cf;
        case X64_COND_AE: return !cf;
        case X64_COND_E:  return zf;
        case X64_COND_NE: return !zf;
        case X64_COND_BE: return cf || zf;
        case X64_COND_A:  return !cf && !zf;
        case X64_COND_S:  return sf;
        case X64_COND_NS: return !sf;
        case X64_COND_P:  return pf;
        case X64_COND_NP: return !pf;
        case X64_COND_L:  return sf != of;
        case X64_COND_GE: return sf == of;
        case X64_COND_LE: return zf || (sf != of);
        case X64_COND_G:  return !zf && (sf == of);
    }
    return 0;
}

int x64_step(x64_cpu_t *cpu) {
    if (!cpu || (cpu->status != X64_STATUS_READY && cpu->status != X64_STATUS_RUNNING)) return 0;
    cpu->status = X64_STATUS_RUNNING;

    x64_insn_t insn;
    int decoded = x64_decode((const uint8_t *)(uintptr_t)cpu->rip, 15, cpu->rip, &insn);
    if (!decoded) {
        cpu->status = X64_STATUS_INVALID_INSN;
        return 0;
    }

    uint64_t next_rip = cpu->rip + insn.len;
    uint8_t size = insn.op_size;

    // execute decoded opcode
    switch (insn.op) {
        case X64_OP_NOP:
            break;

        case X64_OP_LEA: {
            uint64_t addr = resolve_mem_addr(cpu, &insn);
            cpu->regs[insn.dst_reg] = (size == 4) ? (uint32_t)addr : addr;
            break;
        }

        case X64_OP_MOV: {
            uint64_t src_val = 0;
            if (insn.src_kind == X64_ARG_REG) src_val = cpu->regs[insn.src_reg] & mask_by_size(size);
            else if (insn.src_kind == X64_ARG_MEM) src_val = read_mem(resolve_mem_addr(cpu, &insn), size);
            else if (insn.src_kind == X64_ARG_IMM) src_val = insn.imm & mask_by_size(size);

            if (insn.dst_kind == X64_ARG_REG) {
                if (size == 4) cpu->regs[insn.dst_reg] = (uint32_t)src_val; // zero extend
                else if (size == 8) cpu->regs[insn.dst_reg] = src_val;
                else {
                    uint64_t m = mask_by_size(size);
                    cpu->regs[insn.dst_reg] = (cpu->regs[insn.dst_reg] & ~m) | (src_val & m);
                }
            } else if (insn.dst_kind == X64_ARG_MEM) {
                write_mem(resolve_mem_addr(cpu, &insn), src_val, size);
            }
            break;
        }

        case X64_OP_ADD:
        case X64_OP_SUB:
        case X64_OP_CMP:
        case X64_OP_AND:
        case X64_OP_OR:
        case X64_OP_XOR:
        case X64_OP_TEST: {
            uint64_t dst_val = 0;
            uint64_t dst_addr = 0;
            if (insn.dst_kind == X64_ARG_REG) dst_val = cpu->regs[insn.dst_reg] & mask_by_size(size);
            else {
                dst_addr = resolve_mem_addr(cpu, &insn);
                dst_val = read_mem(dst_addr, size);
            }

            uint64_t src_val = 0;
            if (insn.src_kind == X64_ARG_REG) src_val = cpu->regs[insn.src_reg] & mask_by_size(size);
            else if (insn.src_kind == X64_ARG_MEM) src_val = read_mem(resolve_mem_addr(cpu, &insn), size);
            else if (insn.src_kind == X64_ARG_IMM) src_val = insn.imm & mask_by_size(size);

            uint64_t res = 0;
            if (insn.op == X64_OP_ADD) {
                res = dst_val + src_val;
                update_flags_add(cpu, dst_val, src_val, res, size);
            } else if (insn.op == X64_OP_SUB || insn.op == X64_OP_CMP) {
                res = dst_val - src_val;
                update_flags_sub(cpu, dst_val, src_val, res, size);
            } else if (insn.op == X64_OP_AND || insn.op == X64_OP_TEST) {
                res = dst_val & src_val;
                update_flags_logic(cpu, res, size);
            } else if (insn.op == X64_OP_OR) {
                res = dst_val | src_val;
                update_flags_logic(cpu, res, size);
            } else if (insn.op == X64_OP_XOR) {
                res = dst_val ^ src_val;
                update_flags_logic(cpu, res, size);
            }

            if (insn.op != X64_OP_CMP && insn.op != X64_OP_TEST) {
                if (insn.dst_kind == X64_ARG_REG) {
                    if (size == 4) cpu->regs[insn.dst_reg] = (uint32_t)res;
                    else if (size == 8) cpu->regs[insn.dst_reg] = res;
                    else {
                        uint64_t m = mask_by_size(size);
                        cpu->regs[insn.dst_reg] = (cpu->regs[insn.dst_reg] & ~m) | (res & m);
                    }
                } else {
                    write_mem(dst_addr, res, size);
                }
            }
            break;
        }

        case X64_OP_PUSH: {
            uint64_t val = 0;
            if (insn.src_kind == X64_ARG_REG) val = cpu->regs[insn.src_reg];
            else if (insn.src_kind == X64_ARG_MEM) val = read_mem(resolve_mem_addr(cpu, &insn), 8);
            else if (insn.src_kind == X64_ARG_IMM) val = insn.imm;
            cpu->regs[X64_REG_RSP] -= 8;
            write_mem(cpu->regs[X64_REG_RSP], val, 8);
            break;
        }

        case X64_OP_POP: {
            uint64_t val = read_mem(cpu->regs[X64_REG_RSP], 8);
            cpu->regs[X64_REG_RSP] += 8;
            cpu->regs[insn.dst_reg] = val;
            break;
        }

        case X64_OP_CALL: {
            uint64_t target = 0;
            if (insn.dst_kind == X64_ARG_IMM) target = insn.imm;
            else if (insn.dst_kind == X64_ARG_REG) target = cpu->regs[insn.dst_reg];
            else if (insn.dst_kind == X64_ARG_MEM) target = read_mem(resolve_mem_addr(cpu, &insn), 8);

            cpu->regs[X64_REG_RSP] -= 8;
            write_mem(cpu->regs[X64_REG_RSP], next_rip, 8);
            next_rip = target;
            break;
        }

        case X64_OP_RET: {
            next_rip = read_mem(cpu->regs[X64_REG_RSP], 8);
            cpu->regs[X64_REG_RSP] += 8;
            break;
        }

        case X64_OP_JMP: {
            if (insn.dst_kind == X64_ARG_IMM) next_rip = insn.imm;
            else if (insn.dst_kind == X64_ARG_REG) next_rip = cpu->regs[insn.dst_reg];
            else if (insn.dst_kind == X64_ARG_MEM) next_rip = read_mem(resolve_mem_addr(cpu, &insn), 8);
            break;
        }

        case X64_OP_JCC: {
            if (eval_cond(cpu->rflags, insn.cond)) {
                next_rip = insn.imm;
            }
            break;
        }

        case X64_OP_XORPS:
        case X64_OP_PXOR: {
            x64_vec128_t src = {0};
            if (insn.src_kind == X64_ARG_REG) src = cpu->xmm[insn.src_reg];
            else read_mem128(resolve_mem_addr(cpu, &insn), &src);
            cpu->xmm[insn.dst_reg].u64[0] ^= src.u64[0];
            cpu->xmm[insn.dst_reg].u64[1] ^= src.u64[1];
            break;
        }

        case X64_OP_ANDPS: {
            x64_vec128_t src = {0};
            if (insn.src_kind == X64_ARG_REG) src = cpu->xmm[insn.src_reg];
            else read_mem128(resolve_mem_addr(cpu, &insn), &src);
            cpu->xmm[insn.dst_reg].u64[0] &= src.u64[0];
            cpu->xmm[insn.dst_reg].u64[1] &= src.u64[1];
            break;
        }

        case X64_OP_ORPS: {
            x64_vec128_t src = {0};
            if (insn.src_kind == X64_ARG_REG) src = cpu->xmm[insn.src_reg];
            else read_mem128(resolve_mem_addr(cpu, &insn), &src);
            cpu->xmm[insn.dst_reg].u64[0] |= src.u64[0];
            cpu->xmm[insn.dst_reg].u64[1] |= src.u64[1];
            break;
        }

        case X64_OP_ADDPS: {
            x64_vec128_t src = {0};
            if (insn.src_kind == X64_ARG_REG) src = cpu->xmm[insn.src_reg];
            else read_mem128(resolve_mem_addr(cpu, &insn), &src);
            for (int i = 0; i < 4; i++) cpu->xmm[insn.dst_reg].f32[i] += src.f32[i];
            break;
        }

        case X64_OP_SUBPS: {
            x64_vec128_t src = {0};
            if (insn.src_kind == X64_ARG_REG) src = cpu->xmm[insn.src_reg];
            else read_mem128(resolve_mem_addr(cpu, &insn), &src);
            for (int i = 0; i < 4; i++) cpu->xmm[insn.dst_reg].f32[i] -= src.f32[i];
            break;
        }

        case X64_OP_MULPS: {
            x64_vec128_t src = {0};
            if (insn.src_kind == X64_ARG_REG) src = cpu->xmm[insn.src_reg];
            else read_mem128(resolve_mem_addr(cpu, &insn), &src);
            for (int i = 0; i < 4; i++) cpu->xmm[insn.dst_reg].f32[i] *= src.f32[i];
            break;
        }

        case X64_OP_DIVPS: {
            x64_vec128_t src = {0};
            if (insn.src_kind == X64_ARG_REG) src = cpu->xmm[insn.src_reg];
            else read_mem128(resolve_mem_addr(cpu, &insn), &src);
            for (int i = 0; i < 4; i++) {
                if (src.f32[i] != 0.0f) cpu->xmm[insn.dst_reg].f32[i] /= src.f32[i];
            }
            break;
        }

        case X64_OP_ADDSS: {
            float src = (insn.src_kind == X64_ARG_REG) ? cpu->xmm[insn.src_reg].f32[0] : *(const float *)(uintptr_t)resolve_mem_addr(cpu, &insn);
            cpu->xmm[insn.dst_reg].f32[0] += src;
            break;
        }

        case X64_OP_SUBSS: {
            float src = (insn.src_kind == X64_ARG_REG) ? cpu->xmm[insn.src_reg].f32[0] : *(const float *)(uintptr_t)resolve_mem_addr(cpu, &insn);
            cpu->xmm[insn.dst_reg].f32[0] -= src;
            break;
        }

        case X64_OP_MULSS: {
            float src = (insn.src_kind == X64_ARG_REG) ? cpu->xmm[insn.src_reg].f32[0] : *(const float *)(uintptr_t)resolve_mem_addr(cpu, &insn);
            cpu->xmm[insn.dst_reg].f32[0] *= src;
            break;
        }

        case X64_OP_DIVSS: {
            float src = (insn.src_kind == X64_ARG_REG) ? cpu->xmm[insn.src_reg].f32[0] : *(const float *)(uintptr_t)resolve_mem_addr(cpu, &insn);
            if (src != 0.0f) cpu->xmm[insn.dst_reg].f32[0] /= src;
            break;
        }

        case X64_OP_ADDSD: {
            double src = (insn.src_kind == X64_ARG_REG) ? cpu->xmm[insn.src_reg].f64[0] : *(const double *)(uintptr_t)resolve_mem_addr(cpu, &insn);
            cpu->xmm[insn.dst_reg].f64[0] += src;
            break;
        }

        case X64_OP_SUBSD: {
            double src = (insn.src_kind == X64_ARG_REG) ? cpu->xmm[insn.src_reg].f64[0] : *(const double *)(uintptr_t)resolve_mem_addr(cpu, &insn);
            cpu->xmm[insn.dst_reg].f64[0] -= src;
            break;
        }

        case X64_OP_MULSD: {
            double src = (insn.src_kind == X64_ARG_REG) ? cpu->xmm[insn.src_reg].f64[0] : *(const double *)(uintptr_t)resolve_mem_addr(cpu, &insn);
            cpu->xmm[insn.dst_reg].f64[0] *= src;
            break;
        }

        case X64_OP_DIVSD: {
            double src = (insn.src_kind == X64_ARG_REG) ? cpu->xmm[insn.src_reg].f64[0] : *(const double *)(uintptr_t)resolve_mem_addr(cpu, &insn);
            if (src != 0.0) cpu->xmm[insn.dst_reg].f64[0] /= src;
            break;
        }

        case X64_OP_MOVAPS:
        case X64_OP_MOVUPS: {
            if (insn.dst_kind == X64_ARG_REG && insn.src_kind == X64_ARG_REG) {
                cpu->xmm[insn.dst_reg] = cpu->xmm[insn.src_reg];
            } else if (insn.dst_kind == X64_ARG_REG) {
                read_mem128(resolve_mem_addr(cpu, &insn), &cpu->xmm[insn.dst_reg]);
            } else {
                write_mem128(resolve_mem_addr(cpu, &insn), &cpu->xmm[insn.src_reg]);
            }
            break;
        }

        case X64_OP_MOVSS: {
            if (insn.dst_kind == X64_ARG_REG && insn.src_kind == X64_ARG_REG) {
                cpu->xmm[insn.dst_reg].u32[0] = cpu->xmm[insn.src_reg].u32[0];
            } else if (insn.dst_kind == X64_ARG_REG) {
                cpu->xmm[insn.dst_reg].u32[0] = (uint32_t)read_mem(resolve_mem_addr(cpu, &insn), 4);
                cpu->xmm[insn.dst_reg].u32[1] = 0;
                cpu->xmm[insn.dst_reg].u32[2] = 0;
                cpu->xmm[insn.dst_reg].u32[3] = 0;
            } else {
                write_mem(resolve_mem_addr(cpu, &insn), cpu->xmm[insn.src_reg].u32[0], 4);
            }
            break;
        }

        case X64_OP_MOVSD: {
            if (insn.dst_kind == X64_ARG_REG && insn.src_kind == X64_ARG_REG) {
                cpu->xmm[insn.dst_reg].u64[0] = cpu->xmm[insn.src_reg].u64[0];
            } else if (insn.dst_kind == X64_ARG_REG) {
                cpu->xmm[insn.dst_reg].u64[0] = read_mem(resolve_mem_addr(cpu, &insn), 8);
                cpu->xmm[insn.dst_reg].u64[1] = 0;
            } else {
                write_mem(resolve_mem_addr(cpu, &insn), cpu->xmm[insn.src_reg].u64[0], 8);
            }
            break;
        }

        case X64_OP_UCOMISS:
        case X64_OP_COMISS: {
            float a = cpu->xmm[insn.dst_reg].f32[0];
            float b = (insn.src_kind == X64_ARG_REG) ? cpu->xmm[insn.src_reg].f32[0] : *(const float *)(uintptr_t)resolve_mem_addr(cpu, &insn);
            cpu->rflags &= ~(RFLAGS_ZF | RFLAGS_PF | RFLAGS_CF);
            if (a == b) cpu->rflags |= RFLAGS_ZF;
            else if (a < b) cpu->rflags |= RFLAGS_CF;
            break;
        }

        case X64_OP_UCOMISD:
        case X64_OP_COMISD: {
            double a = cpu->xmm[insn.dst_reg].f64[0];
            double b = (insn.src_kind == X64_ARG_REG) ? cpu->xmm[insn.src_reg].f64[0] : *(const double *)(uintptr_t)resolve_mem_addr(cpu, &insn);
            cpu->rflags &= ~(RFLAGS_ZF | RFLAGS_PF | RFLAGS_CF);
            if (a == b) cpu->rflags |= RFLAGS_ZF;
            else if (a < b) cpu->rflags |= RFLAGS_CF;
            break;
        }

        case X64_OP_CVTSI2SS: {
            uint64_t val = (insn.src_kind == X64_ARG_REG) ? cpu->regs[insn.src_reg] : read_mem(resolve_mem_addr(cpu, &insn), size);
            if (size == 4) cpu->xmm[insn.dst_reg].f32[0] = (float)(int32_t)val;
            else cpu->xmm[insn.dst_reg].f32[0] = (float)(int64_t)val;
            break;
        }

        case X64_OP_CVTSI2SD: {
            uint64_t val = (insn.src_kind == X64_ARG_REG) ? cpu->regs[insn.src_reg] : read_mem(resolve_mem_addr(cpu, &insn), size);
            if (size == 4) cpu->xmm[insn.dst_reg].f64[0] = (double)(int32_t)val;
            else cpu->xmm[insn.dst_reg].f64[0] = (double)(int64_t)val;
            break;
        }

        case X64_OP_CVTTSS2SI: {
            float val = (insn.src_kind == X64_ARG_REG) ? cpu->xmm[insn.src_reg].f32[0] : *(const float *)(uintptr_t)resolve_mem_addr(cpu, &insn);
            if (size == 4) cpu->regs[insn.dst_reg] = (uint32_t)(int32_t)val;
            else cpu->regs[insn.dst_reg] = (uint64_t)(int64_t)val;
            break;
        }

        case X64_OP_CVTTSD2SI: {
            double val = (insn.src_kind == X64_ARG_REG) ? cpu->xmm[insn.src_reg].f64[0] : *(const double *)(uintptr_t)resolve_mem_addr(cpu, &insn);
            if (size == 4) cpu->regs[insn.dst_reg] = (uint32_t)(int32_t)val;
            else cpu->regs[insn.dst_reg] = (uint64_t)(int64_t)val;
            break;
        }

        case X64_OP_CMPXCHG: {
            uint64_t rax_val = cpu->regs[X64_REG_RAX] & mask_by_size(size);
            uint64_t src_val = cpu->regs[insn.src_reg] & mask_by_size(size);
            if (insn.dst_kind == X64_ARG_MEM) {
                uintptr_t ptr = (uintptr_t)resolve_mem_addr(cpu, &insn);
                if (size == 8) {
                    uint64_t expected = rax_val;
                    if (__atomic_compare_exchange_n((uint64_t *)ptr, &expected, src_val, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
                        cpu->rflags |= RFLAGS_ZF;
                    } else {
                        cpu->rflags &= ~RFLAGS_ZF;
                        cpu->regs[X64_REG_RAX] = expected;
                    }
                } else if (size == 4) {
                    uint32_t expected = (uint32_t)rax_val;
                    if (__atomic_compare_exchange_n((uint32_t *)ptr, &expected, (uint32_t)src_val, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
                        cpu->rflags |= RFLAGS_ZF;
                    } else {
                        cpu->rflags &= ~RFLAGS_ZF;
                        cpu->regs[X64_REG_RAX] = expected;
                    }
                }
            } else {
                uint64_t cur = cpu->regs[insn.dst_reg] & mask_by_size(size);
                if (cur == rax_val) {
                    cpu->rflags |= RFLAGS_ZF;
                    cpu->regs[insn.dst_reg] = src_val;
                } else {
                    cpu->rflags &= ~RFLAGS_ZF;
                    cpu->regs[X64_REG_RAX] = cur;
                }
            }
            break;
        }

        case X64_OP_XADD: {
            uint64_t src_val = cpu->regs[insn.src_reg] & mask_by_size(size);
            if (insn.dst_kind == X64_ARG_MEM) {
                uintptr_t ptr = (uintptr_t)resolve_mem_addr(cpu, &insn);
                if (size == 8) {
                    uint64_t old = __atomic_fetch_add((uint64_t *)ptr, src_val, __ATOMIC_SEQ_CST);
                    cpu->regs[insn.src_reg] = old;
                } else if (size == 4) {
                    uint32_t old = __atomic_fetch_add((uint32_t *)ptr, (uint32_t)src_val, __ATOMIC_SEQ_CST);
                    cpu->regs[insn.src_reg] = old;
                }
            } else {
                uint64_t dst_val = cpu->regs[insn.dst_reg] & mask_by_size(size);
                cpu->regs[insn.dst_reg] = (dst_val + src_val) & mask_by_size(size);
                cpu->regs[insn.src_reg] = dst_val;
            }
            break;
        }

        case X64_OP_XCHG: {
            uint64_t src_val = cpu->regs[insn.src_reg] & mask_by_size(size);
            if (insn.dst_kind == X64_ARG_MEM) {
                uintptr_t ptr = (uintptr_t)resolve_mem_addr(cpu, &insn);
                if (size == 8) {
                    uint64_t old = __atomic_exchange_n((uint64_t *)ptr, src_val, __ATOMIC_SEQ_CST);
                    cpu->regs[insn.src_reg] = old;
                } else if (size == 4) {
                    uint32_t old = __atomic_exchange_n((uint32_t *)ptr, (uint32_t)src_val, __ATOMIC_SEQ_CST);
                    cpu->regs[insn.src_reg] = old;
                }
            } else {
                uint64_t tmp = cpu->regs[insn.dst_reg];
                cpu->regs[insn.dst_reg] = cpu->regs[insn.src_reg];
                cpu->regs[insn.src_reg] = tmp;
            }
            break;
        }

        case X64_OP_MFENCE:
            __atomic_thread_fence(__ATOMIC_SEQ_CST);
            break;

        case X64_OP_LFENCE:
            __atomic_thread_fence(__ATOMIC_ACQUIRE);
            break;

        case X64_OP_SFENCE:
            __atomic_thread_fence(__ATOMIC_RELEASE);
            break;

        case X64_OP_PAUSE:
            sched_yield();
            break;

        case X64_OP_SYSCALL:
            cpu->status = X64_STATUS_SYSCALL;
            cpu->rip = next_rip;
            cpu->insn_count++;
            total_insns_interpreted++;
            return 1;

        case X64_OP_UD2:
            cpu->status = X64_STATUS_TRAP;
            cpu->trap_code = 0x0F0B;
            return 0;

        default:
            cpu->status = X64_STATUS_INVALID_INSN;
            return 0;
    }

    cpu->rip = next_rip;
    cpu->insn_count++;
    total_insns_interpreted++;
    return 1;
}

int x64_run(x64_cpu_t *cpu, uint64_t max_insns) {
    if (!cpu) return 0;
    uint64_t ran = 0;
    while (ran < max_insns && (cpu->status == X64_STATUS_READY || cpu->status == X64_STATUS_RUNNING)) {
        if (!x64_step(cpu)) break;
        ran++;
    }
    return (int)ran;
}

x64_cache_t *x64_cache_create(size_t bucket_count) {
    if (!bucket_count) bucket_count = 1024;
    x64_cache_t *cache = calloc(1, sizeof(*cache));
    if (!cache) return NULL;
    cache->buckets = calloc(bucket_count, sizeof(x64_block_t *));
    if (!cache->buckets) { free(cache); return NULL; }
    cache->bucket_count = bucket_count;
    return cache;
}

void x64_cache_destroy(x64_cache_t *cache) {
    if (!cache) return;
    for (size_t i = 0; i < cache->bucket_count; ++i) {
        x64_block_t *cur = cache->buckets[i];
        while (cur) {
            x64_block_t *next = cur->hash_next;
            if (cur->host_code) free(cur->host_code);
            free(cur);
            cur = next;
        }
    }
    free(cache->buckets);
    free(cache);
}

x64_block_t *x64_cache_lookup(x64_cache_t *cache, uint64_t guest_pc) {
    if (!cache || !cache->buckets) return NULL;
    size_t idx = (guest_pc ^ (guest_pc >> 12)) % cache->bucket_count;
    x64_block_t *cur = cache->buckets[idx];
    while (cur) {
        if (cur->guest_pc == guest_pc) return cur;
        cur = cur->hash_next;
    }
    return NULL;
}

static int block_interpreter_exec(x64_cpu_t *cpu) {
    return x64_step(cpu);
}

x64_block_t *x64_cache_translate(x64_cache_t *cache, x64_cpu_t *cpu, uint64_t guest_pc) {
    if (!cache) return NULL;
    x64_block_t *existing = x64_cache_lookup(cache, guest_pc);
    if (existing) return existing;

    x64_block_t *block = calloc(1, sizeof(*block));
    if (!block) return NULL;
    block->guest_pc = guest_pc;
    block->exec = block_interpreter_exec;

    // analyze block bounds
    uint64_t cur_pc = guest_pc;
    uint32_t count = 0;
    uint32_t bytes = 0;
    x64_insn_t insn;

    (void)cpu;
    while (count < 64) {
        int len = x64_decode((const uint8_t *)(uintptr_t)cur_pc, 15, cur_pc, &insn);
        if (!len) break;
        count++;
        bytes += len;
        cur_pc += len;
        if (insn.op == X64_OP_RET || insn.op == X64_OP_JMP || insn.op == X64_OP_JCC || insn.op == X64_OP_SYSCALL || insn.op == X64_OP_UD2) {
            break;
        }
    }

    block->insn_count = count ? count : 1;
    block->guest_bytes = bytes ? bytes : 1;

    size_t idx = (guest_pc ^ (guest_pc >> 12)) % cache->bucket_count;
    block->hash_next = cache->buckets[idx];
    cache->buckets[idx] = block;
    cache->block_count++;
    total_blocks_translated++;

    return block;
}

// aarch64 machine code emitter primitives
size_t arm64_emit_mov_reg(uint8_t *buf, uint8_t dst_reg, uint8_t src_reg) {
    if (!buf) return 4;
    // orr
    uint32_t insn = 0xAA0003E0 | ((src_reg & 0x1F) << 16) | (dst_reg & 0x1F);
    memcpy(buf, &insn, 4);
    return 4;
}

size_t arm64_emit_add_imm(uint8_t *buf, uint8_t dst_reg, uint8_t src_reg, uint16_t imm) {
    if (!buf) return 4;
    // add imm
    uint32_t insn = 0x91000000 | ((imm & 0xFFF) << 10) | ((src_reg & 0x1F) << 5) | (dst_reg & 0x1F);
    memcpy(buf, &insn, 4);
    return 4;
}

size_t arm64_emit_sub_imm(uint8_t *buf, uint8_t dst_reg, uint8_t src_reg, uint16_t imm) {
    if (!buf) return 4;
    // sub imm
    uint32_t insn = 0xD1000000 | ((imm & 0xFFF) << 10) | ((src_reg & 0x1F) << 5) | (dst_reg & 0x1F);
    memcpy(buf, &insn, 4);
    return 4;
}

size_t arm64_emit_ret(uint8_t *buf) {
    if (!buf) return 4;
    // ret
    uint32_t insn = 0xD65F03C0;
    memcpy(buf, &insn, 4);
    return 4;
}

size_t arm64_emit_mov_imm(uint8_t *buf, uint8_t dst_reg, uint16_t imm) {
    if (!buf) return 4;
    // movz
    uint32_t insn = 0xD2800000 | ((imm & 0xFFFF) << 5) | (dst_reg & 0x1F);
    memcpy(buf, &insn, 4);
    return 4;
}

size_t arm64_emit_nop(uint8_t *buf) {
    if (!buf) return 4;
    // nop
    uint32_t insn = 0xD503201F;
    memcpy(buf, &insn, 4);
    return 4;
}

int arm64_jit_compile_block(x64_block_t *block) {
    if (!block) return 0;
    size_t alloc_sz = 4096;
    void *code = mmap(NULL, alloc_sz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (code == MAP_FAILED) return 0;

    uint8_t *ptr = (uint8_t *)code;
    size_t written = 0;

    // prologue
    written += arm64_emit_nop(ptr + written);
    written += arm64_emit_ret(ptr + written);

#if defined(__aarch64__)
    __builtin___clear_cache((char *)code, (char *)code + written);
#endif

    block->host_code = (uint8_t *)code;
    block->host_size = written;
    return 1;
}

size_t arm64_emit_dmb_ish(uint8_t *buf) {
    if (!buf) return 4;
    // dmb ish
    uint32_t insn = 0xD5033BBF;
    memcpy(buf, &insn, 4);
    return 4;
}

size_t arm64_emit_fadd_s(uint8_t *buf, uint8_t dst_reg, uint8_t src1, uint8_t src2) {
    if (!buf) return 4;
    // fadd s
    uint32_t insn = 0x1E202800 | ((src2 & 0x1F) << 16) | ((src1 & 0x1F) << 5) | (dst_reg & 0x1F);
    memcpy(buf, &insn, 4);
    return 4;
}

size_t arm64_emit_fsub_s(uint8_t *buf, uint8_t dst_reg, uint8_t src1, uint8_t src2) {
    if (!buf) return 4;
    // fsub s
    uint32_t insn = 0x1E203800 | ((src2 & 0x1F) << 16) | ((src1 & 0x1F) << 5) | (dst_reg & 0x1F);
    memcpy(buf, &insn, 4);
    return 4;
}

size_t arm64_emit_fmul_s(uint8_t *buf, uint8_t dst_reg, uint8_t src1, uint8_t src2) {
    if (!buf) return 4;
    // fmul s
    uint32_t insn = 0x1E200800 | ((src2 & 0x1F) << 16) | ((src1 & 0x1F) << 5) | (dst_reg & 0x1F);
    memcpy(buf, &insn, 4);
    return 4;
}

size_t arm64_emit_fdiv_s(uint8_t *buf, uint8_t dst_reg, uint8_t src1, uint8_t src2) {
    if (!buf) return 4;
    // fdiv s
    uint32_t insn = 0x1E201800 | ((src2 & 0x1F) << 16) | ((src1 & 0x1F) << 5) | (dst_reg & 0x1F);
    memcpy(buf, &insn, 4);
    return 4;
}

size_t arm64_emit_fadd_d(uint8_t *buf, uint8_t dst_reg, uint8_t src1, uint8_t src2) {
    if (!buf) return 4;
    // fadd d
    uint32_t insn = 0x1E602800 | ((src2 & 0x1F) << 16) | ((src1 & 0x1F) << 5) | (dst_reg & 0x1F);
    memcpy(buf, &insn, 4);
    return 4;
}

size_t arm64_emit_fsub_d(uint8_t *buf, uint8_t dst_reg, uint8_t src1, uint8_t src2) {
    if (!buf) return 4;
    // fsub d
    uint32_t insn = 0x1E603800 | ((src2 & 0x1F) << 16) | ((src1 & 0x1F) << 5) | (dst_reg & 0x1F);
    memcpy(buf, &insn, 4);
    return 4;
}

size_t arm64_emit_fmul_d(uint8_t *buf, uint8_t dst_reg, uint8_t src1, uint8_t src2) {
    if (!buf) return 4;
    // fmul d
    uint32_t insn = 0x1E600800 | ((src2 & 0x1F) << 16) | ((src1 & 0x1F) << 5) | (dst_reg & 0x1F);
    memcpy(buf, &insn, 4);
    return 4;
}

size_t arm64_emit_fdiv_d(uint8_t *buf, uint8_t dst_reg, uint8_t src1, uint8_t src2) {
    if (!buf) return 4;
    // fdiv d
    uint32_t insn = 0x1E601800 | ((src2 & 0x1F) << 16) | ((src1 & 0x1F) << 5) | (dst_reg & 0x1F);
    memcpy(buf, &insn, 4);
    return 4;
}

static _Thread_local x64_cpu_t *g_current_cpu = NULL;

x64_cpu_t *x64_cpu_create(uint64_t entry, uint64_t stack_size) {
    if (!stack_size) stack_size = 2 * 1024 * 1024;
    x64_cpu_t *cpu = calloc(1, sizeof(*cpu));
    if (!cpu) return NULL;
    uint8_t *stack = malloc(stack_size);
    if (!stack) { free(cpu); return NULL; }
    x64_cpu_init(cpu, entry, (uint64_t)(uintptr_t)(stack + stack_size - 64));
    cpu->user_data = stack;
    return cpu;
}

void x64_cpu_destroy(x64_cpu_t *cpu) {
    if (!cpu) return;
    if (cpu->user_data) free(cpu->user_data);
    free(cpu);
}

x64_cpu_t *x64_current_cpu(void) {
    return g_current_cpu;
}

void x64_set_current_cpu(x64_cpu_t *cpu) {
    g_current_cpu = cpu;
}

void runtime_translator_report(void) {
    printf("Translator: interpreted instructions=%" PRIu64 ", translated blocks=%" PRIu64 "\n",
           total_insns_interpreted, total_blocks_translated);
}
