#include "tokenizer.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "allocator.h"
#include "defines.h"
#include "libraries/arena_allocator.h"
#include "utils.h"

tokenizer_t tokenizer_init(const char *t_file, rstr_allocator *t_allocator) {
  tokenizer_t ret = {
      .tokens = {}, .tok_idx = 0, .buffer = {}, .allocator = t_allocator};
  rda_init(ret.tokens, 0, sizeof(token_t), t_allocator);
  ret.buffer = utils_read_file(t_file, t_allocator);

  return ret;
}

const char *token_type_name(token_type t_token_type) {
  switch (t_token_type) {
    case token_identifier:
      return "token_identifier";
    case token_num:
      return "token_num";
    case token_exit:
      return "token_exit";
    case token_open_paren:
      return "token_open_paren";
    case token_close_paren:
      return "token_close_paren";
    case token_open_curly:
      return "token_open_curly";
    case token_close_curly:
      return "token_close_curly";
    case token_colon:
      return "token_colon";
    case token_semicolon:
      return "token_semicolon";
    case token_newline:
      return "token_newline";
  }
  return "invalid token type";
}

char tokenizer_peek(tokenizer_t *t_tokenizer) {
  return rstr_at(t_tokenizer->buffer, t_tokenizer->tok_idx);
}

void tokenizer_consume(tokenizer_t *t_tokenizer) { t_tokenizer->tok_idx++; }

void tokenize(tokenizer_t *t_tokenizer) {
  while (t_tokenizer->tok_idx < rstr_size(t_tokenizer->buffer)) {
    // Identifiers and keywords
    if (isalpha(tokenizer_peek(t_tokenizer))) {
      token_t tok;
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
        tok.value = rsv_rstr(value);
      } else {
        tok.type = token_identifier;
        tok.value = rsv_rstr(value);
      }
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
    }
    // Numbers
    if (isdigit(tokenizer_peek(t_tokenizer))) {
      token_t tok;
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
      token_t tok = {.type = token_open_paren, .value = rsv_lit("(")};
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);
    }
    if (tokenizer_peek(t_tokenizer) == ')') {
      token_t tok = {.type = token_close_paren, .value = rsv_lit(")")};
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);
    }
    if (tokenizer_peek(t_tokenizer) == '{') {
      token_t tok = {.type = token_open_curly, .value = rsv_lit("{")};
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);
    }
    if (tokenizer_peek(t_tokenizer) == '}') {
      token_t tok = {.type = token_close_curly, .value = rsv_lit("}")};
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);
    }
    if (tokenizer_peek(t_tokenizer) == ':') {
      token_t tok = {.type = token_colon, .value = rsv_lit(":")};
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);
    }
    if (tokenizer_peek(t_tokenizer) == ';') {
      token_t tok = {.type = token_semicolon, .value = rsv_lit(";")};
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);
    }
    if (tokenizer_peek(t_tokenizer) == '\n') {
      token_t tok = {.type = token_newline, .value = rsv_lit("\n")};
      rda_push_back(t_tokenizer->tokens, tok, t_tokenizer->allocator);
      tokenizer_consume(t_tokenizer);
    }
    // Things to ignore
    if (tokenizer_peek(t_tokenizer) == ' ') {
      tokenizer_consume(t_tokenizer);
    }
  }
}
