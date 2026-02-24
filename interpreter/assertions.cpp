#include "assertions.hpp"
#include "bytefile.hpp"
#include "frame.hpp"
#include "instructions.hpp"
#include "runtime.hpp"

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

char const* kind_description(aint kind)
{
    switch (kind)
    {
    case STRING_TAG:
        return "STRING";
    case ARRAY_TAG:
        return "ARRAY";
    case SEXP_TAG:
        return "SEXP";
    case CLOSURE_TAG:
        return "CLOSURE";
    case UNBOXED_TAG:
        return "INT";
    default:
        return "UNKNOWN";
    }
}

char const* opcode_description(uint8_t opcode)
{
    switch (opcode)
    {
    case opcode_add:
        return "ADD";
    case opcode_sub:
        return "SUB";
    case opcode_mul:
        return "MUL";
    case opcode_div:
        return "DIV";
    case opcode_rem:
        return "REM";
    case opcode_lt:
        return "LT";
    case opcode_leq:
        return "LEQ";
    case opcode_gt:
        return "GT";
    case opcode_geq:
        return "GEQ";
    case opcode_eq:
        return "EQ";
    case opcode_neq:
        return "NEQ";
    case opcode_and:
        return "AND";
    case opcode_or:
        return "OR";

    case opcode_const:
        return "CONST";
    case opcode_string:
        return "STRING";
    case opcode_sexp:
        return "SEXP";
    case opcode_sta:
        return "STA";
    case opcode_jmp:
        return "JMP";
    case opcode_end:
        return "END";
    case opcode_ret:
        return "RET";
    case opcode_drop:
        return "DROP";
    case opcode_dup:
        return "DUP";
    case opcode_swap:
        return "SWAP";
    case opcode_elem:
        return "ELEM";

    case opcode_ld_global:
        return "LD_GLOBAL";
    case opcode_ld_local:
        return "LD_LOCAL";
    case opcode_ld_arg:
        return "LD_ARG";
    case opcode_ld_capture:
        return "LD_CAPTURE";

    case opcode_st_global:
        return "ST_GLOBAL";
    case opcode_st_local:
        return "ST_LOCAL";
    case opcode_st_arg:
        return "ST_ARG";
    case opcode_st_capture:
        return "ST_CAPTURE";

    case opcode_cjmp_z:
        return "CJMP_Z";
    case opcode_cjmp_nz:
        return "CJMP_NZ";
    case opcode_begin:
        return "BEGIN";
    case opcode_beginc:
        return "BEGINC";
    case opcode_closure:
        return "CLOSURE";
    case opcode_callc:
        return "CALLC";
    case opcode_call:
        return "CALL";
    case opcode_tag:
        return "TAG";
    case opcode_array:
        return "ARRAY";
    case opcode_fail:
        return "FAIL";
    case opcode_line:
        return "LINE";

    case opcode_pattern_strcmp:
        return "PATTERN_STRCMP";
    case opcode_pattern_string:
        return "PATTERN_STRING";
    case opcode_pattern_array:
        return "PATTERN_ARRAY";
    case opcode_pattern_sexp:
        return "PATTERN_SEXP";
    case opcode_pattern_boxed:
        return "PATTERN_BOXED";
    case opcode_pattern_unboxed:
        return "PATTERN_UNBOXED";
    case opcode_pattern_closure:
        return "PATTERN_CLOSURE";

    case opcode_builtin_read:
        return "BUILTIN_READ";
    case opcode_builtin_write:
        return "BUILTIN_WRITE";
    case opcode_builtin_length:
        return "BUILTIN_LENGTH";
    case opcode_builtin_string:
        return "BUILTIN_STRING";
    case opcode_builtin_array:
        return "BUILTIN_ARRAY";

    default:
        return "UNKNOWN";
    }
}

void assert_boxed(aint value)
{
    if (UNBOXED(value))
    {
        aint value_kind = LkindOf((void*)value);
        interpret_stage_failure("Expected boxed value, got %s", kind_description(value_kind));
    }
}

void assert_unboxed(aint value)
{
    if (!UNBOXED(value))
    {
        aint value_kind = LkindOf((void*)value);
        interpret_stage_failure("Expected unboxed value, got %s", kind_description(value_kind));
    }
}

aint unbox_safe(aint value)
{
    assert_unboxed(value);
    return UNBOX(value);
}

void assert_kind(aint value, aint expected_kind)
{
    aint value_kind = LkindOf((void*)value);
    if (value_kind != expected_kind)
    {
        interpret_stage_failure(
            "Expected %s, got %s", kind_description(expected_kind), kind_description(value_kind)
        );
    }
}

void interpret_stage_failure(char const* msg, ...)
{
    va_list args;
    va_start(args, msg);
    fprintf(stderr, "*** FAILURE: ");
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
    va_end(args);

    for (auto frame = current_frame; frame != nullptr; frame = frame->prev)
    {
        fprintf(
            stderr, "\tat line %zu, IP %zu, %s\n", UNBOX(frame->current_line),
            (size_t)(frame->current_instruction_ptr - code_ptr),
            opcode_description(*(frame->current_instruction_ptr))
        );
    }

    exit(255);
}
