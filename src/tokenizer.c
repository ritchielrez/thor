#include "tokenizer.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "allocator.h"
#include "defines.h"
#include "libraries/arena_allocator.h"
#include "utils.h"

tokenizer_t tokenizer_init(const char *t_file, rstr_allocator *t_allocator) {
  tokenizer_t ret = {.tokens = {},
                     .idx = 0,
                     .line = 1,
                     .col = 1,
                     .buffer = {},
                     .allocator = t_allocator};
  rda_init(ret.tokens, 0, sizeof(token_t), t_allocator);
  ret.buffer = utils_read_file(t_file, t_allocator);

  return ret;
}

void tokenize(tokenizer_t *t_tokenizer) {
  while (t_tokenizer->idx < rstr_size(t_tokenizer->buffer)) {
    // Identifiers and keywords
    if (isalpha(tokenizer_peek(t_tokenizer))) {
      token_t tok;
      tok.line = t_tokenizer->line;
      tok.col = t_tokenizer->col;
      struct rstr value = {};
      rstr_init(value, 0, t_tokenizer->allocator);
      rstr_push_back(value, tokenizer_peek(t_tokenizer),
                     t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);

      while (isalnum(tokenizer_peek(t_tokenizer))) {
        rstr_push_back(value, tokenizer_peek(t_tokenizer),
                       t_tokenizer->allocator);
        tokenizer_consume(t_tokenizer);
      }
      char *str = "exit";
      if (!strcmp(rstr_cstr(value), str)) {
        tok.type = token_exit;
        tok.value = RSV_NULL;
      } else {
        tok.type = token_ident;
        tok.value = rsv_rstr(value);
      }
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
    }
    // Numbers
    if (isdigit(tokenizer_peek(t_tokenizer))) {
      token_t tok;
      tok.line = t_tokenizer->line;
      tok.col = t_tokenizer->col;
      struct rstr value = {};
      rstr_init(value, 0, t_tokenizer->allocator);
      rstr_push_back(value, tokenizer_peek(t_tokenizer),
                     t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);
      while (isdigit(tokenizer_peek(t_tokenizer))) {
        rstr_push_back(value, tokenizer_peek(t_tokenizer),
                       t_tokenizer->allocator);
        tokenizer_consume(t_tokenizer);
      }
      tok.type = token_num;
      tok.value = rsv_rstr(value);
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
    }
    // Symbols
    if (tokenizer_peek(t_tokenizer) == '(') {
      token_t tok = {.type = token_open_paren,
                     .value = rsv_lit("("),
                     .line = t_tokenizer->line,
                     .col = t_tokenizer->col};
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);
    }
    if (tokenizer_peek(t_tokenizer) == ')') {
      token_t tok = {.type = token_close_paren,
                     .value = rsv_lit(")"),
                     .line = t_tokenizer->line,
                     .col = t_tokenizer->col};
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);
    }
    if (tokenizer_peek(t_tokenizer) == '{') {
      token_t tok = {.type = token_open_curly,
                     .value = rsv_lit("{"),
                     .line = t_tokenizer->line,
                     .col = t_tokenizer->col};
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);
    }
    if (tokenizer_peek(t_tokenizer) == '}') {
      token_t tok = {.type = token_close_curly,
                     .value = rsv_lit("}"),
                     .line = t_tokenizer->line,
                     .col = t_tokenizer->col};
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);
    }
    if (tokenizer_peek(t_tokenizer) == ':') {
      token_t tok = {.type = token_colon,
                     .value = rsv_lit(":"),
                     .line = t_tokenizer->line,
                     .col = t_tokenizer->col};
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);
    }
    if (tokenizer_peek(t_tokenizer) == ';') {
      token_t tok = {.type = token_semicolon,
                     .value = rsv_lit(";"),
                     .line = t_tokenizer->line,
                     .col = t_tokenizer->col};
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);
    }
    if (tokenizer_peek(t_tokenizer) == '\n') {
      token_t tok = {.type = token_newline,
                     .value = rsv_lit("\n"),
                     .line = t_tokenizer->line,
                     .col = t_tokenizer->col};
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);
      t_tokenizer->line++;
      t_tokenizer->col = 1;
    }
    // Things to ignore
    if (tokenizer_peek(t_tokenizer) == ' ') {
      tokenizer_consume(t_tokenizer);
    }
  }

#ifndef DEBUG
  rda_for_each(it, t_tokenizer->tokens) {
    printf("token_type: %s, token_value: %s, line: %zu, col: %zu\n",
           token_type_name(it->type), rsv_get(it->value), it->line, it->col);
  }
#endif  // DEBUG
}
