#include "parser.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "allocator.h"
#include "defines.h"
#include "generator.h"
#include "libraries/arena_allocator.h"
#include "libraries/rit_dyn_arr.h"
#include "libraries/rit_str.h"
#include "tokenizer.h"

parser_t parser_init(const char *t_file) {
  Arena *arena = malloc(sizeof(Arena));
  arena->m_begin = nullptr;
  arena->m_active = nullptr;
  rda_allocator *allocator = malloc(sizeof(rstr_allocator));
  allocator->alloc = arena_allocator_alloc;
  allocator->free = arena_allocator_free;
  allocator->realloc = arena_allocator_realloc;
  allocator->m_ctx = arena;

  tokenizer_t *tokenizer =
      (tokenizer_t *)arena_alloc(allocator->m_ctx, sizeof(tokenizer_t));
  *tokenizer = tokenizer_init(t_file, allocator);
  tokenize(tokenizer);

  parser_t ret = {.prg = {}, .tokenizer = tokenizer, .allocator = allocator};
  rda_init(ret.prg, 0, sizeof(node_stmt), allocator);

  return ret;
}

INTERNAL_DEF token_t parser_peek(parser_t *t_parser, int64_t t_offset) {
  return rda_at(t_parser->tokenizer->tokens, t_parser->idx + t_offset);
}

INTERNAL_DEF token_t parser_consume(parser_t *t_parser) {
  token_t tok = rda_at(t_parser->tokenizer->tokens, t_parser->idx);
  t_parser->idx++;
  return tok;
}

#define TOK_LINE parser_peek(t_parser, -1).line
#define TOK_COL \
  parser_peek(t_parser, -1).col + rsv_size(parser_peek(t_parser, -1).value)

INTERNAL_DEF void parser_skip_statement(parser_t *t_parser) {
  while (parser_peek(t_parser, 0).type != token_semicolon &&
         parser_peek(t_parser, 0).type != token_newline) {
    parser_consume(t_parser);
  }
  parser_consume(t_parser);
}

INTERNAL_DEF token_t parser_try_consume(parser_t *t_parser,
                                        token_type t_token_type) {
  if (parser_peek(t_parser, 0).type != t_token_type) {
    return (token_t){.type = token_invalid,
                     .value = {.m_size = 0, .m_str = ""},
                     .line = parser_peek(t_parser, 0).line,
                     .col = parser_peek(t_parser, 0).col};
  }
  return parser_consume(t_parser);
}

INTERNAL_DEF token_t parser_expected_consume(parser_t *t_parser,
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

INTERNAL_DEF binding_power binding_power_lookup(token_type t_token_type) {
  switch (t_token_type) {
    case token_num:
      return bp_primary;
    case token_plus:
      return bp_add;
    case token_minus:
      return bp_sub;
    case token_star:
      return bp_mul;
    case token_fslash:
      return bp_div;
    default: {
      return bp_default;
    }
  }
}

#ifdef DEBUG
INTERNAL_DEF void print_expr(const char *t_prefix, node_expr *t_expr) {
  switch (t_expr->type) {
    case expr_num: {
      printf("[DEBUG] %s: %" PRId64 "\n", t_prefix,
             t_expr->value.num_expr.value);
      break;
    }
    case expr_var: {
      printf("[DEBUG] %s: %s\n", t_prefix, rsv_get(t_expr->value.var_expr));
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

INTERNAL_DEF node_expr *parse_primary_expr(parser_t *t_parser) {
  // token_t tok = parser_expected_consume(t_parser, token_num);
  node_expr *expr = arena_alloc_struct(t_parser->allocator->m_ctx, node_expr);
  switch (parser_peek(t_parser, 0).type) {
    case token_num: {
      token_t tok = parser_consume(t_parser);
      node_num_expr num_expr = {.value = atoi(rsv_get(tok.value))};
      expr->type = expr_num;
      expr->value.num_expr = num_expr;
      break;
    }
    case token_ident: {
      token_t tok = parser_consume(t_parser);
      rsv var_expr = tok.value;
      expr->type = expr_var;
      expr->value.var_expr = var_expr;
      break;
    }
    default: {
      fprintf(stderr, "invalid token type\n");
      break;
    }
  }
  // if (tok.type == token_error) exit(1);
  return expr;
}

// Forward declare because `parse_bin_expr()` and `parse_expr()` rely on each
// other.
INTERNAL_DEF node_expr *parse_expr(parser_t *t_parser,
                                   binding_power t_binding_power);

INTERNAL_DEF node_expr *parse_bin_expr(parser_t *t_parser, node_expr *t_lhs) {
  node_expr *expr = arena_alloc_struct(t_parser->allocator->m_ctx, node_expr);
  expr->type = expr_bin;
  expr->value.bin_expr.lhs = t_lhs;
  expr->value.bin_expr.op = parser_consume(t_parser).type;
  expr->value.bin_expr.rhs =
      parse_expr(t_parser, binding_power_lookup(expr->value.bin_expr.op));
  return expr;
}

INTERNAL_DEF node_expr *parse_expr(parser_t *t_parser,
                                   binding_power t_binding_power) {
  node_expr *expr = parse_primary_expr(t_parser);
  while (binding_power_lookup(parser_peek(t_parser, 0).type) >
         t_binding_power) {
    expr = parse_bin_expr(t_parser, expr);
  }
  return expr;
}

INTERNAL_DEF bool parse_stmt_exit(parser_t *t_parser) {
  if (parser_expected_consume(t_parser, token_open_paren).type == token_error) {
    return false;
  }
  node_expr *expr = parse_expr(t_parser, bp_default);
  if (parser_expected_consume(t_parser, token_close_paren).type ==
      token_error) {
    return false;
  }
  if (parser_peek(t_parser, 0).type != token_semicolon &&
      parser_peek(t_parser, 0).type != token_newline) {
    fprintf(stderr, "Error:%zu:%zu: expected a newline or ;\n", TOK_LINE,
            TOK_COL);
    return false;
  }
  parser_consume(t_parser);  // Consume the semicolon or newline
  node_stmt stmt;
  stmt.type = stmt_exit;
  stmt.value.exit_stmt.status = expr;
  rda_push_back(t_parser->prg, stmt, t_parser->allocator);
  return true;
}

INTERNAL_DEF bool parse_stmt_var_decl(parser_t *t_parser,
                                      token_t t_token_ident) {
  if (parser_expected_consume(t_parser, token_colon).type == token_error) {
    return false;
  }
  if (parser_expected_consume(t_parser, token_assignment).type == token_error) {
    return false;
  }
  node_expr *expr = parse_expr(t_parser, bp_default);
  if (parser_peek(t_parser, 0).type != token_semicolon &&
      parser_peek(t_parser, 0).type != token_newline) {
    fprintf(stderr, "Error:%zu:%zu: expected a newline or ;\n", TOK_LINE,
            TOK_COL);
    return false;
  }
  parser_consume(t_parser);  // Consume the semicolon or newline
  node_stmt stmt;
  stmt.type = stmt_var_decl;
  stmt.value.var_decl_stmt.name = t_token_ident.value;
  stmt.value.var_decl_stmt.expr = expr;
  rda_push_back(t_parser->prg, stmt, t_parser->allocator);
  return true;
}

INTERNAL_DEF bool parse_stmt(parser_t *t_parser) {
  if (parser_peek(t_parser, 0).type == token_newline ||
      parser_peek(t_parser, 0).type == token_semicolon) {
    parser_consume(t_parser);
    return true;
  } else if (parser_try_consume(t_parser, token_exit).type != token_invalid) {
    return parse_stmt_exit(t_parser);
  } else if (parser_peek(t_parser, 0).type == token_ident) {
    return parse_stmt_var_decl(t_parser, parser_consume(t_parser));
  } else if (parser_try_consume(t_parser, token_fslash).type != token_invalid &&
             parser_try_consume(t_parser, token_fslash).type != token_invalid) {
    while (parser_peek(t_parser, 0).type != token_newline) {
      parser_consume(t_parser);
    }
    parser_consume(t_parser);
    return true;
  } else {
    fprintf(stderr, "Error:%zu:%zu: invalid identifier %s\n",
            parser_peek(t_parser, 0).line, parser_peek(t_parser, 0).col,
            rsv_get(parser_peek(t_parser, 0).value));
    parser_skip_statement(t_parser);
    return false;
  }
}

void parse(parser_t *t_parser) {
  bool success = true;
  while (t_parser->idx < rda_size(t_parser->tokenizer->tokens)) {
    success = parse_stmt(t_parser) ? success : false;
  }

#ifdef DEBUG
  rda_for_each(it, t_parser->prg) {
    switch (it->type) {
      case stmt_exit: {
        print_expr("stmt_exit.status", it->value.exit_stmt.status);
        break;
      }
      case stmt_var_decl: {
        printf("[DEBUG] stmt_var_decl.name: %s\n",
               rsv_get(it->value.var_decl_stmt.name));
        print_expr("stmt_var_decl.expr", it->value.var_decl_stmt.expr);
        break;
      }
    }
  }
#endif  // DEBUG

  if (success) generate(nullptr, &(t_parser->prg));
}

void parser_deinit(parser_t *t_parser) {
  arena_allocator_free(t_parser->allocator->m_ctx, nullptr);
  free(t_parser->allocator->m_ctx);
  t_parser->allocator->m_ctx = nullptr;
  free(t_parser->allocator);
  t_parser->allocator = nullptr;
}
