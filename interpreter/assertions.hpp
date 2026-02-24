#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "runtime.hpp"

char const* kind_description(aint kind);

char const* opcode_description(uint8_t opcode);

aint unbox_safe(aint value);

void assert_unboxed(aint value);

void assert_boxed(aint value);

void assert_kind(aint value, aint expected_kind);

void interpret_stage_failure(char const* msg, ...);

#endif
