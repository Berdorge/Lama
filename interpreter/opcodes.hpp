#ifndef OPCODES_HPP
#define OPCODES_HPP

#include <cstddef>
#include <cstdint>

constexpr uint8_t opcode_add = 0x01;
constexpr uint8_t opcode_sub = 0x02;
constexpr uint8_t opcode_mul = 0x03;
constexpr uint8_t opcode_div = 0x04;
constexpr uint8_t opcode_rem = 0x05;
constexpr uint8_t opcode_lt = 0x06;
constexpr uint8_t opcode_leq = 0x07;
constexpr uint8_t opcode_gt = 0x08;
constexpr uint8_t opcode_geq = 0x09;
constexpr uint8_t opcode_eq = 0x0A;
constexpr uint8_t opcode_neq = 0x0B;
constexpr uint8_t opcode_and = 0x0C;
constexpr uint8_t opcode_or = 0x0D;

constexpr uint8_t opcode_const = 0x10;
constexpr uint8_t opcode_string = 0x11;
constexpr uint8_t opcode_sexp = 0x12;
constexpr uint8_t opcode_sta = 0x14;
constexpr uint8_t opcode_jmp = 0x15;
constexpr uint8_t opcode_end = 0x16;
constexpr uint8_t opcode_ret = 0x17;
constexpr uint8_t opcode_drop = 0x18;
constexpr uint8_t opcode_dup = 0x19;
constexpr uint8_t opcode_swap = 0x1A;
constexpr uint8_t opcode_elem = 0x1B;

constexpr uint8_t designation_global = 0x00;
constexpr uint8_t designation_local = 0x01;
constexpr uint8_t designation_arg = 0x02;
constexpr uint8_t designation_capture = 0x03;

constexpr uint8_t opcode_ld_base = 0x20;
constexpr uint8_t opcode_ld_global = opcode_ld_base + designation_global;
constexpr uint8_t opcode_ld_local = opcode_ld_base + designation_local;
constexpr uint8_t opcode_ld_arg = opcode_ld_base + designation_arg;
constexpr uint8_t opcode_ld_capture = opcode_ld_base + designation_capture;

constexpr uint8_t opcode_st_base = 0x40;
constexpr uint8_t opcode_st_global = opcode_st_base + designation_global;
constexpr uint8_t opcode_st_local = opcode_st_base + designation_local;
constexpr uint8_t opcode_st_arg = opcode_st_base + designation_arg;
constexpr uint8_t opcode_st_capture = opcode_st_base + designation_capture;

constexpr uint8_t opcode_cjmp_z = 0x50;
constexpr uint8_t opcode_cjmp_nz = 0x51;
constexpr uint8_t opcode_begin = 0x52;
constexpr uint8_t opcode_beginc = 0x53;
constexpr uint8_t opcode_closure = 0x54;
constexpr uint8_t opcode_callc = 0x55;
constexpr uint8_t opcode_call = 0x56;
constexpr uint8_t opcode_tag = 0x57;
constexpr uint8_t opcode_array = 0x58;
constexpr uint8_t opcode_fail = 0x59;
constexpr uint8_t opcode_line = 0x5A;

constexpr uint8_t opcode_pattern_strcmp = 0x60;
constexpr uint8_t opcode_pattern_string = 0x61;
constexpr uint8_t opcode_pattern_array = 0x62;
constexpr uint8_t opcode_pattern_sexp = 0x63;
constexpr uint8_t opcode_pattern_boxed = 0x64;
constexpr uint8_t opcode_pattern_unboxed = 0x65;
constexpr uint8_t opcode_pattern_closure = 0x66;

constexpr uint8_t opcode_builtin_read = 0x70;
constexpr uint8_t opcode_builtin_write = 0x71;
constexpr uint8_t opcode_builtin_length = 0x72;
constexpr uint8_t opcode_builtin_string = 0x73;
constexpr uint8_t opcode_builtin_array = 0x74;

#endif
