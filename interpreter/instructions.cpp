#include "instructions.hpp"
#include "assertions.hpp"
#include "bytefile.hpp"
#include "frame.hpp"
#include "runtime.hpp"

#include <cstdio>
#include <cstring>

static void unknown_instruction()
{
    interpret_stage_failure(
        "Unknown instruction 0x%02X", *(current_frame->current_instruction_ptr)
    );
}

static void check_args_eq(aint x, aint y)
{
}

static void check_args_unboxed(aint& x, aint& y)
{
    x = unbox_safe(x);
    y = unbox_safe(y);
}

static void check_args_div(aint& x, aint& y)
{
    check_args_unboxed(x, y);
    if (y == 0)
    {
        interpret_stage_failure("Division by zero");
    }
}

#define BINOP(op, check_args)                                                                      \
    {                                                                                              \
        check_stack_has(2);                                                                        \
        auto y = pop_stack_unsafe();                                                               \
        auto x = pop_stack_unsafe();                                                               \
        check_args(x, y);                                                                          \
        push_stack_unsafe(BOX(x op y));                                                            \
        break;                                                                                     \
    }

static void const_insn()
{
    aint value = next_code_int32_t();
    push_stack_safe(BOX(value));
}

static void string_insn()
{
    auto contents = next_code_string();
    auto string_obj = Bstring(&contents);
    push_stack_safe((aint)string_obj);
}

static void sexp_insn()
{
    auto tag_contents = next_code_string();
    aint n = next_code_int32_t();
    check_stack_has(n);
    auto tag_hash = LtagHash(tag_contents);
    push_stack_safe(tag_hash);
    auto result = Bsexp(&stack_at_unsafe(stack_size() - (n + 1)), BOX(n + 1));
    drop_stack_unsafe(n + 1);
    push_stack_unsafe((aint)result);
}

static void sta()
{
    aint v = pop_stack_safe();
    aint i = pop_stack_safe();
    aint x = pop_stack_safe();
    assert_unboxed(i);
    assert_boxed(x);
    auto result = Bsta((void*)x, i, (void*)v);
    push_stack_unsafe((aint)result);
}

static void swap()
{
    aint x = pop_stack_safe();
    aint y = pop_stack_safe();
    push_stack_unsafe(x);
    push_stack_unsafe(y);
}

static void elem()
{
    aint i = pop_stack_safe();
    aint p = pop_stack_safe();
    assert_unboxed(i);
    assert_boxed(p);
    auto x = Belem((void*)p, i);
    push_stack_unsafe((aint)x);
}

#define LD(name) push_stack_safe(next_code_##name());

#define ST(name) next_code_##name() = peek_stack_safe();

static void cjmp_z()
{
    auto address = next_code_fixup();
    aint cond = unbox_safe(pop_stack_safe());
    if (cond == 0)
    {
        ip = address;
    }
}

static void cjmp_nz()
{
    auto address = next_code_fixup();
    aint cond = unbox_safe(pop_stack_safe());
    if (cond != 0)
    {
        ip = address;
    }
}

static void begin_insn()
{
    next_code_uint32_t();
    auto locals_size = next_code_uint32_t();

    for (size_t i = 0; i < locals_size; ++i)
    {
        push_stack_safe(0);
    }

    current_frame->locals_size = BOX(locals_size);
    current_frame->stack_base += locals_size;
}

static void closure_insn()
{
    aint address = (aint)next_code_fixup();
    aint args_size = next_code_uint32_t();
    push_stack_safe(address);
    for (int i = 0; i < args_size; i++)
    {
        switch (next_code_byte())
        {
        case designation_global:
            push_stack_safe(next_code_global());
            break;
        case designation_local:
            push_stack_safe(next_code_local());
            break;
        case designation_arg:
            push_stack_safe(next_code_arg());
            break;
        case designation_capture:
            push_stack_safe(next_code_capture());
            break;
        default:
            interpret_stage_failure("Invalid closure capture kind");
        }
    }
    auto result = Bclosure(&stack_at_unsafe(stack_size() - (args_size + 1)), BOX(args_size + 1));
    drop_stack_unsafe(args_size + 1);
    push_stack_unsafe((aint)result);
}

static void
generic_call(uint32_t args_size, uint32_t capture_size, uint8_t* destination, const char* name)
{
    auto new_frame = push_frame_safe();

    new_frame->prev = current_frame;
    new_frame->current_instruction_ptr = destination;
    new_frame->stack_base = (aint*)__gc_stack_bottom;
    new_frame->current_line = BOX(1);
    new_frame->capture_size = BOX(capture_size);
    new_frame->args_size = BOX(args_size);
    new_frame->locals_size = BOX(0);

    current_frame = new_frame;
    ip = destination;
}

#define READ_CALLC_ARGS auto args_size = next_code_uint32_t();

static void callc()
{
    READ_CALLC_ARGS;

    check_stack_has(args_size + 1);

    aint closure_ptr = stack_at_unsafe(stack_size() - (args_size + 1));
    data* closure = TO_DATA((void*)closure_ptr);
    size_t capture_size = LEN(closure->data_header) - 1;

    return generic_call(args_size, capture_size, (uint8_t*)((aint*)closure->contents)[0], "CALLC");
}

#define READ_CALL_ARGS                                                                             \
    auto destination = next_code_fixup();                                                          \
    auto args_size = next_code_uint32_t();

static void call_insn()
{
    READ_CALL_ARGS;

    check_stack_has(args_size);

    return generic_call(args_size, 0, destination, "CALL");
}

static void tag_insn()
{
    auto tag_contents = next_code_string();
    auto args_count = next_code_uint32_t();
    auto checked_obj = pop_stack_safe();
    auto tag_hash = LtagHash(tag_contents);
    auto result = Btag((void*)checked_obj, tag_hash, BOX(args_count));
    push_stack_unsafe(result);
}

static void array_insn()
{
    auto len = next_code_uint32_t();
    auto x = pop_stack_safe();
    auto result = Barray_patt((void*)x, BOX(len));
    push_stack_unsafe(result);
}

static void fail_insn()
{
    next_code_uint32_t();
    next_code_uint32_t();
    interpret_stage_failure("Encountered FAIL instruction");
}

#define PATTERN(func)                                                                              \
    {                                                                                              \
        auto x = pop_stack_safe();                                                                 \
        push_stack_unsafe(B##func##_patt((void*)x));                                               \
        break;                                                                                     \
    }

static void pattern_strcmp()
{
    auto x = pop_stack_safe();
    auto y = pop_stack_safe();
    assert_kind(x, STRING_TAG);
    assert_kind(y, STRING_TAG);
    auto result = Bstring_patt((void*)x, (void*)y);
    push_stack_unsafe(result);
}

static void call_builtin_write()
{
    auto x = pop_stack_safe();
    assert_unboxed(x);
    Lwrite(x);
    push_stack_unsafe(BOX(0));
}

static void call_builtin_length()
{
    auto obj_ptr = pop_stack_safe();
    assert_boxed(obj_ptr);
    auto len = Llength((void*)obj_ptr);
    push_stack_unsafe(len);
}

static void call_builtin_string()
{
    aint obj_ptr = pop_stack_safe();
    auto obj_str_ptr = Lstring(&obj_ptr);
    push_stack_unsafe((aint)obj_str_ptr);
}

static void call_builtin_array()
{
    auto length = next_code_uint32_t();
    check_stack_has(length);
    auto array = Barray(&stack_at_unsafe(stack_size() - length), BOX(length));
    drop_stack_unsafe(length);
    push_stack_safe((aint)array);
}

static void pop_frame()
{
    local_frame* popped_frame = current_frame;

    auto return_value = pop_stack_safe();
    drop_stack_unsafe(stack_size() + UNBOX(popped_frame->locals_size) + local_frame_size_on_stack);

    current_frame = popped_frame->prev;
    ip = current_frame->current_instruction_ptr + 1;
    if (UNBOX(popped_frame->capture_size))
    {
        drop_stack_unsafe(UNBOX(popped_frame->args_size) + 1);
        READ_CALLC_ARGS;
    }
    else
    {
        drop_stack_unsafe(UNBOX(popped_frame->args_size));
        READ_CALL_ARGS;
    }

    push_stack_unsafe(return_value);
}

void run_instructions()
{
    while (true)
    {
        current_frame->current_instruction_ptr = ip;
        uint8_t opcode = next_code_byte();
        switch (opcode)
        {
        case opcode_add:
            BINOP(+, check_args_unboxed)
        case opcode_sub:
            BINOP(-, check_args_unboxed)
        case opcode_mul:
            BINOP(*, check_args_unboxed)
        case opcode_div:
            BINOP(/, check_args_div)
        case opcode_rem:
            BINOP(%, check_args_div)
        case opcode_lt:
            BINOP(<, check_args_unboxed)
        case opcode_leq:
            BINOP(<=, check_args_unboxed)
        case opcode_gt:
            BINOP(>, check_args_unboxed)
        case opcode_geq:
            BINOP(>=, check_args_unboxed)
        case opcode_eq:
            BINOP(==, check_args_eq)
        case opcode_neq:
            BINOP(!=, check_args_unboxed)
        case opcode_and:
            BINOP(&&, check_args_unboxed)
        case opcode_or:
            BINOP(||, check_args_unboxed)
        case opcode_const:
            const_insn();
            break;
        case opcode_string:
            string_insn();
            break;
        case opcode_sexp:
            sexp_insn();
            break;
        case opcode_sta:
            sta();
            break;
        case opcode_jmp:
            ip = next_code_fixup();
            break;
        case opcode_end:
        case opcode_ret:
            if (current_frame->prev == nullptr)
            {
                return;
            }
            pop_frame();
            break;
        case opcode_drop:
            pop_stack_safe();
            break;
        case opcode_dup:
            push_stack_safe(peek_stack_safe());
            break;
        case opcode_swap:
            swap();
            break;
        case opcode_elem:
            elem();
            break;
        case opcode_ld_global:
            LD(global)
            break;
        case opcode_ld_local:
            LD(local)
            break;
        case opcode_ld_arg:
            LD(arg)
            break;
        case opcode_ld_capture:
            LD(capture)
            break;
        case opcode_st_global:
            ST(global)
            break;
        case opcode_st_local:
            ST(local)
            break;
        case opcode_st_arg:
            ST(arg)
            break;
        case opcode_st_capture:
            ST(capture)
            break;
        case opcode_cjmp_z:
            cjmp_z();
            break;
        case opcode_cjmp_nz:
            cjmp_nz();
            break;
        case opcode_begin:
        case opcode_beginc:
            begin_insn();
            break;
        case opcode_closure:
            closure_insn();
            break;
        case opcode_callc:
            callc();
            break;
        case opcode_call:
            call_insn();
            break;
        case opcode_tag:
            tag_insn();
            break;
        case opcode_array:
            array_insn();
            break;
        case opcode_fail:
            fail_insn();
            break;
        case opcode_line:
            current_frame->current_line = BOX(next_code_uint32_t());
            break;
        case opcode_pattern_strcmp:
            pattern_strcmp();
            break;
        case opcode_pattern_string:
            PATTERN(string_tag)
        case opcode_pattern_array:
            PATTERN(array_tag)
        case opcode_pattern_sexp:
            PATTERN(sexp_tag)
        case opcode_pattern_boxed:
            PATTERN(boxed)
        case opcode_pattern_unboxed:
            PATTERN(unboxed)
        case opcode_pattern_closure:
            PATTERN(closure_tag)
        case opcode_builtin_read:
            printf(" ");
            push_stack_unsafe(Lread());
            break;
        case opcode_builtin_write:
            call_builtin_write();
            break;
        case opcode_builtin_length:
            call_builtin_length();
            break;
        case opcode_builtin_string:
            call_builtin_string();
            break;
        case opcode_builtin_array:
            call_builtin_array();
            break;
        default:
            unknown_instruction();
            break;
        }
    }
}
