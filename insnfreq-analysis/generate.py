import random
import instructions
import struct
import sys

if len(sys.argv) != 2:
    print("Usage: generate.py <min_file_size>")
    sys.exit(1)

min_file_size = int(sys.argv[1])

instruction_lengths = {
    "ADD": 0,
    "SUB": 0,
    "MUL": 0,
    "DIV": 0,
    "REM": 0,
    "LT": 0,
    "LEQ": 0,
    "GT": 0,
    "GEQ": 0,
    "EQ": 0,
    "NEQ": 0,
    "AND": 0,
    "OR": 0,
    "CONST": 4,
    "STRING": 4,
    "SEXP": 8,
    "STA": 0,
    "DROP": 0,
    "DUP": 0,
    "SWAP": 0,
    "ELEM": 0,
    "LD_GLOBAL": 4,
    "LD_LOCAL": 4,
    "LD_ARG": 4,
    "LD_CAPTURE": 4,
    "ST_GLOBAL": 4,
    "ST_LOCAL": 4,
    "ST_ARG": 4,
    "ST_CAPTURE": 4,
    "CJMP_Z": 4,
    "CJMP_NZ": 4,
    "BEGIN": 8,
    "BEGINC": 8,
    "CLOSURE": 8,
    "CALLC": 4,
    "CALL": 8,
    "TAG": 8,
    "ARRAY": 4,
    "LINE": 4,
    "PATTERN_STRCMP": 0,
    "PATTERN_STRING": 0,
    "PATTERN_ARRAY": 0,
    "PATTERN_SEXP": 0,
    "PATTERN_BOXED": 0,
    "PATTERN_UNBOXED": 0,
    "PATTERN_CLOSURE": 0,
    "BUILTIN_READ": 0,
    "BUILTIN_WRITE": 0,
    "BUILTIN_LENGTH": 0,
    "BUILTIN_STRING": 0,
    "BUILTIN_ARRAY": 4,
}

instruction_mnemonics = list(instruction_lengths.keys())

sys.stdout.buffer.write(struct.pack("i", 1))
sys.stdout.buffer.write(struct.pack("i", 0))
sys.stdout.buffer.write(struct.pack("i", 1))
sys.stdout.buffer.write(struct.pack("i", 0))
sys.stdout.buffer.write(struct.pack("i", 0))
sys.stdout.buffer.write(bytes([0]))

code = bytearray()
insn_begins = []

while len(code) < min_file_size:
    insn_begins.append(len(code))
    random_insn = random.randint(0, len(instruction_mnemonics) - 1)
    mnemonic = instruction_mnemonics[random_insn]
    code.append(instructions.by_mnemonic[mnemonic][0])
    arg_length = instruction_lengths[mnemonic]
    code.extend(random.randbytes(arg_length))

for begin in insn_begins:
    mnemonic = instructions.by_opcode[code[begin]][0]
    if mnemonic in ["JMP", "CJMP_Z", "CJMP_NZ", "CALL", "CLOSURE"]:
        target = random.choice(insn_begins)
        struct.pack_into("i", code, begin + 1, target)
    if mnemonic in ["CLOSURE"]:
        struct.pack_into("i", code, begin + 5, 0)
    if mnemonic in ["STRING"]:
        struct.pack_into("i", code, begin + 1, 0)
    if mnemonic in ["SEXP"]:
        struct.pack_into("i", code, begin + 1, 0)
    if mnemonic in ["TAG"]:
        struct.pack_into("i", code, begin + 1, 0)

sys.stdout.buffer.write(code)
sys.stdout.buffer.write(bytes([0x16]))
