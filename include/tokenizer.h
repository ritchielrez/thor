#ifndef TOKENIZER_H_INCLUDED
#define TOKENIZER_H_INCLUDED

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "external/arena_allocator.h"
#include "external/rit_dyn_arr.h"
#include "external/rit_str.h"
#include "utils.h"

// Disable MSVC warning 4702: unreachable code
#pragma warning(push)
#pragma warning(disable : 4702)

static inline void *tokenizer_allocator_alloc(void *t_arena,
                                              size_t t_size_in_bytes) {
  return arena_alloc((Arena *)t_arena, t_size_in_bytes);
}
static inline void tokenizer_allocator_free(void *t_arena, void *t_ptr) {
  (void)t_ptr;
  (void)t_arena;
}
static inline void *tokenizer_allocator_realloc(void *t_arena, void *t_old_ptr,
                                                size_t t_old_size_in_bytes,
                                                size_t t_new_size_in_bytes) {
  return arena_realloc((Arena *)t_arena, t_old_ptr, t_old_size_in_bytes,
                       t_new_size_in_bytes);
}

typedef enum {
  token_id,  // Identifier: names assigned by the programmer

  token_lit,  // For now only an int

  // Keywords
  token_kw_exit,
  token_kw_fn,

  // Seperators
  token_sep_open_paren,
  token_sep_close_paren,
  token_sep_open_brace,
  token_sep_close_brace,
  token_sep_semi,
  token_sep_colon,
  token_sep_dcolon,  // Double colon
} token_type_e;

char *token_type_to_str(token_type_e type);

typedef struct {
  token_type_e type;
  char *value;
} token_t;

typedef rda_struct(token_t) tokens_t;

typedef struct {
  struct rstr src;
  size_t i;
  rda_allocator tokenizer_allocator;
  rda_allocator *parser_allocator;
} tokenizer_t;

#define tokenizer_create(t_tokenizer, t_src, t_allocator) \
  tokenizer_t t_tokenizer;                                \
  tokenizer_init(&t_tokenizer, t_src, t_allocator)
void tokenizer_init(tokenizer_t *t_tokenizer, const char *t_src,
                    rda_allocator *t_allocator);

#define tokenizer_peek(t_tokenizer, t_offset) \
  rstr_at(((t_tokenizer)->src), ((t_tokenizer)->i + t_offset))

char tokenizer_consume(tokenizer_t *t_tokenizer);

void tokenizer_skip_whitespace(tokenizer_t *t_tokenizer);

tokens_t tokenize(tokenizer_t *t_tokenizer);

#endif  // TOKENIZER_H_INCLUDED
#ifdef TOKENIZER_IMPLEMENTATION

#define ARENA_ALLOCATOR_IMPLEMENTATION
#define RIT_STR_IMPLEMENTATION
#include "external/arena_allocator.h"
#include "external/rit_str.h"

char *token_type_to_str(token_type_e type) {
  switch (type) {
    case token_id:
      return "token_id";
    case token_lit:
      return "token_lit";
    case token_kw_exit:
      return "token_kw_exit";
    case token_kw_fn:
      return "token_kw_fn";
    case token_sep_open_paren:
      return "token_sep_open_paren";
    case token_sep_close_paren:
      return "token_sep_close_paren";
    case token_sep_open_brace:
      return "token_sep_open_brace";
    case token_sep_close_brace:
      return "token_sep_close_brace";
    case token_sep_semi:
      return "token_sep_semi";
    case token_sep_colon:
      return "token_sep_colon";
    case token_sep_dcolon:
      return "token_sep_dcolon";
    default:
      fprintf(stderr, "Invalid type\n");
      return NULL;
  }
}

#define tokenizer_create(t_tokenizer, t_src, t_allocator) \
  tokenizer_t t_tokenizer;                                \
  tokenizer_init(&t_tokenizer, t_src, t_allocator)

void tokenizer_init(tokenizer_t *t_tokenizer, const char *t_src,
                    rda_allocator *t_allocator) {
  // Allocate a buffer of 1000000 8 byte chunks(8 MB)
  Arena *arena = malloc(sizeof(Arena));
  *arena = arena_init(1000000);
  t_tokenizer->tokenizer_allocator.alloc = tokenizer_allocator_alloc;
  t_tokenizer->tokenizer_allocator.free = tokenizer_allocator_free;
  t_tokenizer->tokenizer_allocator.realloc = tokenizer_allocator_realloc;
  t_tokenizer->tokenizer_allocator.m_ctx = arena;
  t_tokenizer->parser_allocator = t_allocator;
  t_tokenizer->src =
      utils_read_file(t_src, &(t_tokenizer->tokenizer_allocator));
  t_tokenizer->i = 0;
}

char tokenizer_consume(tokenizer_t *t_tokenizer) {
  if (t_tokenizer->i + 1 >= rstr_size(t_tokenizer->src)) {
    t_tokenizer->i++;
    return rstr_back(t_tokenizer->src);
  }
  size_t i = t_tokenizer->i++;
  return rstr_at(t_tokenizer->src, i);
}

void tokenizer_skip_whitespace(tokenizer_t *t_tokenizer) {
  for (; t_tokenizer->i < rstr_size(t_tokenizer->src) &&
         (tokenizer_peek(t_tokenizer, 0) == '\n' ||
          tokenizer_peek(t_tokenizer, 0) == '\r' ||
          tokenizer_peek(t_tokenizer, 0) == ' ' ||
          tokenizer_peek(t_tokenizer, 0) == '\t');
       tokenizer_consume(t_tokenizer)) {
  }
}

tokens_t tokenize(tokenizer_t *t_tokenizer) {
  token_t token;
  tokens_t tokens;
  rda_init(tokens, 400, sizeof(token_t), t_tokenizer->parser_allocator);
  rda_clear(tokens);
  while (t_tokenizer->i < rstr_size(t_tokenizer->src)) {
    // TODO: implement `value` as a rstr.
    if (isalpha(tokenizer_peek(t_tokenizer, 0))) {
      rstr(buf, rsv_lit(""), t_tokenizer->parser_allocator);
      rstr_push_back(buf, tokenizer_consume(t_tokenizer),
                     t_tokenizer->parser_allocator);
      while (isalnum(tokenizer_peek(t_tokenizer, 0))) {
        rstr_push_back(buf, tokenizer_consume(t_tokenizer),
                       t_tokenizer->parser_allocator);
      }
      if (!strcmp(rstr_cstr(buf), "exit")) {
        token.type = token_kw_exit;
        token.value = "";
        rda_push_back(tokens, token, t_tokenizer->parser_allocator);
      } else if (!strcmp(rstr_cstr(buf), "fn")) {
        token.type = token_kw_fn;
        token.value = "";
        rda_push_back(tokens, token, t_tokenizer->parser_allocator);
      } else {
        token.type = token_id;
        token.value = rstr_cstr(buf);
        rda_push_back(tokens, token, t_tokenizer->parser_allocator);
      }
      rstr_clear(buf);
    } else if (isdigit(tokenizer_peek(t_tokenizer, 0))) {
      rstr(buf, rsv_lit(""), t_tokenizer->parser_allocator);
      rstr_push_back(buf, tokenizer_consume(t_tokenizer),
                     t_tokenizer->parser_allocator);
      while (isdigit(tokenizer_peek(t_tokenizer, 0))) {
        rstr_push_back(buf, tokenizer_consume(t_tokenizer),
                       t_tokenizer->parser_allocator);
      }
      token.type = token_lit;
      token.value = rstr_cstr(buf);
      rda_push_back(tokens, token, t_tokenizer->parser_allocator);
      rstr_clear(buf);
    } else if (tokenizer_peek(t_tokenizer, 0) == ':') {
      tokenizer_consume(t_tokenizer);
      if (tokenizer_peek(t_tokenizer, 0) == ':') {
        tokenizer_consume(t_tokenizer);
        token.type = token_sep_dcolon;
        token.value = "";
        rda_push_back(tokens, token, t_tokenizer->parser_allocator);
      } else {
        token.type = token_sep_colon;
        token.value = "";
        rda_push_back(tokens, token, t_tokenizer->parser_allocator);
      }
    } else if (tokenizer_peek(t_tokenizer, 0) == ';') {
      tokenizer_consume(t_tokenizer);
      token.type = token_sep_semi;
      token.value = "";
      rda_push_back(tokens, token, t_tokenizer->parser_allocator);
    } else if (tokenizer_peek(t_tokenizer, 0) == '(') {
      tokenizer_consume(t_tokenizer);
      token.type = token_sep_open_paren;
      token.value = "";
      rda_push_back(tokens, token, t_tokenizer->parser_allocator);
    } else if (tokenizer_peek(t_tokenizer, 0) == ')') {
      tokenizer_consume(t_tokenizer);
      token.type = token_sep_close_paren;
      token.value = "";
      rda_push_back(tokens, token, t_tokenizer->parser_allocator);
    } else if (tokenizer_peek(t_tokenizer, 0) == '{') {
      tokenizer_consume(t_tokenizer);
      token.type = token_sep_open_brace;
      token.value = "";
      rda_push_back(tokens, token, t_tokenizer->parser_allocator);
    } else if (tokenizer_peek(t_tokenizer, 0) == '}') {
      tokenizer_consume(t_tokenizer);
      token.type = token_sep_close_brace;
      token.value = "";
      rda_push_back(tokens, token, t_tokenizer->parser_allocator);
    }
    tokenizer_skip_whitespace(t_tokenizer);
  }
  arena_free(t_tokenizer->tokenizer_allocator.m_ctx);
  free(t_tokenizer->tokenizer_allocator.m_ctx);
  return tokens;
}

#endif  // TOKENIZER_IMPLEMENTATION
