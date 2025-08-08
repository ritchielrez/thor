#ifndef TOKENIZER_H_INCLUDED
#define TOKENIZER_H_INCLUDED

#include "libraries/rit_dyn_arr.h"
#include "libraries/rit_str.h"

typedef enum {
  token_identifier,
  token_exit,
  token_num,
  token_open_paren,
  token_close_paren,
  token_open_curly,
  token_close_curly,
  token_colon,
  token_semicolon,
  token_newline,
} token_type;

typedef struct {
  size_t line;
  size_t col;
} loc_t;

typedef struct {
  token_type type;
  rsv value;
  // loc_t *loc;
} token_t;

typedef rda_struct(token_t) tokens_t;

typedef struct {
  tokens_t tokens;
  size_t tok_idx;
  struct rstr buffer;
  rstr_allocator *allocator;
} tokenizer_t;

#define tokenizer_create(t_tokenizer_name, t_file, t_allocator) \
  tokenizer_t t_tokenizer_name = tokenizer_init(t_file, t_allocator)

tokenizer_t tokenizer_init(const char *t_file, rstr_allocator *t_allocator);
const char *token_type_name(token_type t_token_type);
char tokenizer_peek(tokenizer_t *t_tokenizer);
void tokenizer_consume(tokenizer_t *t_tokenizer);
void tokenize(tokenizer_t *t_tokenizer);

#endif  // TOKENIZER_H_INCLUDED
