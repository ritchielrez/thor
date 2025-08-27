#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "defines.h"
#include "libraries/rit_dyn_arr.h"
#include "tokenizer.h"

typedef enum {
  bp_default,
  bp_sub,
  bp_add,
  bp_mul,
  bp_div,
  bp_primary
} binding_power;
typedef enum { stmt_exit } node_stmt_type;
typedef enum { expr_num, expr_bin } node_expr_type;

typedef struct node_expr node_expr;

typedef struct {
  int64_t num;
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
  } value;
  node_expr_type type;
};

typedef struct {
  node_expr *status;
} node_stmt_exit;

typedef struct {
  union {
    node_stmt_exit exit_stmt;
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
/// @internal
INTERNAL_DEF node_expr *parse_expr(parser_t *t_parser,
                                   binding_power t_binding_power);
/// @internal
INTERNAL_DEF inline node_expr *parse_num_expr(parser_t *t_parser);
/// @internal
INTERNAL_DEF inline node_expr *parse_bin_expr(parser_t *t_parser,
                                              node_expr *t_lhs);
/// @internal
INTERNAL_DEF bool parse_stmt_exit(parser_t *t_parser);
/// @internal
INTERNAL_DEF bool parse_stmt(parser_t *t_parser);
void parse(parser_t *t_parser);
void parser_deinit(parser_t *t_parser);

/// @internal
INTERNAL_DEF inline token_t parser_peek(parser_t *t_parser, int64_t t_offset) {
  return rda_at(t_parser->tokenizer->tokens, t_parser->idx + t_offset);
}

/// @internal
INTERNAL_DEF inline token_t parser_consume(parser_t *t_parser) {
  token_t tok = rda_at(t_parser->tokenizer->tokens, t_parser->idx);
  t_parser->idx++;
  return tok;
}

#define TOK_LINE parser_peek(t_parser, -1).line
#define TOK_COL \
  parser_peek(t_parser, -1).col + rsv_size(parser_peek(t_parser, -1).value)

/// @internal
/// @brief Look for a statement terminator token.
///
/// Look for it until one is found and then consume that token.
/// Thus the parser will be at a new statment after this function
/// is finished executing.
INTERNAL_DEF inline void parser_skip_statement(parser_t *t_parser) {
  while (parser_peek(t_parser, 0).type != token_semicolon &&
         parser_peek(t_parser, 0).type != token_newline) {
    parser_consume(t_parser);
  }
  parser_consume(t_parser);
}

/// @internal
/// @brief Try consuming a token of specific type.
///
/// If not found, return a `token_invalid`.
INTERNAL_DEF inline token_t parser_try_consume(parser_t *t_parser,
                                               token_type t_token_type) {
  if (parser_peek(t_parser, 0).type != t_token_type) {
    return (token_t){.type = token_invalid,
                     .value = {.m_size = 0, .m_str = ""},
                     .line = parser_peek(t_parser, 0).line,
                     .col = parser_peek(t_parser, 0).col};
  }
  return parser_consume(t_parser);
}

/// @internal
/// @brief Look for a token of specific type.
///
/// If not found, print an error and skip over until a newline, semicolon or
/// EOF is found.
INTERNAL_DEF inline token_t parser_expected_consume(parser_t *t_parser,
                                                    token_type t_token_type) {
  token_t tok = parser_try_consume(t_parser, t_token_type);
  if (tok.type == token_invalid) {
    fprintf(stderr, "Error:%zu:%zu: expected %s\n", TOK_LINE, TOK_COL,
            token_type_to_str(t_token_type));
    parser_skip_statement(t_parser);
    return (token_t){.type = token_error,
                     .value = {.m_size = 0, .m_str = ""},
                     .line = parser_peek(t_parser, 0).line,
                     .col = parser_peek(t_parser, 0).col};
  }
  return tok;
}

/// @internal
INTERNAL_DEF inline binding_power binding_power_lookup(
    token_type t_token_type) {
  switch (t_token_type) {
    case token_num:
      return bp_primary;
    case token_add:
      return bp_add;
    case token_sub:
      return bp_sub;
    case token_mul:
      return bp_mul;
    case token_div:
      return bp_div;
    default: {
      return bp_default;
    }
  }
}

#ifdef DEBUG
/// @internal
INTERNAL_DEF inline void print_expr(const char *t_prefix, node_expr *t_expr) {
  switch (t_expr->type) {
    case expr_num: {
      printf("[DEBUG] %s: %I64d\n", t_prefix, t_expr->value.num_expr.num);
      break;
    }
    case expr_bin: {
#define PREFIX_SZ 64
      char prefix[PREFIX_SZ];
      int written_chars = snprintf(prefix, PREFIX_SZ, "%s.lhs", t_prefix);
      if (written_chars >= PREFIX_SZ) {
        fprintf(stderr,
                "[DEBUG] Error: prefix needs to be a bigger buffer, file: %s, "
                "line: %u",
                __FILE__, __LINE__);
        exit(1);
      }
      print_expr(prefix, t_expr->value.bin_expr.lhs);

      printf("[DEBUG] %s.op: %s\n", t_prefix,
             token_type_to_str(t_expr->value.bin_expr.op));

      written_chars = snprintf(prefix, PREFIX_SZ, "%s.rhs", t_prefix);
      if (written_chars >= PREFIX_SZ) {
        fprintf(stderr,
                "[DEBUG] Error: prefix needs to be a bigger buffer, file: %s, "
                "line: %u",
                __FILE__, __LINE__);
        exit(1);
      }
      print_expr(prefix, t_expr->value.bin_expr.rhs);
      break;
    }
  }
}
#endif  // DEBUG

#endif  // PARSER_H_INCLUDED
