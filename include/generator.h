#ifndef GENRATOR_H_INCLUDED
#define GENERATOR_H_INCLUDED

#include <stdio.h>

#include "defines.h"
#include "libraries/rit_dyn_arr.h"
#include "parser.h"

/// @internal
INTERNAL_DEF inline void generate_expr(FILE *t_file, node_expr *t_expr) {
  switch (t_expr->type) {
    case expr_num: {
      fprintf(t_file, "%I64d", t_expr->value.num_expr.num);
      break;
    }
    case expr_bin: {
      generate_expr(t_file, t_expr->value.bin_expr.lhs);
      fprintf(t_file, "%s", token_type_to_str(t_expr->value.bin_expr.op));
      generate_expr(t_file, t_expr->value.bin_expr.rhs);
      break;
    }
  }
}

/// @internal
INTERNAL_DEF inline void generate_stmt_exit(FILE *t_file,
                                            node_stmt_exit *t_stmt) {
  fprintf(t_file, "\texit(");
  generate_expr(t_file, t_stmt->status);
  fprintf(t_file, ");\n");
}

static inline void generate(const char *t_file_name, node_prg *t_prg) {
  const char *file_name = "out.c";
  if (t_file_name != nullptr) {
    file_name = t_file_name;
  }
  FILE *file = fopen(file_name, "w");
  fprintf(file, "#include <stdlib.h>\n");
  fprintf(file, "int main() {\n");
  rda_for_each(it, (*t_prg)) {
    switch (it->type) {
      case stmt_exit: {
        generate_stmt_exit(file, &(it->value.exit_stmt));
        break;
      }
      case stmt_var_decl: {
        fprintf(file, "\tint %s=", rsv_get(it->value.var_decl_stmt.name));
        generate_expr(file, it->value.var_decl_stmt.expr);
        fprintf(file, ";\n");
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
