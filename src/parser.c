#include "parser.h"

#include <stdlib.h>

#include "allocator.h"
#include "defines.h"
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

token_t parser_peek(parser_t *t_parser, size_t t_offset) {
  return rda_at(t_parser->tokenizer->tokens, t_parser->idx + t_offset);
}

void parser_consume(parser_t *t_parser, size_t t_offset) {
  t_parser->idx += t_offset + 1;
}

void parse(parser_t *t_parser) {
  while (t_parser->idx < rda_size(t_parser->tokenizer->tokens)) {
    if (parser_peek(t_parser, 0).type == token_exit &&
        parser_peek(t_parser, 1).type == token_open_paren &&
        parser_peek(t_parser, 2).type == token_num &&
        parser_peek(t_parser, 3).type == token_close_paren &&
        (parser_peek(t_parser, 4).type == token_semicolon ||
         parser_peek(t_parser, 4).type == token_newline)) {
      node_stmt stmt;
      stmt.type = stmt_exit;
      stmt.value.stmt_exit.status =
          atoi(rsv_get(parser_peek(t_parser, 2).value));
      parser_consume(t_parser, 4);
      rda_push_back(t_parser->prg, stmt, t_parser->allocator);
    }
    // Things to ignore
    // Useless newlines
    if (parser_peek(t_parser, 0).type == token_newline) {
      parser_consume(t_parser, 0);
    }
  }
}

void parser_deinit(parser_t *t_parser) {
  arena_allocator_free(t_parser->allocator->m_ctx, nullptr);
  free(t_parser->allocator->m_ctx);
  t_parser->allocator->m_ctx = nullptr;
  free(t_parser->allocator);
  t_parser->allocator = nullptr;
}
