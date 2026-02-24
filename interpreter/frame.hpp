#ifndef FRAME_HPP
#define FRAME_HPP

#include "assertions.hpp"
#include "bytefile.hpp"
#include "runtime.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

constexpr size_t global_stack_capacity = 256 * 1024;

/**
 * Represents info about a single callstack entry,
 * but does not store any data (capture, arguments, locals, stack).
 *
 * The data and the local_frame itself are stored in the fixed size global stack.
 * The stack looks like this (growing from left to right):
 * ... -> (previous frame stack element)* -> (closure object with captured data)? ->
 *  -> (argument)* -> local_frame -> (local)* -> (current frame stack element)*
 * Previous frame stack is unaccessible to current frame.
 *
 * In this configuration, when calling a function, the possible closure object
 * and arguments do not need to be moved in the global stack at all.
 * They can be removed after the callee returns.
 */
struct local_frame
{
    local_frame* prev;

    /**
     * This is not an actual IP. It points to the beginning
     * of currently executed instruction. Used for generating
     * failure backtraces.
     */
    uint8_t* current_instruction_ptr;

    /**
     * This points to the first element of the frame's stack,
     * even though it is not the beginning of the space
     * that the frame occupies in the global stack.
     * I believe this is more efficient for stack operations
     * (because we don't need to do extra arithmetic to find the stack base).
     */
    aint* stack_base;

    /**
     * Used for generating failure backtraces only.
     */
    aint current_line;

    aint capture_size;
    aint args_size;
    aint locals_size;
};

constexpr size_t local_frame_size_on_stack = sizeof(local_frame) / sizeof(aint);

extern local_frame* current_frame;
extern uint8_t* ip;

inline aint& global_at_unsafe(size_t index)
{
    return *((aint*)__gc_stack_top + 1 + index);
}

inline aint* frame_locals_begin()
{
    return current_frame->stack_base - UNBOX(current_frame->locals_size);
}

inline aint* frame_frame_begin()
{
    aint* locals = frame_locals_begin();
    return locals - local_frame_size_on_stack;
}

inline aint* frame_args_begin()
{
    aint* frame = frame_frame_begin();
    return frame - UNBOX(current_frame->args_size);
}

inline aint& capture_at_unsafe(size_t index)
{
    aint closure_ptr = *(frame_args_begin() - 1);
    data* closure = TO_DATA((void*)closure_ptr);
    return ((aint*)closure->contents)[index + 1];
}

inline aint& arg_at_unsafe(size_t index)
{
    return *(frame_args_begin() + index);
}

inline aint& local_at_unsafe(size_t index)
{
    return *(frame_locals_begin() + index);
}

inline size_t stack_size()
{
    return ((aint*)__gc_stack_bottom - current_frame->stack_base);
}

inline void check_stack_has_free(bytefile* bf, size_t n)
{
    if (((__gc_stack_bottom - __gc_stack_top) / sizeof(aint)) + n >= global_stack_capacity)
    {
        interpret_stage_failure(bf, "Stack overflow");
    }
}

inline aint& stack_at_unsafe(size_t index)
{
    return *(current_frame->stack_base + index);
}

inline aint& peek_stack_unsafe()
{
    return *((aint*)__gc_stack_bottom - 1);
}

inline aint pop_stack_unsafe()
{
    __gc_stack_bottom -= sizeof(aint);
    return *((aint*)__gc_stack_bottom);
}

inline aint pop_stack_safe(bytefile* bf)
{
    return pop_stack_unsafe();
}

inline void drop_stack_unsafe(size_t n)
{
    __gc_stack_bottom -= n * sizeof(aint);
}

inline void push_stack_unsafe(aint value)
{
    *((aint*)__gc_stack_bottom) = value;
    __gc_stack_bottom += sizeof(aint);
}

inline void push_stack_safe(bytefile* bf, aint value)
{
    check_stack_has_free(bf, 1);
    push_stack_unsafe(value);
}

inline local_frame* push_frame_safe(bytefile* bf)
{
    check_stack_has_free(bf, local_frame_size_on_stack);
    local_frame* new_frame = (local_frame*)__gc_stack_bottom;
    __gc_stack_bottom += sizeof(local_frame);
    return new_frame;
}

inline void
generic_call(bytefile* bf, uint32_t args_size, uint32_t capture_size, uint8_t* destination)
{
    auto new_frame = push_frame_safe(bf);

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

#endif
