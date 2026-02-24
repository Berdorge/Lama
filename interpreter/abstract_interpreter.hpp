#ifndef ABSTRACT_INTERPRETER_HPP
#define ABSTRACT_INTERPRETER_HPP

#include "assertions.hpp"
#include "bytefile.hpp"
#include "frame.hpp"
#include "opcodes.hpp"

enum class operation_safety
{
    safe,
    unsafe
};

enum class binop
{
    add,
    sub,
    mul,
    div,
    rem,
    lt,
    leq,
    gt,
    geq,
    eq,
    neq,
    logical_and,
    logical_or
};

enum class designation
{
    global,
    local,
    arg,
    capture
};

enum class condition
{
    zero,
    nonzero
};

enum class pattern
{
    string_tag,
    array_tag,
    sexp_tag,
    boxed,
    unboxed,
    closure_tag
};

template <operation_safety safety>
void check_index(bytefile* bf, uint32_t index, uint32_t size, char const* what)
{
    if (safety == operation_safety::safe && index >= size)
    {
        interpret_stage_failure(bf, "%s index %u is out of bounds (>= %u)", what, index, size);
    }
}

inline void check_arg_index(bytefile* bf, uint32_t index)
{
    check_index<operation_safety::safe>(bf, index, UNBOX(current_frame->args_size), "Argument");
}

template <operation_safety safety>
char* get_string(bytefile* bf, uint32_t pos)
{
    check_index<safety>(bf, pos, bf->stringtab_size, "String");
    return &bf->string_ptr[pos];
}

inline char* get_public_name(bytefile* bf, uint32_t i)
{
    uint32_t offset = bf->public_ptr[i * 2];
    check_index<operation_safety::safe>(bf, offset, bf->stringtab_size, "Public symbol name");
    return get_string<operation_safety::unsafe>(bf, offset);
}

inline uint8_t* get_public_ip(bytefile* bf, uint32_t i)
{
    uint32_t offset = bf->public_ptr[i * 2 + 1];
    check_index<operation_safety::safe>(bf, offset, bf->code_length, "Public symbol begin");
    return &bf->code_ptr[offset];
}

template <operation_safety safety>
void peek(bytefile* bf, uint8_t*& ip, uint8_t* buffer, size_t length, char const* what)
{
    if (safety == operation_safety::safe && (ip - bf->code_ptr) + length > bf->code_length)
    {
        interpret_stage_failure(bf, "Unexpected end of bytecode while peeking %s", what);
    }
    for (size_t i = 0; i < length; i++)
    {
        buffer[i] = ip[i];
    }
}

template <operation_safety safety>
void read(bytefile* bf, uint8_t*& ip, uint8_t* buffer, size_t length, char const* what)
{
    peek<safety>(bf, ip, buffer, length, what);
    ip += length;
}

template <operation_safety safety>
uint8_t peek_byte(bytefile* bf, uint8_t*& ip)
{
    uint8_t buffer[1];
    peek<safety>(bf, ip, buffer, 1, "byte");
    return buffer[0];
}

template <operation_safety safety>
uint8_t read_byte(bytefile* bf, uint8_t*& ip)
{
    uint8_t buffer[1];
    read<safety>(bf, ip, buffer, 1, "byte");
    return buffer[0];
}

template <operation_safety safety>
uint32_t read_uint32_t(bytefile* bf, uint8_t*& ip)
{
    uint8_t buffer[4];
    read<safety>(bf, ip, buffer, 4, "4-byte unsigned int");
    return le_bytes_to_uint32_t(buffer);
}

template <operation_safety safety>
int32_t read_int32_t(bytefile* bf, uint8_t*& ip)
{
    return read_uint32_t<safety>(bf, ip);
}

template <operation_safety safety>
char* read_string(bytefile* bf, uint8_t*& ip)
{
    uint32_t offset = read_uint32_t<safety>(bf, ip);
    return get_string<safety>(bf, offset);
}

template <operation_safety safety>
uint8_t* read_fixup(bytefile* bf, uint8_t*& ip)
{
    uint32_t offset = read_uint32_t<safety>(bf, ip);
    check_index<safety>(bf, offset, bf->code_length, "Target");
    return &bf->code_ptr[offset];
}

#define RESOLVE_DESIGNATION(desig, visitor)                                                        \
    visitor.template resolve_designation<designation::desig>(read_uint32_t<safety>(bf, ip))

#define BINOP(opcode_name, enum_name, visitor)                                                     \
    case opcode_##opcode_name:                                                                     \
    {                                                                                              \
        aint y = visitor.pop_stack();                                                              \
        aint x = visitor.pop_stack();                                                              \
        visitor.push_stack(visitor.template binop<binop::enum_name>(x, y));                        \
        break;                                                                                     \
    }

#define PATTERN(pattern_name, enum_name, visitor)                                                  \
    case opcode_pattern_##pattern_name:                                                            \
    {                                                                                              \
        aint x = visitor.pop_stack();                                                              \
        visitor.push_stack(visitor.template pattern<pattern::enum_name>(x));                       \
        break;                                                                                     \
    }

#define INTERPRET_INSTRUCTION(bf, ip, visitor)                                                     \
    uint8_t opcode = read_byte<safety>(bf, ip);                                                    \
    switch (opcode)                                                                                \
    {                                                                                              \
        BINOP(add, add, visitor)                                                                   \
        BINOP(sub, sub, visitor)                                                                   \
        BINOP(mul, mul, visitor)                                                                   \
        BINOP(div, div, visitor)                                                                   \
        BINOP(rem, rem, visitor)                                                                   \
        BINOP(lt, lt, visitor)                                                                     \
        BINOP(leq, leq, visitor)                                                                   \
        BINOP(gt, gt, visitor)                                                                     \
        BINOP(geq, geq, visitor)                                                                   \
        BINOP(eq, eq, visitor)                                                                     \
        BINOP(neq, neq, visitor)                                                                   \
        BINOP(and, logical_and, visitor)                                                           \
        BINOP(or, logical_or, visitor)                                                             \
    case opcode_const:                                                                             \
        visitor.push_stack(BOX(read_int32_t<safety>(bf, ip)));                                     \
        break;                                                                                     \
    case opcode_string:                                                                            \
        visitor.push_stack(visitor.make_string(read_string<safety>(bf, ip)));                      \
        break;                                                                                     \
    case opcode_sexp:                                                                              \
    {                                                                                              \
        auto tag_contents = read_string<safety>(bf, ip);                                           \
        auto n = read_uint32_t<safety>(bf, ip);                                                    \
        visitor.push_stack(visitor.make_sexp(tag_contents, n));                                    \
        break;                                                                                     \
    }                                                                                              \
    case opcode_sta:                                                                               \
    {                                                                                              \
        aint v = visitor.pop_stack();                                                              \
        aint i = visitor.pop_stack();                                                              \
        aint x = visitor.pop_stack();                                                              \
        visitor.push_stack(visitor.sta(x, i, v));                                                  \
        break;                                                                                     \
    }                                                                                              \
    case opcode_jmp:                                                                               \
        visitor.visit_jmp(read_fixup<safety>(bf, ip));                                             \
        break;                                                                                     \
    case opcode_end:                                                                               \
    case opcode_ret:                                                                               \
        visitor.visit_ret();                                                                       \
        break;                                                                                     \
    case opcode_drop:                                                                              \
        visitor.pop_stack();                                                                       \
        break;                                                                                     \
    case opcode_dup:                                                                               \
    {                                                                                              \
        aint value = visitor.pop_stack();                                                          \
        visitor.push_stack(value);                                                                 \
        visitor.push_stack(value);                                                                 \
        break;                                                                                     \
    }                                                                                              \
    case opcode_swap:                                                                              \
    {                                                                                              \
        aint y = visitor.pop_stack();                                                              \
        aint x = visitor.pop_stack();                                                              \
        visitor.push_stack(x);                                                                     \
        visitor.push_stack(y);                                                                     \
        break;                                                                                     \
    }                                                                                              \
    case opcode_elem:                                                                              \
    {                                                                                              \
        auto i = visitor.pop_stack();                                                              \
        auto p = visitor.pop_stack();                                                              \
        visitor.push_stack(visitor.elem(p, i));                                                    \
    }                                                                                              \
    break;                                                                                         \
    case opcode_ld_global:                                                                         \
        visitor.push_stack(RESOLVE_DESIGNATION(global, visitor));                                  \
        break;                                                                                     \
    case opcode_ld_local:                                                                          \
        visitor.push_stack(RESOLVE_DESIGNATION(local, visitor));                                   \
        break;                                                                                     \
    case opcode_ld_arg:                                                                            \
        visitor.push_stack(RESOLVE_DESIGNATION(arg, visitor));                                     \
        break;                                                                                     \
    case opcode_ld_capture:                                                                        \
        visitor.push_stack(RESOLVE_DESIGNATION(capture, visitor));                                 \
        break;                                                                                     \
    case opcode_st_global:                                                                         \
        RESOLVE_DESIGNATION(global, visitor) = visitor.peek_stack();                               \
        break;                                                                                     \
    case opcode_st_local:                                                                          \
        RESOLVE_DESIGNATION(local, visitor) = visitor.peek_stack();                                \
        break;                                                                                     \
    case opcode_st_arg:                                                                            \
        RESOLVE_DESIGNATION(arg, visitor) = visitor.peek_stack();                                  \
        break;                                                                                     \
    case opcode_st_capture:                                                                        \
        RESOLVE_DESIGNATION(capture, visitor) = visitor.peek_stack();                              \
        break;                                                                                     \
    case opcode_cjmp_z:                                                                            \
        visitor.template visit_cjmp<condition::zero>(                                              \
            read_fixup<safety>(bf, ip), visitor.pop_stack()                                        \
        );                                                                                         \
        break;                                                                                     \
    case opcode_cjmp_nz:                                                                           \
        visitor.template visit_cjmp<condition::nonzero>(                                           \
            read_fixup<safety>(bf, ip), visitor.pop_stack()                                        \
        );                                                                                         \
        break;                                                                                     \
    case opcode_begin:                                                                             \
    case opcode_beginc:                                                                            \
    {                                                                                              \
        auto maxstack = read_uint32_t<safety>(bf, ip);                                             \
        visitor.visit_begin(maxstack, read_uint32_t<safety>(bf, ip));                              \
        break;                                                                                     \
    }                                                                                              \
    case opcode_closure:                                                                           \
    {                                                                                              \
        auto destination = read_fixup<safety>(bf, ip);                                             \
        auto capture_size = read_uint32_t<safety>(bf, ip);                                         \
        aint captured[capture_size];                                                               \
        for (uint32_t i = 0; i < capture_size; ++i)                                                \
        {                                                                                          \
            uint8_t desig = read_byte<safety>(bf, ip);                                             \
            switch (desig)                                                                         \
            {                                                                                      \
            case designation_global:                                                               \
                captured[i] = RESOLVE_DESIGNATION(global, visitor);                                \
                break;                                                                             \
            case designation_local:                                                                \
                captured[i] = RESOLVE_DESIGNATION(local, visitor);                                 \
                break;                                                                             \
            case designation_arg:                                                                  \
                captured[i] = RESOLVE_DESIGNATION(arg, visitor);                                   \
                break;                                                                             \
            case designation_capture:                                                              \
                captured[i] = RESOLVE_DESIGNATION(capture, visitor);                               \
                break;                                                                             \
            default:                                                                               \
                interpret_stage_failure(bf, "Invalid closure capture kind");                       \
            }                                                                                      \
        }                                                                                          \
        visitor.visit_closure(destination, capture_size, captured);                                \
        break;                                                                                     \
    }                                                                                              \
    case opcode_callc:                                                                             \
        visitor.visit_callc(read_uint32_t<safety>(bf, ip));                                        \
        break;                                                                                     \
    case opcode_call:                                                                              \
    {                                                                                              \
        auto destination = read_fixup<safety>(bf, ip);                                             \
        auto args_size = read_uint32_t<safety>(bf, ip);                                            \
        visitor.visit_call(destination, args_size);                                                \
        break;                                                                                     \
    }                                                                                              \
    case opcode_tag:                                                                               \
    {                                                                                              \
        auto tag_contents = read_string<safety>(bf, ip);                                           \
        auto args_count = read_uint32_t<safety>(bf, ip);                                           \
        auto checked_obj = visitor.pop_stack();                                                    \
        visitor.push_stack(visitor.make_tag(tag_contents, args_count, checked_obj));               \
        break;                                                                                     \
    }                                                                                              \
    case opcode_array:                                                                             \
    {                                                                                              \
        auto length = read_uint32_t<safety>(bf, ip);                                               \
        auto x = visitor.pop_stack();                                                              \
        visitor.push_stack(visitor.make_array_pattern(length, x));                                 \
        break;                                                                                     \
    }                                                                                              \
    case opcode_fail:                                                                              \
    {                                                                                              \
        auto arg1 = read_uint32_t<safety>(bf, ip);                                                 \
        auto arg2 = read_uint32_t<safety>(bf, ip);                                                 \
        visitor.visit_fail(arg1, arg2);                                                            \
        break;                                                                                     \
    }                                                                                              \
    case opcode_line:                                                                              \
        current_frame->current_line = BOX(read_uint32_t<safety>(bf, ip));                          \
        break;                                                                                     \
    case opcode_pattern_strcmp:                                                                    \
    {                                                                                              \
        auto x = visitor.pop_stack();                                                              \
        auto y = visitor.pop_stack();                                                              \
        visitor.push_stack(visitor.strcmp(x, y));                                                  \
        break;                                                                                     \
    }                                                                                              \
        PATTERN(string, string_tag, visitor)                                                       \
        PATTERN(array, array_tag, visitor)                                                         \
        PATTERN(sexp, sexp_tag, visitor)                                                           \
        PATTERN(boxed, boxed, visitor)                                                             \
        PATTERN(unboxed, unboxed, visitor)                                                         \
        PATTERN(closure, closure_tag, visitor)                                                     \
    case opcode_builtin_read:                                                                      \
        visitor.push_stack(visitor.read());                                                        \
        break;                                                                                     \
    case opcode_builtin_write:                                                                     \
        visitor.visit_write(visitor.pop_stack());                                                  \
        visitor.push_stack(BOX(0));                                                                \
        break;                                                                                     \
    case opcode_builtin_length:                                                                    \
        visitor.push_stack(visitor.length(visitor.pop_stack()));                                   \
        break;                                                                                     \
    case opcode_builtin_string:                                                                    \
        visitor.push_stack(visitor.make_string(visitor.pop_stack()));                              \
        break;                                                                                     \
    case opcode_builtin_array:                                                                     \
        visitor.push_stack(visitor.make_array(read_uint32_t<safety>(bf, ip)));                     \
        break;                                                                                     \
    default:                                                                                       \
        interpret_stage_failure(bf, "Unknown instruction 0x%02X", opcode);                         \
        break;                                                                                     \
    }

template <operation_safety safety, typename Visitor>
void interpret_instruction(bytefile* bf, uint8_t*& ip, Visitor visitor)
{
    INTERPRET_INSTRUCTION(bf, ip, visitor);
}

struct empty_visitor
{
    aint peek_stack()
    {
        return 0;
    }

    aint pop_stack()
    {
        return 0;
    }

    void push_stack(aint value)
    {
    }

    template <designation desig>
    aint& resolve_designation(uint32_t index)
    {
        static aint fake_designation = 0;
        return fake_designation;
    }

    template <binop op>
    aint binop(aint x, aint y)
    {
        return 0;
    }

    aint make_string(char* contents)
    {
        return 0;
    }

    aint make_sexp(char* tag_contents, uint32_t n)
    {
        return 0;
    }

    aint sta(aint x, aint i, aint v)
    {
        return 0;
    }

    void visit_jmp(uint8_t* address)
    {
    }

    void visit_ret()
    {
    }

    aint elem(aint p, aint i)
    {
        return 0;
    }

    template <condition cond>
    void visit_cjmp(uint8_t* address, aint value)
    {
    }

    void visit_begin(uint32_t maxstack, uint32_t locals_size)
    {
    }

    void visit_closure(uint8_t* destination, uint32_t capture_size, aint* captured)
    {
    }

    void visit_callc(uint32_t args_size)
    {
    }

    void visit_call(uint8_t* destination, uint32_t args_size)
    {
    }

    aint make_tag(char* tag_contents, uint32_t args_count, aint checked_obj)
    {
        return 0;
    }

    aint make_array_pattern(uint32_t length, aint x)
    {
        return 0;
    }

    void visit_fail(uint32_t arg1, uint32_t arg2)
    {
    }

    aint strcmp(aint x, aint y)
    {
        return 0;
    }

    template <pattern kind>
    aint pattern(aint obj)
    {
        return 0;
    }

    aint read()
    {
        return 0;
    }

    void visit_write(aint value)
    {
    }

    aint length(aint obj_ptr)
    {
        return 0;
    }

    aint make_string(aint obj_ptr)
    {
        return 0;
    }

    aint make_array(uint32_t length)
    {
        return 0;
    }
};

#endif
