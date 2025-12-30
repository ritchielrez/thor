#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "defines.h"
#include "libraries/rit_dyn_arr.h"
#include "libraries/rit_str.h"
#include "tokenizer.h"

typedef enum {
  bp_default,
  bp_sub,
  bp_add,
  bp_mul,
  bp_div,
  bp_primary
} binding_power;
typedef enum { stmt_exit, stmt_var_decl } node_stmt_type;
typedef enum { expr_num, expr_var, expr_bin } node_expr_type;

typedef struct node_expr node_expr;

typedef struct {
  int64_t value;
} node_num_expr;

typedef struct {
  node_expr *lhs;
  node_expr *rhs;
  token_type op;
} node_bin_expr;

struct node_expr {
  union {
    node_num_expr num_expr;
    node_bin_expr bin_expr;
    rsv var_expr;
  } value;
  node_expr_type type;
};

typedef struct {
  node_expr *status;
} node_stmt_exit;

typedef struct {
  rsv name;
  node_expr *expr;
} node_stmt_var_decl;

typedef struct {
  union {
    node_stmt_exit exit_stmt;
    node_stmt_var_decl var_decl_stmt;
  } value;
  node_stmt_type type;
} node_stmt;

typedef rda_struct(node_stmt) node_prg;

typedef struct {
  size_t idx;
  node_prg prg;
  rda_allocator *allocator;
  tokenizer_t *tokenizer;
} parser_t;

#define parser_create(t_parser_name, t_file) \
  parser_t t_parser_name = parser_init(t_file)

parser_t parser_init(const char *t_file);
void parse(parser_t *t_parser);
void parser_deinit(parser_t *t_parser);

#endif  // PARSER_H_INCLUDED
