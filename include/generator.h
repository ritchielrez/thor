#ifndef GENRATOR_H_INCLUDED
#define GENERATOR_H_INCLUDED

#include <stdio.h>

#include "defines.h"
#include "libraries/rit_dyn_arr.h"
#include "parser.h"

/// @internal
INTERNAL_DEF inline void generate_stmt_exit(FILE *t_file,
                                            node_stmt_exit *t_stmt) {
  fprintf(t_file, "\texit(%I64d);\n", t_stmt->status);
}

static inline void generator(const char *t_file_name, node_prg *prg) {
  const char *file_name = "out.c";
  if (t_file_name != nullptr) {
    file_name = t_file_name;
  }
  FILE *file = fopen(file_name, "w");
  fprintf(file, "#include <stdio.h>\n");
  fprintf(file, "int main() {\n");
  rda_for_each(it, (*prg)) {
    switch (it->type) {
      case stmt_exit: {
        generate_stmt_exit(file, &(it->value.stmt_exit));
        break;
      }
      default: {
        fprintf(stderr, "Error: unknown statment type\n");
        exit(1);
      }
    }
  }
  fprintf(file, "}\n");
  fclose(file);
}

#endif  // GENERATOR_H_INCLUDED
