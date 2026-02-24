#ifndef BYTEFILE_HPP
#define BYTEFILE_HPP

#include "runtime.hpp"

extern uint32_t global_area_size;
extern aint* global_stack;
extern uint8_t* code_ptr;

aint& global_at_unsafe(size_t index);

void read_file(char* fname);

uint8_t next_code_byte();

uint32_t next_code_uint32_t();

int32_t next_code_int32_t();

char* next_code_string();

uint8_t* next_code_fixup();

aint& next_code_global();

aint& next_code_local();

aint& next_code_arg();

aint& next_code_capture();

#endif
