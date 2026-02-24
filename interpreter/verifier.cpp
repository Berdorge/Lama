#include "verifier.hpp"
#include "abstract_interpreter.hpp"
#include "assertions.hpp"
#include "frame.hpp"

#include <cassert>
#include <vector>

static void assert_begin(bytefile* bf, uint8_t* ip, char const* what)
{
    uint8_t opcode = read_byte<operation_safety::safe>(bf, ip);
    if (opcode != opcode_begin && opcode != opcode_beginc)
    {
        interpret_stage_failure(bf, "%s destination is not a BEGIN[C] instruction", what);
    }
}

static void update_begin_maxstack(uint8_t* address, uint32_t max_stack_size)
{
    uint32_t current_maxstack = le_bytes_to_uint32_t(address + 1);
    if (max_stack_size > current_maxstack)
    {
        uint32_t_to_le_bytes(max_stack_size, address + 1);
    }
}

template <designation desig>
static void check_desig_index(bytefile* bf, uint32_t index)
{
    switch (desig)
    {
    case designation::global:
        return check_index<operation_safety::safe>(bf, index, bf->global_area_size, "Global");
    case designation::local:
        return check_index<operation_safety::safe>(
            bf, index, UNBOX(current_frame->locals_size), "Local"
        );
    case designation::arg:
        return check_arg_index(bf, index);
    case designation::capture:
        return check_index<operation_safety::safe>(
            bf, index, UNBOX(current_frame->capture_size), "Capture"
        );
    }
}

struct controlflow_visitor : empty_visitor
{
    bytefile* bf;
    bool* should_continue;

    void visit_jmp(uint8_t* address)
    {
        ip = address;
    }

    void visit_ret()
    {
        if (stack_size())
        {
            ip = reinterpret_cast<uint8_t*>(pop_stack_unsafe());
        }
        else
        {
            *should_continue = false;
        }
    }

    template <condition cond>
    void visit_cjmp(uint8_t* address, aint value)
    {
        push_stack_safe(bf, reinterpret_cast<aint>(ip));
        ip = address;
    }

    void visit_closure(uint8_t* destination, uint32_t capture_size, aint* captured)
    {
        assert_begin(bf, destination, "Closure");
        visit_cjmp<condition::zero>(destination, 0);
    }

    void visit_call(uint8_t* destination, uint32_t args_size)
    {
        assert_begin(bf, destination, "Call");
        visit_cjmp<condition::zero>(destination, 0);
    }

    void visit_fail(uint32_t arg1, uint32_t arg2)
    {
        visit_ret();
    }
};

struct write_maxstack_visitor : empty_visitor
{
    bytefile* bf;
    uint32_t max_stack_size;

    void visit_closure(uint8_t* destination, uint32_t capture_size, aint* captured)
    {
        update_begin_maxstack(destination, max_stack_size);
    }

    void visit_call(uint8_t* destination, uint32_t args_size)
    {
        update_begin_maxstack(destination, max_stack_size);
    }
};

struct abstract_stack_visitor : empty_visitor
{
    bytefile* bf;
    bool* should_continue;

    aint pop_stack()
    {
        drop_stack(1);
        return 0;
    }

    void drop_stack(size_t n)
    {
        aint& stack_size = abstract_stack_size();
        if (stack_size < n)
        {
            interpret_stage_failure(bf, "Stack underflow");
        }
        stack_size -= n;
    }

    void push_stack(aint value)
    {
        grow_stack(1);
    }

    void grow_stack(size_t n)
    {
        aint& maxstack = max_stack_size();
        aint& stack_size = abstract_stack_size();
        stack_size += n;
        maxstack = std::max(maxstack, stack_size);
    }

    template <designation desig>
    aint& resolve_designation(uint32_t index)
    {
        check_desig_index<desig>(bf, index);
        return empty_visitor::resolve_designation<desig>(index);
    }

    aint make_sexp(char* tag_contents, uint32_t n)
    {
        grow_stack(1);
        drop_stack(n + 1);
        return 0;
    }

    void visit_jmp(uint8_t* address)
    {
        ip = address;
    }

    void visit_ret()
    {
        if (stack_size() > 2)
        {
            // other branch of cjmp
            ip = reinterpret_cast<uint8_t*>(pop_stack_unsafe());
            abstract_stack_size() = pop_stack_unsafe();
        }
        else if (current_frame->prev == nullptr)
        {
            *should_continue = false;
        }
        else
        {
            uint32_t maxstack = max_stack_size();
            drop_stack_unsafe(stack_size() + local_frame_size_on_stack);
            current_frame = current_frame->prev;
            ip = current_frame->current_instruction_ptr;
            interpret_instruction<operation_safety::unsafe>(
                bf, ip, write_maxstack_visitor{empty_visitor{}, bf, maxstack}
            );
            grow_stack(1); // return value or closure
        }
    }

    template <condition cond>
    void visit_cjmp(uint8_t* address, aint value)
    {
        push_stack_safe(bf, abstract_stack_size());
        push_stack_safe(bf, reinterpret_cast<aint>(ip));
        ip = address;
    }

    void visit_begin(uint32_t maxstack, uint32_t locals_size)
    {
        current_frame->locals_size = BOX(locals_size);
    }

    void visit_closure(uint8_t* destination, uint32_t capture_size, aint* captured)
    {
        grow_stack(capture_size + 1);
        drop_stack(capture_size + 1);
        assert_begin(bf, destination, "Closure");
        abstract_call(UINT32_MAX, capture_size, destination);
    }

    void visit_callc(uint32_t args_size)
    {
        drop_stack(args_size + 1);
        grow_stack(1); // return value
    }

    void visit_call(uint8_t* destination, uint32_t args_size)
    {
        assert_begin(bf, destination, "Call");
        drop_stack(args_size);
        abstract_call(args_size, 0, destination);
    }

    void visit_fail(uint32_t arg1, uint32_t arg2)
    {
        visit_ret();
    }

    aint make_array(uint32_t length)
    {
        drop_stack(length);
        return 0;
    }

    aint& max_stack_size()
    {
        return stack_at_unsafe(0);
    }

    aint& abstract_stack_size()
    {
        return stack_at_unsafe(1);
    }

    void abstract_call(uint32_t args_size, uint32_t capture_size, uint8_t* destination)
    {
        generic_call(bf, args_size, capture_size, destination);
        push_stack_safe(bf, 0); // max_stack_size()
        push_stack_safe(bf, 0); // abstract_stack_size()
    }
};

struct flow_merge
{
    uint32_t offset;
    uint32_t stack_size;

    bool operator<(flow_merge const& other) const
    {
        return offset < other.offset;
    }
};

static std::vector<bool> find_flow_merges(bytefile* bf, uint8_t* initial_ip)
{
    bool should_continue = true;
    std::vector<bool> visited(bf->code_length, false);
    std::vector<bool> is_flow_merge(bf->code_length, false);

    current_frame = nullptr;
    generic_call(bf, 0, 0, initial_ip);
    ip = initial_ip;

    while (should_continue)
    {
        uint32_t offset = ip - bf->code_ptr;
        controlflow_visitor visitor{empty_visitor{}, bf, &should_continue};

        if (visited[offset])
        {
            is_flow_merge[offset] = true;
            visitor.visit_ret();
            continue;
        }

        current_frame->current_instruction_ptr = ip;
        visited[offset] = true;
        interpret_instruction<operation_safety::safe>(bf, ip, visitor);
    }

    return is_flow_merge;
}

static std::vector<flow_merge> build_flow_merges(std::vector<bool> const& is_flow_merge)
{
    std::vector<flow_merge> flow_merges;
    for (size_t i = 0; i < is_flow_merge.size(); ++i)
    {
        if (is_flow_merge[i])
        {
            flow_merge m;
            m.offset = i;
            m.stack_size = UINT32_MAX;
            flow_merges.push_back(m);
        }
    }
    return flow_merges;
}

static flow_merge& find_merge(bytefile* bf, std::vector<flow_merge>& flow_merges, uint32_t offset)
{
    return *std::lower_bound(flow_merges.begin(), flow_merges.end(), flow_merge{offset, 0});
}

static void
verify_stack_safety(bytefile* bf, uint8_t* initial_ip, std::vector<bool> const& is_flow_merge)
{
    auto flow_merges = build_flow_merges(is_flow_merge);
    bool should_continue = true;
    abstract_stack_visitor visitor{empty_visitor{}, bf, &should_continue};

    current_frame = nullptr;
    visitor.abstract_call(0, 0, initial_ip);
    ip = initial_ip;

    while (should_continue)
    {
        uint32_t offset = ip - bf->code_ptr;
        current_frame->current_instruction_ptr = ip;

        if (is_flow_merge[offset])
        {
            flow_merge& m = find_merge(bf, flow_merges, offset);
            if (m.stack_size == UINT32_MAX)
            {
                m.stack_size = visitor.abstract_stack_size();
            }
            else
            {
                if (m.stack_size != visitor.abstract_stack_size())
                {
                    interpret_stage_failure(bf, "Inconsistent stack size at flow merge");
                }
                visitor.visit_ret();
                continue;
            }
        }

        interpret_instruction<operation_safety::safe>(bf, ip, visitor);
    }

    update_begin_maxstack(initial_ip, visitor.max_stack_size());
}

void verify(bytefile* bf, uint8_t* initial_ip, aint* stack)
{
    __gc_stack_top = reinterpret_cast<size_t>(stack);
    __gc_stack_bottom = __gc_stack_top;

    assert_begin(bf, initial_ip, "Entrypoint");

    auto is_flow_merge = find_flow_merges(bf, initial_ip);
    verify_stack_safety(bf, initial_ip, is_flow_merge);
}
