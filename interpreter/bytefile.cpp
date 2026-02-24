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

static int read_uint32_t(FILE* f, uint32_t& out)
{
    uint8_t bytes[4];
    if (fread(bytes, 1, 4, f) != 4)
    {
        return -1;
    }

    out = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16) |
          ((uint32_t)bytes[3] << 24);
    return 0;
}

aint& global_at_unsafe(size_t index)
{
    return *((aint*)__gc_stack_top + 1 + index);
}

void read_file(char* fname)
{
    FILE* f = fopen(fname, "rb");

    if (f == 0)
    {
        INITIAL_STAGE_FAILURE("Unable to open input file");
    }

    long ftold_size;
    if (fseek(f, 0, SEEK_END) == -1 || (ftold_size = ftell(f)) < 0)
    {
        INITIAL_STAGE_FAILURE("Unable to get input file size. Reason: %s", strerror(errno));
    }
    rewind(f);

    size_t size = ftold_size;
    char* contents = (char*)malloc(size);
    if (contents == 0)
    {
        INITIAL_STAGE_FAILURE("Unable to allocate memory for input file");
    }

    if (read_uint32_t(f, stringtab_size) || read_uint32_t(f, global_area_size) ||
        read_uint32_t(f, public_symbols_number))
    {
        INITIAL_STAGE_FAILURE("Unable to read input file header");
    }
    if ((ftold_size = ftell(f)) < 0)
    {
        INITIAL_STAGE_FAILURE("Unable to get input file header size. Reason: %s", strerror(errno));
    }
    size -= ftold_size;

    if (fread(contents, 1, size, f) != size)
    {
        INITIAL_STAGE_FAILURE("Unable to read input file content");
    }

    size_t public_area_size = public_symbols_number * 2 * sizeof(uint32_t);
    if (size < public_area_size)
    {
        INITIAL_STAGE_FAILURE("Input file content is too small for public area");
    }
    size -= public_area_size;

    if (size < stringtab_size)
    {
        INITIAL_STAGE_FAILURE("Input file content is too small for string table");
    }
    size -= stringtab_size;

    code_length = size;
    string_ptr = &contents[public_area_size];
    code_ptr = (uint8_t*)(string_ptr + stringtab_size);

    fclose(f);
}

static void check_code_has(size_t n, char const* what)
{
    if ((ip - code_ptr) + n > code_length)
    {
        size_t offset = ip - current_frame->current_instruction_ptr;
        interpret_stage_failure("Expected %s at offset %zu, got end of bytecode", what, offset);
    }
}

uint8_t next_code_byte()
{
    check_code_has(1, "byte");
    uint8_t value = ip[0];
    ip += 1;
    return value;
}

uint32_t next_code_uint32_t()
{
    check_code_has(4, "4-byte int");
    uint32_t value = (uint32_t)ip[0] | ((uint32_t)ip[1] << 8) | ((uint32_t)ip[2] << 16) |
                     ((uint32_t)ip[3] << 24);
    ip += 4;
    return value;
}

int32_t next_code_int32_t()
{
    int32_t value = next_code_uint32_t();
    return value;
}

static uint32_t next_code_index(size_t size, char const* what)
{
    auto index = next_code_uint32_t();
    if (index >= size)
    {
        size_t offset = ip - current_frame->current_instruction_ptr - 4;
        interpret_stage_failure("%s at offset %zu is out of bounds", what, offset);
    }
    return index;
}

char* next_code_string()
{
    auto pos = next_code_index(stringtab_size, "String position");
    return &string_ptr[pos];
}

uint8_t* next_code_fixup()
{
    auto offset = next_code_index(code_length, "Fixup offset");
    return code_ptr + offset;
}

aint& next_code_global()
{
    auto index = next_code_index(global_area_size, "Global area index");
    return global_at_unsafe(index);
}

aint& next_code_local()
{
    auto index = next_code_index(UNBOX(current_frame->locals_size), "Local area index");
    return local_at_unsafe(index);
}

aint& next_code_arg()
{
    auto index = next_code_index(UNBOX(current_frame->args_size), "Argument area index");
    return arg_at_unsafe(index);
}

aint& next_code_capture()
{
    auto index = next_code_index(UNBOX(current_frame->capture_size), "Closure area index");
    return capture_at_unsafe(index);
}
