#include "bytefile.hpp"
#include "assertions.hpp"
#include "frame.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

uint32_t stringtab_size;
uint32_t global_area_size;
uint32_t public_symbols_number;
size_t code_length;
char* string_ptr;
uint8_t* code_ptr;
aint* global_stack;

static int read_uint32_t_from_file(FILE* f, uint32_t* out)
{
    uint8_t bytes[4];
    if (fread(bytes, 1, 4, f) != 4)
    {
        return -1;
    }
    *out = le_bytes_to_uint32_t(bytes);
    return 0;
}

bytefile* read_file(char* fname)
{
    FILE* f = fopen(fname, "rb");
    if (f == NULL)
    {
        failure("Unable to open file. Reason: %s", fname, strerror(errno));
    }

    bytefile* result = NULL;
    long ftold_size;

    if (fseek(f, 0, SEEK_END) == -1 || (ftold_size = ftell(f)) < 0)
    {
        failure("Unable to get file size. Reason: %s", strerror(errno));
    }
    rewind(f);

    size_t size = ftold_size;
    uint32_t stringtab_size;
    uint32_t global_area_size;
    uint32_t public_symbols_number;
    if (read_uint32_t_from_file(f, &stringtab_size) ||
        read_uint32_t_from_file(f, &global_area_size) ||
        read_uint32_t_from_file(f, &public_symbols_number))
    {
        failure("Unable to read file header");
    }
    if ((ftold_size = ftell(f)) < 0)
    {
        failure("Unable to get file header size. Reason: %s", strerror(errno));
    }
    size -= ftold_size;

    result = (bytefile*)malloc(sizeof(bytefile) + size);
    if (result == NULL)
    {
        failure("Unable to allocate memory for file content");
    }
    result->stringtab_size = stringtab_size;
    result->global_area_size = global_area_size;
    result->public_symbols_number = public_symbols_number;

    if (fread(result->content, 1, size, f) != size)
    {
        failure("Unable to read file content");
    }

    uint32_t public_area_size = result->public_symbols_number * 2 * sizeof(uint32_t);
    if (size < public_area_size)
    {
        failure("File content is too small for public area");
    }

    size -= public_area_size;
    if (size < result->stringtab_size)
    {
        failure("File content is too small for string table");
    }
    size -= result->stringtab_size;

    result->code_length = size;
    result->public_ptr = (uint32_t*)&result->content[0]; // alignment should be ok
                                                         // since content is after uint32_t fields
    result->string_ptr = (char*)&result->content[public_area_size];
    result->code_ptr = (uint8_t*)result->string_ptr + result->stringtab_size;

    fclose(f);

    return result;
}
