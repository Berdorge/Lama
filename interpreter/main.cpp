#include "assertions.hpp"
#include "bytefile.hpp"
#include "frame.hpp"
#include "instructions.hpp"
#include "runtime.hpp"

#include <cstdio>
#include <cstring>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        INITIAL_STAGE_FAILURE("Usage: %s <bytecode_file>\n", argv[0]);
        return 1;
    }

    read_file(argv[1]);
    __gc_init();

    aint stack[global_stack_capacity + 2];
    memset(stack, 0, sizeof(stack));

    __gc_stack_top = (size_t)(&stack[1]) & ~0xF;
    __gc_stack_bottom = __gc_stack_top + sizeof(aint) + global_area_size * sizeof(aint);

    auto root_frame = push_frame_safe();

    root_frame->prev = nullptr;
    root_frame->current_instruction_ptr = nullptr;
    root_frame->stack_base = (aint*)__gc_stack_bottom;
    root_frame->current_line = BOX(1);
    root_frame->capture_size = BOX(0);
    root_frame->args_size = BOX(0);
    root_frame->locals_size = BOX(0);

    current_frame = root_frame;
    ip = code_ptr;
    run_instructions();

    return 0;
}
