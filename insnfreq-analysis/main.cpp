#include "analyzer.hpp"
#include "assertions.hpp"
#include "bytefile.hpp"

#include <cstdarg>
#include <string>

constexpr uint8_t opcode_jmp = 0x15;
constexpr uint8_t opcode_end = 0x16;
constexpr uint8_t opcode_ret = 0x17;

constexpr uint8_t opcode_cjmp_z = 0x50;
constexpr uint8_t opcode_cjmp_nz = 0x51;
constexpr uint8_t opcode_closure = 0x54;
constexpr uint8_t opcode_callc = 0x55;
constexpr uint8_t opcode_call = 0x56;

constexpr uint8_t opcode_fail = 0x59;

constexpr uint8_t opcode_stop = 0xFF;

struct handler
{
    bytefile* bf;

    instruction_result describe_flow(reader_t& reader)
    {
        instruction_result result;
        result.target = UINT32_MAX;
        result.flow = instruction_flow::normal;

        uint8_t opcode;
        read(bf, reader.ip, &opcode, 1, "opcode");

        switch (opcode)
        {
        case opcode_jmp:
        case opcode_end:
        case opcode_ret:
        case opcode_fail:
        case opcode_stop:
            result.flow = instruction_flow::stop;
            break;

        case opcode_call:
        case opcode_callc:
            result.flow = instruction_flow::call;
            break;
        }

        switch (opcode)
        {
        case opcode_jmp:
        case opcode_cjmp_z:
        case opcode_cjmp_nz:
        case opcode_call:
        case opcode_closure:
            read_uint32_t(bf, reader.ip + 1, &result.target);
            if (result.target >= bf->code_length)
            {
                fprintf(stderr, "Target offset of instruction at %u is out of bounds: ", reader.ip);
                print(reader, stderr);
                failure("");
            }
            break;
        }

        return result;
    }

    void print(reader_t& reader, FILE* file)
    {
        size_t length = disassemble_one(bf, reader.ip, file);
        reader.advance(length);
    }
};

extern "C" void failure(char* s, ...)
{
    va_list args;
    va_start(args, s);
    vfprintf(stderr, s, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

int main(int argc, char* argv[])
{
    uint32_t output_threshold = 1;
    char* input_file = nullptr;

    for (int i = 1; i < argc;)
    {
        std::string arg = argv[i];
        if (arg == "--threshold")
        {
            output_threshold = std::stoul(argv[i + 1]);
            i += 2;
        }
        else if (arg == "--input")
        {
            input_file = argv[i + 1];
            i += 2;
        }
        else
        {
            failure("Unknown argument: %s", argv[i]);
        }
    }

    if (input_file == nullptr)
    {
        failure("--input file not specified");
    }

    bytefile* bf = read_file(input_file);

    uint32_t max_entries = bf->code_length / 5 + 256 + bf->code_length / 3 + 65536;
    analyzer analyzer(bf->code_ptr, bf->code_length, max_entries, handler{bf});

    for (uint32_t i = 0; i < bf->public_symbols_number; ++i)
    {
        uint32_t symbol_offset = get_public_offset(bf, i);
        if (symbol_offset >= bf->code_length)
        {
            failure("Offset of public symbol #%u is out of bounds", i);
        }
        analyzer.find_reachable(symbol_offset);
    }

    analyzer.count_occurrences();
    analyzer.print_hashtable(output_threshold);
}
