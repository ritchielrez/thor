#ifndef TOKENIZER_H_INCLUDED
#define TOKENIZER_H_INCLUDED

#include "defines.h"
#include "libraries/rit_dyn_arr.h"
#include "libraries/rit_str.h"

typedef enum {
  token_ident,
  token_exit,
  token_num,
  token_open_paren,
  token_close_paren,
  token_open_curly,
  token_close_curly,
  token_colon,
  token_semicolon,
  token_newline,
  token_invalid,  // Used when parser tries to find a token of specific type but
                  // did not find it

  token_error,  // Used when parser is expecting one type of token but cannot
                // find it
} token_type;

static const char *token_types_names[] = {
    "token_identifier",  "token_exit",        "token_num",
    "token_open_paren",  "token_close_paren", "token_open_curly",
    "token_close_curly", "token_colon",       "token_semicolon",
    "token_newline",     "token_eof",
};

typedef struct {
  size_t line;
  size_t col;
  rsv value;
  token_type type;
} token_t;

typedef rda_struct(token_t) tokens_t;

typedef struct {
  tokens_t tokens;
  size_t idx;
  size_t line;
  size_t col;
  struct rstr buffer;
  rstr_allocator *allocator;
} tokenizer_t;

#define tokenizer_create(t_tokenizer_name, t_file, t_allocator) \
  tokenizer_t t_tokenizer_name = tokenizer_init(t_file, t_allocator)

tokenizer_t tokenizer_init(const char *t_file, rstr_allocator *t_allocator);
void tokenize(tokenizer_t *t_tokenizer);

INTERNAL_DEF inline const char *token_type_name(token_type t_token_type) {
  return token_types_names[t_token_type];
/// @internal
INTERNAL_DEF inline const char *token_type_to_str(token_type t_token_type) {
  return token_type_strs[t_token_type];
}

/// @internal
INTERNAL_DEF inline char tokenizer_peek(tokenizer_t *t_tokenizer) {
  return rstr_at(t_tokenizer->buffer, t_tokenizer->idx);
}

/// @internal
INTERNAL_DEF void tokenizer_consume(tokenizer_t *t_tokenizer) {
  t_tokenizer->idx++;
  t_tokenizer->col++;
}

#endif  // TOKENIZER_H_INCLUDED
