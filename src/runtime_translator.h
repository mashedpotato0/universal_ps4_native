#ifndef RUNTIME_TRANSLATOR_H
#define RUNTIME_TRANSLATOR_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    X64_REG_RAX = 0,
    X64_REG_RCX = 1,
    X64_REG_RDX = 2,
    X64_REG_RBX = 3,
    X64_REG_RSP = 4,
    X64_REG_RBP = 5,
    X64_REG_RSI = 6,
    X64_REG_RDI = 7,
    X64_REG_R8  = 8,
    X64_REG_R9  = 9,
    X64_REG_R10 = 10,
    X64_REG_R11 = 11,
    X64_REG_R12 = 12,
    X64_REG_R13 = 13,
    X64_REG_R14 = 14,
    X64_REG_R15 = 15,
    X64_REG_COUNT = 16
} x64_reg_t;

typedef enum {
    X64_STATUS_READY = 0,
    X64_STATUS_RUNNING = 1,
    X64_STATUS_HALTED = 2,
    X64_STATUS_SYSCALL = 3,
    X64_STATUS_TRAP = 4,
    X64_STATUS_INVALID_INSN = 5,
    X64_STATUS_MEM_FAULT = 6
} x64_status_t;

typedef enum {
    X64_OP_INVALID = 0,
    X64_OP_NOP,
    X64_OP_MOV,
    X64_OP_MOVZX,
    X64_OP_MOVSX,
    X64_OP_MOVSXD,
    X64_OP_LEA,
    X64_OP_ADD,
    X64_OP_SUB,
    X64_OP_CMP,
    X64_OP_TEST,
    X64_OP_AND,
    X64_OP_OR,
    X64_OP_XOR,
    X64_OP_INC,
    X64_OP_DEC,
    X64_OP_NEG,
    X64_OP_NOT,
    X64_OP_IMUL,
    X64_OP_SHL,
    X64_OP_SHR,
    X64_OP_SAR,
    X64_OP_PUSH,
    X64_OP_POP,
    X64_OP_CALL,
    X64_OP_RET,
    X64_OP_JMP,
    X64_OP_JCC,
    X64_OP_SYSCALL,
    X64_OP_UD2
} x64_op_t;

typedef enum {
    X64_COND_O  = 0,
    X64_COND_NO = 1,
    X64_COND_B  = 2,
    X64_COND_AE = 3,
    X64_COND_E  = 4,
    X64_COND_NE = 5,
    X64_COND_BE = 6,
    X64_COND_A  = 7,
    X64_COND_S  = 8,
    X64_COND_NS = 9,
    X64_COND_P  = 10,
    X64_COND_NP = 11,
    X64_COND_L  = 12,
    X64_COND_GE = 13,
    X64_COND_LE = 14,
    X64_COND_G  = 15
} x64_cond_t;

typedef enum {
    X64_ARG_NONE = 0,
    X64_ARG_REG,
    X64_ARG_MEM,
    X64_ARG_IMM
} x64_arg_t;

typedef struct {
    x64_op_t op;
    uint8_t len;
    uint8_t op_size;
    x64_cond_t cond;

    x64_arg_t dst_kind;
    x64_reg_t dst_reg;

    x64_arg_t src_kind;
    x64_reg_t src_reg;

    uint64_t imm;

    x64_reg_t mem_base;
    x64_reg_t mem_index;
    uint8_t mem_scale;
    int32_t mem_disp;
    uint8_t is_rip_rel;
    uint8_t has_index;
    uint8_t segment_override;
} x64_insn_t;

typedef union {
    uint8_t  u8[16];
    uint32_t u32[4];
    uint64_t u64[2];
    float    f32[4];
    double   f64[2];
} x64_vec128_t;

typedef struct x64_cpu {
    uint64_t regs[X64_REG_COUNT];
    uint64_t rip;
    uint64_t rflags;
    uint64_t fs_base;
    uint64_t gs_base;
    x64_vec128_t xmm[16];
    uint32_t mxcsr;
    x64_status_t status;
    uint32_t trap_code;
    uint64_t insn_count;
    void *user_data;
} x64_cpu_t;

typedef struct x64_block {
    uint64_t guest_pc;
    uint32_t insn_count;
    uint32_t guest_bytes;
    int (*exec)(x64_cpu_t *cpu);
    uint8_t *host_code;
    size_t host_size;
    struct x64_block *hash_next;
} x64_block_t;

typedef struct {
    x64_block_t **buckets;
    size_t bucket_count;
    size_t block_count;
} x64_cache_t;

void x64_cpu_init(x64_cpu_t *cpu, uint64_t entry, uint64_t stack_top);
void x64_cpu_set_tls(x64_cpu_t *cpu, uint64_t fs_base, uint64_t gs_base);
int x64_decode(const uint8_t *code, size_t max_len, uint64_t rip, x64_insn_t *insn);
int x64_step(x64_cpu_t *cpu);
int x64_run(x64_cpu_t *cpu, uint64_t max_insns);

x64_cache_t *x64_cache_create(size_t bucket_count);
void x64_cache_destroy(x64_cache_t *cache);
x64_block_t *x64_cache_lookup(x64_cache_t *cache, uint64_t guest_pc);
x64_block_t *x64_cache_translate(x64_cache_t *cache, x64_cpu_t *cpu, uint64_t guest_pc);

size_t arm64_emit_mov_reg(uint8_t *buf, uint8_t dst_reg, uint8_t src_reg);
size_t arm64_emit_add_imm(uint8_t *buf, uint8_t dst_reg, uint8_t src_reg, uint16_t imm);
size_t arm64_emit_sub_imm(uint8_t *buf, uint8_t dst_reg, uint8_t src_reg, uint16_t imm);
size_t arm64_emit_ret(uint8_t *buf);

void runtime_translator_report(void);

#endif
