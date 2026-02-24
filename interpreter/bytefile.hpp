#ifndef BYTEFILE_HPP
#define BYTEFILE_HPP

#include "runtime.hpp"

#include <cstdint>
#include <cstdio>

typedef struct
{
    char* string_ptr;
    uint32_t* public_ptr;
    uint8_t* code_ptr;
    uint32_t stringtab_size;
    uint32_t global_area_size;
    uint32_t public_symbols_number;
    uint32_t code_length;
    uint8_t content[0];
} bytefile;

bytefile* read_file(char* fname);

inline uint32_t le_bytes_to_uint32_t(uint8_t const* bytes)
{
    return static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) |
           (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
}

inline void uint32_t_to_le_bytes(uint32_t value, uint8_t* bytes)
{
    bytes[0] = static_cast<uint8_t>(value & 0xFF);
    bytes[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    bytes[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    bytes[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

#endif
