#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <stdint.h>

#include "defines.h"
#include "libraries/rit_dyn_arr.h"
#include "tokenizer.h"

typedef enum { stmt_exit } node_stmt_type;

typedef struct {
  int64_t status;
} node_stmt_exit;

typedef struct {
  node_stmt_type type;
  union {
    node_stmt_exit stmt_exit;
  } value;
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
token_t parser_peek(parser_t *t_parser, size_t t_offset);
void parser_consume(parser_t *t_parser, size_t t_offset);
void parser_deinit(parser_t *t_parser);

INTERNAL_DEF inline token_t parser_peek(parser_t *t_parser, size_t t_offset) {
  return rda_at(t_parser->tokenizer->tokens, t_parser->idx + t_offset);
}

INTERNAL_DEF inline token_t parser_consume(parser_t *t_parser) {
  token_t tok = rda_at(t_parser->tokenizer->tokens, t_parser->idx);
  t_parser->idx++;
  return tok;
}

#endif  // PARSER_H_INCLUDED
