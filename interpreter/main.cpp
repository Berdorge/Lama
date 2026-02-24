#include "abstract_interpreter.hpp"
#include "bytefile.hpp"
#include "frame.hpp"
#include "interpreter.hpp"
#include "verifier.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

#define ENTRYPOINT "main"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        INITIAL_STAGE_FAILURE("Usage: %s <bytecode_file>\n", argv[0]);
        return 1;
    }

    bytefile* bf = read_file(argv[1]);
    aint stack[global_stack_capacity + 2];

    for (uint32_t i = 0; i < bf->public_symbols_number; ++i)
    {
        if (std::string(get_public_name(bf, i)) == ENTRYPOINT)
        {
            auto begin = std::chrono::steady_clock::now();

            verify(bf, get_public_ip(bf, i), stack);

            auto end = std::chrono::steady_clock::now();
            auto verify_duration =
                std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
            fprintf(stderr, "Verifier took %lld us\n", verify_duration.count());

            interpret(bf, get_public_ip(bf, i), stack);

            return 0;
        }
    }

    failure("Public symbol \"" ENTRYPOINT "\" not found");
}
