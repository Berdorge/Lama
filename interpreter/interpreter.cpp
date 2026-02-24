#include "abstract_interpreter.hpp"
#include "assertions.hpp"
#include "bytefile.hpp"
#include "frame.hpp"
#include "runtime.hpp"

#include <cstdio>
#include <cstring>

template <designation desig>
struct designation_traits
{
};

template <binop op>
struct binop_traits
{
};

template <condition cond>
struct condition_traits
{
};

template <pattern kind>
struct pattern_traits
{
};

#define DESIGNATION_TRAITS(desig, check)                                                           \
    template <>                                                                                    \
    struct designation_traits<designation::desig>                                                  \
    {                                                                                              \
        static aint& resolve(bytefile* bf, uint32_t index)                                         \
        {                                                                                          \
            check;                                                                                 \
            return desig##_at_unsafe(index);                                                       \
        }                                                                                          \
    }

#define BINOP_TRAITS(name, op, check_args)                                                         \
    template <>                                                                                    \
    struct binop_traits<binop::name>                                                               \
    {                                                                                              \
        static aint apply(bytefile* bf, aint x, aint y)                                            \
        {                                                                                          \
            check_args(bf, x, y);                                                                  \
            return BOX(x op y);                                                                    \
        }                                                                                          \
    }

#define CONDITION_TRAITS(cond)                                                                     \
    template <>                                                                                    \
    struct condition_traits<condition::cond>                                                       \
    {                                                                                              \
        static bool check(aint value)                                                              \
        {                                                                                          \
            return (condition::cond == condition::zero) ? (value == 0) : (value != 0);             \
        }                                                                                          \
    }

#define PATTERN_TRAITS(kind)                                                                       \
    template <>                                                                                    \
    struct pattern_traits<pattern::kind>                                                           \
    {                                                                                              \
        static aint apply(bytefile* bf, aint x)                                                    \
        {                                                                                          \
            return reinterpret_cast<aint>(B##kind##_patt((void*)x));                               \
        }                                                                                          \
    }

static void check_args_eq(bytefile* bf, aint x, aint y)
{
}

static void check_args_unboxed(bytefile* bf, aint& x, aint& y)
{

    x = unbox_safe(bf, x);
    y = unbox_safe(bf, y);
}

static void check_args_div(bytefile* bf, aint& x, aint& y)
{
    check_args_unboxed(bf, x, y);
    if (y == 0)
    {
        interpret_stage_failure(bf, "Division by zero");
    }
}

DESIGNATION_TRAITS(global, {});
DESIGNATION_TRAITS(local, {});
DESIGNATION_TRAITS(arg, check_arg_index(bf, index));
DESIGNATION_TRAITS(capture, {});

BINOP_TRAITS(add, +, check_args_unboxed);
BINOP_TRAITS(sub, -, check_args_unboxed);
BINOP_TRAITS(mul, *, check_args_unboxed);
BINOP_TRAITS(div, /, check_args_div);
BINOP_TRAITS(rem, %, check_args_div);
BINOP_TRAITS(lt, <, check_args_unboxed);
BINOP_TRAITS(leq, <=, check_args_unboxed);
BINOP_TRAITS(gt, >, check_args_unboxed);
BINOP_TRAITS(geq, >=, check_args_unboxed);
BINOP_TRAITS(eq, ==, check_args_eq);
BINOP_TRAITS(neq, !=, check_args_unboxed);
BINOP_TRAITS(logical_and, &&, check_args_unboxed);
BINOP_TRAITS(logical_or, ||, check_args_unboxed);

CONDITION_TRAITS(zero);
CONDITION_TRAITS(nonzero);

PATTERN_TRAITS(string_tag);
PATTERN_TRAITS(array_tag);
PATTERN_TRAITS(sexp_tag);
PATTERN_TRAITS(boxed);
PATTERN_TRAITS(unboxed);
PATTERN_TRAITS(closure_tag);

#define READ_CALLC_ARGS auto args_size = read_uint32_t<operation_safety::unsafe>(bf, ip);

#define READ_CALL_ARGS                                                                             \
    auto destination = read_fixup<operation_safety::unsafe>(bf, ip);                               \
    auto args_size = read_uint32_t<operation_safety::unsafe>(bf, ip);

struct interpreter_visitor
{
    bytefile* bf;
    bool* should_continue;

    aint peek_stack()
    {
        return peek_stack_unsafe();
    }

    aint pop_stack()
    {
        return pop_stack_unsafe();
    }

    void push_stack(aint value)
    {
        push_stack_unsafe(value);
    }

    template <designation desig>
    aint& resolve_designation(uint32_t index)
    {
        return designation_traits<desig>::resolve(bf, index);
    }

    template <binop op>
    aint binop(aint x, aint y)
    {
        return binop_traits<op>::apply(bf, x, y);
    }

    aint make_string(char* contents)
    {
        return reinterpret_cast<aint>(Bstring(&contents));
    }

    aint make_sexp(char* tag_contents, uint32_t n)
    {
        auto tag_hash = LtagHash(tag_contents);
        push_stack_unsafe(tag_hash);
        auto result = Bsexp(&stack_at_unsafe(stack_size() - (n + 1)), BOX(n + 1));
        drop_stack_unsafe(n + 1);
        return reinterpret_cast<aint>(result);
    }

    aint sta(aint x, aint i, aint v)
    {
        assert_unboxed(bf, i);
        assert_boxed(bf, x);
        return reinterpret_cast<aint>(Bsta((void*)x, i, (void*)v));
    }

    void visit_jmp(uint8_t* address)
    {
        ip = address;
    }

    void visit_ret()
    {
        if (current_frame->prev == nullptr)
        {
            *should_continue = false;
            return;
        }

        local_frame* popped_frame = current_frame;

        auto return_value = pop_stack_safe(bf);
        drop_stack_unsafe(
            stack_size() + UNBOX(popped_frame->locals_size) + local_frame_size_on_stack
        );

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

    aint elem(aint p, aint i)
    {
        assert_unboxed(bf, i);
        assert_boxed(bf, p);
        return reinterpret_cast<aint>(Belem((void*)p, i));
    }

    template <condition cond>
    void visit_cjmp(uint8_t* address, aint value)
    {
        bool jump = condition_traits<cond>::check(unbox_safe(bf, value));
        if (jump)
        {
            ip = address;
        }
    }

    void visit_begin(uint32_t maxstack, uint32_t locals_size)
    {
        check_stack_has_free(bf, maxstack + locals_size);
        for (size_t i = 0; i < locals_size; ++i)
        {
            push_stack_unsafe(0);
        }

        current_frame->locals_size = BOX(locals_size);
        current_frame->stack_base += locals_size;
    }

    void visit_closure(uint8_t* destination, uint32_t capture_size, aint* captured)
    {
        push_stack_unsafe((aint)destination);
        for (uint32_t i = 0; i < capture_size; i++)
        {
            push_stack_unsafe(captured[i]);
        }
        auto result =
            Bclosure(&stack_at_unsafe(stack_size() - (capture_size + 1)), BOX(capture_size + 1));
        drop_stack_unsafe(capture_size + 1);
        push_stack_unsafe(reinterpret_cast<aint>(result));
    }

    void visit_callc(uint32_t args_size)
    {
        aint closure_ptr = stack_at_unsafe(stack_size() - (args_size + 1));
        data* closure = TO_DATA((void*)closure_ptr);
        size_t capture_size = LEN(closure->data_header) - 1;

        generic_call(bf, args_size, capture_size, (uint8_t*)((aint*)closure->contents)[0]);
    }

    void visit_call(uint8_t* destination, uint32_t args_size)
    {
        generic_call(bf, args_size, 0, destination);
    }

    aint make_tag(char* tag_contents, uint32_t args_count, aint checked_obj)
    {
        auto tag_hash = LtagHash(tag_contents);
        return reinterpret_cast<aint>(Btag((void*)checked_obj, tag_hash, BOX(args_count)));
    }

    aint make_array_pattern(uint32_t length, aint x)
    {
        return reinterpret_cast<aint>(Barray_patt((void*)x, BOX(length)));
    }

    void visit_fail(uint32_t arg1, uint32_t arg2)
    {
        interpret_stage_failure(bf, "Encountered FAIL instruction");
    }

    aint strcmp(aint x, aint y)
    {
        assert_kind(bf, x, STRING_TAG);
        assert_kind(bf, y, STRING_TAG);
        auto result = Bstring_patt((void*)x, (void*)y);
        return static_cast<aint>(result);
    }

    template <pattern kind>
    aint pattern(aint x)
    {
        return pattern_traits<kind>::apply(bf, x);
    }

    aint read()
    {
        printf(" ");
        return Lread();
    }

    void visit_write(aint x)
    {
        assert_unboxed(bf, x);
        Lwrite(x);
    }

    aint length(aint obj_ptr)
    {
        assert_boxed(bf, obj_ptr);
        auto len = Llength((void*)obj_ptr);
        return len;
    }

    aint make_string(aint obj_ptr)
    {
        auto obj_str_ptr = Lstring(&obj_ptr);
        return reinterpret_cast<aint>(obj_str_ptr);
    }

    aint make_array(uint32_t length)
    {
        auto array = Barray(&stack_at_unsafe(stack_size() - length), BOX(length));
        drop_stack_unsafe(length);
        return reinterpret_cast<aint>(array);
    }
};

void interpret(bytefile* bf, uint8_t* initial_ip, aint* stack)
{
    constexpr operation_safety safety = operation_safety::unsafe;

    __gc_init();
    __gc_stack_top = (size_t)(&stack[1]) & ~0xF;
    __gc_stack_bottom = __gc_stack_top + sizeof(aint) + bf->global_area_size * sizeof(aint);
    memset(stack, 0, sizeof(aint) * (global_stack_capacity + 2));

    current_frame = nullptr;
    generic_call(bf, 0, 0, initial_ip);

    bool should_continue = true;
    for (ip = initial_ip; should_continue;)
    {
        current_frame->current_instruction_ptr = ip;
        interpreter_visitor visitor{bf, &should_continue};

        INTERPRET_INSTRUCTION(bf, ip, visitor);
    }
}
