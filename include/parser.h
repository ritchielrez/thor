#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include "external/arena_allocator.h"
#include "external/rit_dyn_arr.h"
#include "external/rit_str.h"
#include "tokenizer.h"

void *parser_allocator_alloc(void *t_arena, size_t t_size_in_bytes);
void parser_allocator_free(void *t_arena, void *t_ptr);
void *parser_allocator_realloc(void *t_arena, void *t_old_ptr,
                               size_t t_old_size_in_bytes,
                               size_t t_new_size_in_bytes);

typedef struct stmt_t stmt_t;
typedef enum stmt_type_e stmt_type_e;
typedef struct stmt_fn_define_t stmt_fn_define_t;
typedef struct stmt_exit_t stmt_exit_t;

typedef struct parser_t parser_t;

typedef rda_struct(stmt_t) scope_t;
#define scope_none \
  (scope_t) { 0 }

struct stmt_fn_define_t {
  char *id;
  scope_t scope;
};

struct stmt_exit_t {
  struct rstr id;
};

enum stmt_type_e {
  stmt_type_none,
  stmt_type_fn_define,
  stmt_type_exit,
  stmt_type_scope
};

// Disable warning C4201: nonstandard extension used: nameless struct/union on
// MSVC
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif

struct stmt_t {
  stmt_type_e type;
  struct {
    stmt_fn_define_t fn_define;
    stmt_exit_t exit;
    scope_t scope;
  };
};
#define stmt_none \
  (stmt_t) { 0 }

#ifdef _MSC_VER
#pragma warning(pop)
#endif

static inline stmt_t stmt_fn_define(stmt_fn_define_t *t_stmt_fn_define) {
  return (stmt_t){.type = stmt_type_fn_define, .fn_define = *t_stmt_fn_define};
}
static inline stmt_t stmt_exit(stmt_exit_t *t_stmt_exit) {
  return (stmt_t){.type = stmt_type_exit, .exit = *t_stmt_exit};
}
static inline stmt_t stmt_scope(scope_t *t_stmt_scope) {
  return (stmt_t){.type = stmt_type_scope, .scope = *t_stmt_scope};
}

typedef rda_struct(stmt_t) prg_t;
#define prg_none \
  (prg_t) { 0 }

struct parser_t {
  rda_allocator parser_allocator;
  tokens_t tokens;
  size_t i;
};

#define parser_create(t_parser) \
  parser_t t_parser;            \
  parser_init(&t_parser)
void parser_init(parser_t *t_parser);

#define parser_peek(t_parser, t_offset) \
  rda_at(((t_parser)->tokens), ((t_parser)->i + t_offset))

token_t parser_consume(parser_t *t_parser);

stmt_t parse_stmt(parser_t *t_parser);
scope_t parse_scope(parser_t *t_parser);
prg_t parse_prg(parser_t *t_parser, const char *t_src);

#ifdef PARSER_IMPLEMENTATION

#define RIT_STR_IMPLEMENTATION
#include "external/rit_str.h"

#define TOKENIZER_IMPLEMENTATION
#include "tokenizer.h"

void *parser_allocator_alloc(void *t_arena, size_t t_size_in_bytes) {
  return arena_alloc((Arena *)t_arena, t_size_in_bytes);
}
void parser_allocator_free(void *t_arena, void *t_ptr) {
  (void)t_ptr;
  (void)t_arena;
}
void *parser_allocator_realloc(void *t_arena, void *t_old_ptr,
                               size_t t_old_size_in_bytes,
                               size_t t_new_size_in_bytes) {
  return arena_realloc((Arena *)t_arena, t_old_ptr, t_old_size_in_bytes,
                       t_new_size_in_bytes);
}

void parser_init(parser_t *t_parser) {
  // Allocate a buffer of 1000000 8 byte chunks(8 MB)
  Arena *arena = malloc(sizeof(Arena));
  *arena = arena_init(1000000);
  t_parser->parser_allocator.alloc = parser_allocator_alloc;
  t_parser->parser_allocator.free = parser_allocator_free;
  t_parser->parser_allocator.realloc = parser_allocator_realloc;
  t_parser->parser_allocator.m_ctx = arena;
  t_parser->i = 0;
}

token_t parser_consume(parser_t *t_parser) {
  if (t_parser->i + 1 >= rda_size(t_parser->tokens)) {
    t_parser->i++;
    return rda_back(t_parser->tokens);
  }
  size_t i = t_parser->i++;
  return rda_at(t_parser->tokens, i);
}

stmt_t parse_stmt(parser_t *t_parser) {
  stmt_t stmt = {0};
  if (parser_peek(t_parser, 0).type == token_id) {
    char *id = parser_consume(t_parser).value;
    if (parser_peek(t_parser, 0).type == token_sep_dcolon) {
      if (parser_peek(t_parser, 0).type == token_kw_fn) {
        // If the scope struct is empty, then there was not a valid scope to
        // parse.
        scope_t scope = parse_scope(t_parser);
        if (scope.m_capacity != 0) {
          stmt_fn_define_t fn_define = {.id = id, .scope = scope};
          stmt = stmt_fn_define(&fn_define);
        }
      }
    }
  }
  return stmt;
}

scope_t parse_scope(parser_t *t_parser) {
  scope_t scope = {0};
  rda_init(scope, 50, sizeof(stmt_t), &(t_parser->parser_allocator));
  for (stmt_t stmt = parse_stmt(t_parser);
       t_parser->i < rda_size(t_parser->tokens) && stmt.type != stmt_type_none;
       stmt = parse_stmt(t_parser)) {
    rda_push_back(scope, stmt, &(t_parser->parser_allocator));
  }
  return scope;
}

prg_t parse_prg(parser_t *t_parser, const char *t_src) {
  tokenizer_create(tokenizer, t_src, &(t_parser->parser_allocator));
  t_parser->tokens = tokenize(&tokenizer);
  rda_for_each(it, t_parser->tokens) {
    printf("Token type: %s, token value: %s\n", token_type_to_str(it->type),
           it->value);
  }
  prg_t prg = {0};
  rda_init(prg, 100, sizeof(stmt_t), &(t_parser->parser_allocator));
  rda_clear(prg);
  for (stmt_t stmt = parse_stmt(t_parser);
       t_parser->i < rda_size(t_parser->tokens) && stmt.type != stmt_type_none;
       stmt = parse_stmt(t_parser)) {
    rda_push_back(prg, stmt, &(t_parser->parser_allocator));
  }
  arena_free(t_parser->parser_allocator.m_ctx);
  free(t_parser->parser_allocator.m_ctx);
  return prg;
}

#endif  // PARSER_IMPLEMENTATION
