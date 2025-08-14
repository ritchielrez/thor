#ifndef GENRATOR_H_INCLUDED
#define GENERATOR_H_INCLUDED

#include <stdio.h>

#include "defines.h"
#include "libraries/rit_dyn_arr.h"
#include "parser.h"

static inline void generator(const char *t_file, node_prg *prg) {
  const char *file_name = "out.c";
  if (t_file != nullptr) {
    file_name = t_file;
  }
  FILE *file = fopen(file_name, "w");
  fprintf(file, "#include <stdio.h>\n");
  fprintf(file, "int main() {\n");
  fprintf(file, "\texit(%I64d);\n", rda_at((*prg), 0).value.stmt_exit.status);
  fprintf(file, "}\n");
  fclose(file);
}

#endif  // GENERATOR_H_INCLUDED
