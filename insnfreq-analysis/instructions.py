def read_none_continue(code, i, worklist):
    return [], False


def read_none_break(code, i, worklist):
    return [], True


def read_4_continue(code, i, worklist):
    return [code[j] for j in range(i, i + 4)], False


def read_8_continue(code, i, worklist):
    return [code[j] for j in range(i, i + 8)], False


def read_8_break(code, i, worklist):
    return [code[j] for j in range(i, i + 8)], True


def read_jmp(code, i, worklist):
    result = [code[j] for j in range(i, i + 4)]
    target = int.from_bytes(result, byteorder='little')
    worklist.append(target)
    return result, True


def read_cjmp(code, i, worklist):
    result = [code[j] for j in range(i, i + 4)]
    target = int.from_bytes(result, byteorder='little')
    worklist.append(target)
    worklist.append(i + 4)
    return result, True


def read_callc(code, i, worklist):
    result = [code[j] for j in range(i, i + 4)]
    worklist.append(i + 4)
    return result, True


def read_call(code, i, worklist):
    result = [code[j] for j in range(i, i + 8)]
    target = int.from_bytes(result[0:4], byteorder='little')
    worklist.append(target)
    worklist.append(i + 8)
    return result, True


def read_closure(code, i, worklist):
    result = [code[j] for j in range(i, i + 8)]
    worklist.append(int.from_bytes(result[0:4], byteorder='little'))
    num_captures = int.from_bytes(result[4:8], byteorder='little')
    i += 8
    for _ in range(num_captures):
        result.extend([code[j] for j in range(i, i + 5)])
        i += 5
    return result, False


by_mnemonic = {
    "ADD": (0x01, read_none_continue),
    "SUB": (0x02, read_none_continue),
    "MUL": (0x03, read_none_continue),
    "DIV": (0x04, read_none_continue),
    "REM": (0x05, read_none_continue),
    "LT": (0x06, read_none_continue),
    "LEQ": (0x07, read_none_continue),
    "GT": (0x08, read_none_continue),
    "GEQ": (0x09, read_none_continue),
    "EQ": (0x0A, read_none_continue),
    "NEQ": (0x0B, read_none_continue),
    "AND": (0x0C, read_none_continue),
    "OR": (0x0D, read_none_continue),
    "CONST": (0x10, read_4_continue),
    "STRING": (0x11, read_4_continue),
    "SEXP": (0x12, read_8_continue),
    "STA": (0x14, read_none_continue),
    "JMP": (0x15, read_jmp),
    "END": (0x16, read_none_break),
    "RET": (0x17, read_none_break),
    "DROP": (0x18, read_none_continue),
    "DUP": (0x19, read_none_continue),
    "SWAP": (0x1A, read_none_continue),
    "ELEM": (0x1B, read_none_continue),
    "LD_GLOBAL": (0x20, read_4_continue),
    "LD_LOCAL": (0x21, read_4_continue),
    "LD_ARG": (0x22, read_4_continue),
    "LD_CAPTURE": (0x23, read_4_continue),
    "ST_GLOBAL": (0x40, read_4_continue),
    "ST_LOCAL": (0x41, read_4_continue),
    "ST_ARG": (0x42, read_4_continue),
    "ST_CAPTURE": (0x43, read_4_continue),
    "CJMP_Z": (0x50, read_cjmp),
    "CJMP_NZ": (0x51, read_cjmp),
    "BEGIN": (0x52, read_8_continue),
    "BEGINC": (0x53, read_8_continue),
    "CLOSURE": (0x54, read_closure),
    "CALLC": (0x55, read_callc),
    "CALL": (0x56, read_call),
    "TAG": (0x57, read_8_continue),
    "ARRAY": (0x58, read_4_continue),
    "FAIL": (0x59, read_8_break),
    "LINE": (0x5A, read_4_continue),
    "PATTERN_STRCMP": (0x60, read_none_continue),
    "PATTERN_STRING": (0x61, read_none_continue),
    "PATTERN_ARRAY": (0x62, read_none_continue),
    "PATTERN_SEXP": (0x63, read_none_continue),
    "PATTERN_BOXED": (0x64, read_none_continue),
    "PATTERN_UNBOXED": (0x65, read_none_continue),
    "PATTERN_CLOSURE": (0x66, read_none_continue),
    "BUILTIN_READ": (0x70, read_none_continue),
    "BUILTIN_WRITE": (0x71, read_none_continue),
    "BUILTIN_LENGTH": (0x72, read_none_continue),
    "BUILTIN_STRING": (0x73, read_none_continue),
    "BUILTIN_ARRAY": (0x74, read_4_continue),
    "STOP": (0xFF, read_none_break),
}

by_opcode = {v[0]: (k, v[1]) for k, v in by_mnemonic.items()}
