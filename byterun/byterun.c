/* Lama SM Bytecode interpreter */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "byterun-lib.h"

void *__start_custom_data;
void *__stop_custom_data;

/* Gets a name for a public symbol */
char *get_public_name(bytefile *f, int i)
{
  return get_string(f, f->public_ptr[i * 2]);
}

/* Disassembles the bytecode pool */
void disassemble(FILE *f, bytefile *bf)
{
  uint32_t ip = 0;
  while (1)
  {
    fprintf(f, "0x%.8x:\t", ip);
    uint32_t len = disassemble_one(bf, ip, f);
    fprintf(f, "\n");
    if (len == 0)
    {
      break;
    }
    ip += len;
  }
  fprintf(f, "<end>\n");
}

/* Dumps the contents of the file */
void dump_file(FILE *f, bytefile *bf)
{
  int i;

  fprintf(f, "String table size       : %d\n", bf->stringtab_size);
  fprintf(f, "Global area size        : %d\n", bf->global_area_size);
  fprintf(f, "Number of public symbols: %d\n", bf->public_symbols_number);
  fprintf(f, "Public symbols          :\n");

  for (i = 0; i < bf->public_symbols_number; i++)
    fprintf(f, "   0x%.8x: %s\n", get_public_offset(bf, i), get_public_name(bf, i));

  fprintf(f, "Code:\n");
  disassemble(f, bf);
}

int main(int argc, char *argv[])
{
  bytefile *f = read_file(argv[1]);
  dump_file(stdout, f);
  return 0;
}
