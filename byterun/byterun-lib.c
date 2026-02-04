#include "byterun-lib.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        return NULL;
    }

    bytefile* result = NULL;
    long ftold_size;

    if (fseek(f, 0, SEEK_END) == -1 || (ftold_size = ftell(f)) < 0)
    {
        failure("Unable to get file size. Reason: %s", strerror(errno));
        goto bad;
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
        goto bad;
    }
    if ((ftold_size = ftell(f)) < 0)
    {
        failure("Unable to get file header size. Reason: %s", strerror(errno));
        goto bad;
    }
    size -= ftold_size;

    result = (bytefile*)malloc(sizeof(bytefile) + size);
    if (result == NULL)
    {
        failure("Unable to allocate memory for file content");
        goto bad;
    }
    result->stringtab_size = stringtab_size;
    result->global_area_size = global_area_size;
    result->public_symbols_number = public_symbols_number;

    if (fread(result->content, 1, size, f) != size)
    {
        failure("Unable to read file content");
        goto bad;
    }

    uint32_t public_area_size = result->public_symbols_number * 2 * sizeof(uint32_t);
    if (size < public_area_size)
    {
        failure("File content is too small for public area");
        goto bad;
    }

    size -= public_area_size;
    if (size < result->stringtab_size)
    {
        failure("File content is too small for string table");
        goto bad;
    }
    size -= result->stringtab_size;

    result->code_length = size;
    result->public_ptr = (uint32_t*)&result->content[0]; // alignment should be ok
                                                         // since content is after uint32_t fields
    result->string_ptr = (char*)&result->content[public_area_size];
    result->code_ptr = (uint8_t*)result->string_ptr + result->stringtab_size;

    return result;

bad:
    if (result != NULL)
    {
        free(result);
    }
    if (f != NULL)
    {
        fclose(f);
    }
    return NULL;
}

uint32_t disassemble_one(bytefile* f, uint32_t ip, FILE* output)
{
#define READ(buffer, length, what)                                                                 \
    if (read(f, ip, buffer, length, what))                                                         \
    {                                                                                              \
        goto stop;                                                                                 \
    }                                                                                              \
    ip += length;
#define BYTE(x)                                                                                    \
    uint8_t x;                                                                                     \
    READ(&x, 1, "byte")
#define INT32_T(x)                                                                                 \
    uint8_t buffer_##x[4];                                                                         \
    READ(buffer_##x, 4, "4-byte signed int");                                                      \
    int32_t x = (int32_t)le_bytes_to_uint32_t(buffer_##x);
#define UINT32_T(x)                                                                                \
    uint32_t x;                                                                                    \
    if (read_uint32_t(f, ip, &x))                                                                  \
    {                                                                                              \
        goto stop;                                                                                 \
    }                                                                                              \
    ip += 4;
#define STRING(x)                                                                                  \
    UINT32_T(str_pos_##x);                                                                         \
    char* x = get_string(f, str_pos_##x);                                                          \
    if (x == NULL)                                                                                 \
    {                                                                                              \
        failure("String position at %u is out of bounds\n", ip - 4);                               \
        goto stop;                                                                                 \
    }
#define FAIL                                                                                       \
    failure("ERROR: invalid opcode %d-%d\n", h, l);                                                \
    goto stop;
#define FAIL_IF(cond)                                                                              \
    if (cond)                                                                                      \
    {                                                                                              \
        FAIL;                                                                                      \
    }
#define PRINT_DESIGNATION(designation)                                                             \
    switch (designation)                                                                           \
    {                                                                                              \
    case 0:                                                                                        \
        PRINTF("G");                                                                               \
        break;                                                                                     \
    case 1:                                                                                        \
        PRINTF("L");                                                                               \
        break;                                                                                     \
    case 2:                                                                                        \
        PRINTF("A");                                                                               \
        break;                                                                                     \
    case 3:                                                                                        \
        PRINTF("C");                                                                               \
        break;                                                                                     \
    default:                                                                                       \
        FAIL;                                                                                      \
    }                                                                                              \
    UINT32_T(index);                                                                               \
    PRINTF("(%u)", index);
#define PRINTF(...) output == NULL ? 0 : fprintf(output, __VA_ARGS__)

    static char* ops[] = {"+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "&&", "!!"};
    static char* pats[] = {"=str", "#string", "#array", "#sexp", "#ref", "#val", "#fun"};
    static char* lds[] = {"LD", "LDA", "ST"};

    uint32_t initial_ip = ip;
    BYTE(opcode);
    uint8_t h = (opcode & 0xF0) >> 4;
    uint8_t l = opcode & 0x0F;

    switch (h)
    {
    case 15:
        goto stop;

    /* BINOP */
    case 0:
        FAIL_IF(l == 0 || (l - 1) >= sizeof(ops) / sizeof(ops[0]));
        PRINTF("BINOP\t%s", ops[l - 1]);
        break;

    case 1:
        switch (l)
        {
        case 0:
            INT32_T(const_value);
            PRINTF("CONST\t%d", const_value);
            break;

        case 1:
            STRING(string_content);
            PRINTF("STRING\t%s", string_content);
            break;

        case 2:
            STRING(sexp_tag);
            UINT32_T(sexp_size);
            PRINTF("SEXP\t%s ", sexp_tag);
            PRINTF("%u", sexp_size);
            break;

        case 3:
            PRINTF("STI");
            break;

        case 4:
            PRINTF("STA");
            break;

        case 5:
            UINT32_T(jmp_target);
            PRINTF("JMP\t0x%.8x", jmp_target);
            break;

        case 6:
            PRINTF("END");
            break;

        case 7:
            PRINTF("RET");
            break;

        case 8:
            PRINTF("DROP");
            break;

        case 9:
            PRINTF("DUP");
            break;

        case 10:
            PRINTF("SWAP");
            break;

        case 11:
            PRINTF("ELEM");
            break;

        default:
            FAIL;
        }
        break;

    case 2:
    case 3:
    case 4:
        PRINTF("%s\t", lds[h - 2]);
        PRINT_DESIGNATION(l);
        break;

    case 5:
        switch (l)
        {
        case 0:
            UINT32_T(cjmpz_target);
            PRINTF("CJMPz\t0x%.8x", cjmpz_target);
            break;

        case 1:
            UINT32_T(cjmpnz_target);
            PRINTF("CJMPnz\t0x%.8x", cjmpnz_target);
            break;

        case 2:
            UINT32_T(begin_maxstack);
            UINT32_T(begin_locals);
            PRINTF("BEGIN\t%u ", begin_maxstack);
            PRINTF("%u", begin_locals);
            break;

        case 3:
            UINT32_T(cbegin_maxstack);
            UINT32_T(cbegin_locals);
            PRINTF("CBEGIN\t%u ", cbegin_maxstack);
            PRINTF("%u", cbegin_locals);
            break;

        case 4:
            UINT32_T(closure_target);
            PRINTF("CLOSURE\t0x%.8x", closure_target);
            UINT32_T(closure_args);
            for (uint32_t i = 0; i < closure_args; i++)
            {
                BYTE(designation);
                PRINT_DESIGNATION(designation);
            }
            break;

        case 5:
            UINT32_T(callc_args);
            PRINTF("CALLC\t%u", callc_args);
            break;

        case 6:
            UINT32_T(call_target);
            UINT32_T(call_args);
            PRINTF("CALL\t0x%.8x ", call_target);
            PRINTF("%u", call_args);
            break;

        case 7:
            STRING(tag_tag);
            UINT32_T(tag_size);
            PRINTF("TAG\t%s ", tag_tag);
            PRINTF("%u", tag_size);
            break;

        case 8:
            UINT32_T(array_size);
            PRINTF("ARRAY\t%u", array_size);
            break;

        case 9:
            UINT32_T(fail_x1);
            UINT32_T(fail_x2);
            PRINTF("FAIL\t%u", fail_x1);
            PRINTF("%u", fail_x2);
            break;

        case 10:
            UINT32_T(line);
            PRINTF("LINE\t%u", line);
            break;

        default:
            FAIL;
        }
        break;

    case 6:
        FAIL_IF(l >= sizeof(pats) / sizeof(pats[0]));
        PRINTF("PATT\t%s", pats[l]);
        break;

    case 7:
        switch (l)
        {
        case 0:
            PRINTF("CALL\tLread");
            break;

        case 1:
            PRINTF("CALL\tLwrite");
            break;

        case 2:
            PRINTF("CALL\tLlength");
            break;

        case 3:
            PRINTF("CALL\tLstring");
            break;

        case 4:
            UINT32_T(x);
            PRINTF("CALL\tBarray\t%u", x);
            break;

        default:
            FAIL;
        }
        break;

    default:
        FAIL;
    }

    return ip - initial_ip;

stop:
    return 0;
}
