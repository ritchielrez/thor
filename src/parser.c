#include "parser.h"

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

INTERNAL_DEF node_expr *parse_expr(parser_t *t_parser,
                                   binding_power t_binding_power) {
  node_expr *expr = parse_num_expr(t_parser);
  while (binding_power_lookup(parser_peek(t_parser, 0).type) >
         t_binding_power) {
    expr = parse_bin_expr(t_parser, expr);
  }
  return expr;
}

INTERNAL_DEF node_expr *parse_num_expr(parser_t *t_parser) {
  token_t tok = parser_expected_consume(t_parser, token_num);
  if (tok.type == token_error) exit(1);
  node_num_expr num_expr = {.num = atoi(rsv_get(tok.value))};
  node_expr *expr = arena_alloc_struct(t_parser->allocator->m_ctx, node_expr);
  expr->type = expr_num;
  expr->value.num_expr = num_expr;
  return expr;
}

INTERNAL_DEF node_expr *parse_bin_expr(parser_t *t_parser, node_expr *t_lhs) {
  node_expr *expr = arena_alloc_struct(t_parser->allocator->m_ctx, node_expr);
  expr->type = expr_bin;
  expr->value.bin_expr.lhs = t_lhs;
  expr->value.bin_expr.op = parser_consume(t_parser).type;
  expr->value.bin_expr.rhs =
      parse_expr(t_parser, binding_power_lookup(expr->value.bin_expr.op));
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

INTERNAL_DEF bool parse_stmt(parser_t *t_parser) {
  if (parser_peek(t_parser, 0).type == token_newline ||
      parser_peek(t_parser, 0).type == token_semicolon) {
    parser_consume(t_parser);
    return true;
  } else if (parser_try_consume(t_parser, token_exit).type != token_invalid) {
    return parse_stmt_exit(t_parser);
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
      case stmt_exit:
        print_expr("stmt_exit.status", it->value.exit_stmt.status);
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
