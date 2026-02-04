#ifndef BYTERUN_LIB_H
#define BYTERUN_LIB_H

#include <stdint.h>
#include <stdio.h>

/* The unpacked representation of bytecode file */
typedef struct
{
    char* string_ptr;               /* A pointer to the beginning of the string table */
    uint32_t* public_ptr;           /* A pointer to the beginning of publics table    */
    uint8_t* code_ptr;              /* A pointer to the bytecode itself               */
    uint32_t stringtab_size;        /* The size (in bytes) of the string table        */
    uint32_t global_area_size;      /* The size (in words) of the global area         */
    uint32_t public_symbols_number; /* The number of public symbols                   */
    uint32_t code_length;
    uint8_t content[0];
} bytefile;

void failure(char* s, ...);

/* Reads a binary bytecode file by name and unpacks it */
bytefile* read_file(char* fname);

inline char* get_string(bytefile* f, uint32_t pos)
{
    if (pos >= f->stringtab_size)
    {
        return NULL;
    }
    return &f->string_ptr[pos];
}

/* Gets an offset for a public symbol */
inline uint32_t get_public_offset(bytefile* f, uint32_t i)
{
    return f->public_ptr[i * 2 + 1];
}

inline int read(bytefile* bf, uint32_t ip, uint8_t* buffer, size_t length, char const* what)
{
    if (ip + length > bf->code_length)
    {
        failure("Unexpected end of bytecode while reading %s\n", what);
        return -1;
    }
    for (size_t i = 0; i < length; i++)
    {
        buffer[i] = bf->code_ptr[ip + i];
    }
    return 0;
}

inline uint32_t le_bytes_to_uint32_t(uint8_t const* bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

inline int read_uint32_t(bytefile* bf, uint32_t ip, uint32_t* value)
{
    uint8_t buffer[4];
    if (read(bf, ip, buffer, 4, "4-byte unsigned int"))
    {
        return -1;
    }
    *value = le_bytes_to_uint32_t(buffer);
    return 0;
}

/**
 * Returns the length of the disassembled instruction.
 *
 * Return value of 0 indicates that either
 * - stop instruction is reached
 * - `failure` has been called
 */
uint32_t disassemble_one(bytefile* f, uint32_t ip, FILE* output);

#endif
