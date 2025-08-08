#include "parser.h"

#include "allocator.h"
#include "defines.h"
#include "libraries/arena_allocator.h"
#include "libraries/rit_dyn_arr.h"
#include "tokenizer.h"

parser_t parser_init() {
  Arena *arena = malloc(sizeof(Arena));
  arena->m_begin = nullptr;
  arena->m_active = nullptr;
  rda_allocator *allocator = malloc(sizeof(rstr_allocator));
  allocator->alloc = arena_allocator_alloc;
  allocator->free = arena_allocator_free;
  allocator->realloc = arena_allocator_realloc;
  allocator->m_ctx = arena;

  parser_t ret = {.prg = {}, .allocator = allocator};
  rda_init(ret.prg, 0, sizeof(node_stmt), allocator);

  return ret;
}

void parse(parser_t *t_parser, const char *t_file) {
  (void)t_parser;
  tokenizer_t *tokenizer = (tokenizer_t *)arena_alloc(
      t_parser->allocator->m_ctx, sizeof(tokenizer_t));
  *tokenizer = tokenizer_init(t_file, t_parser->allocator);
  tokenize(tokenizer);
}

void parser_deinit(parser_t *t_parser) {
  arena_allocator_free(t_parser->allocator->m_ctx, nullptr);
  free(t_parser->allocator->m_ctx);
  t_parser->allocator->m_ctx = nullptr;
  free(t_parser->allocator);
  t_parser->allocator = nullptr;
}
