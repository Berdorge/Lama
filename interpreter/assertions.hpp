#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "runtime.hpp"
#include "bytefile.hpp"

char const* kind_description(aint kind);

char const* opcode_description(uint8_t opcode);

aint unbox_safe(bytefile* bf, aint value);

void assert_unboxed(bytefile* bf, aint value);

void assert_boxed(bytefile* bf, aint value);

void assert_kind(bytefile* bf, aint value, aint expected_kind);

void interpret_stage_failure(bytefile* bf, char const* msg, ...);

#endif
