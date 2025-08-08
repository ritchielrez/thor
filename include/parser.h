#ifndef PARSER_H_INCLUDED

#include <stdint.h>

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
  };
} node_stmt;

typedef rda_struct(node_stmt) node_prg;

typedef struct {
  node_prg prg;
  rda_allocator *allocator;
} parser_t;

#define parser_create(t_parser_name) parser_t t_parser_name = parser_init()

parser_t parser_init();
void parse(parser_t *t_parser, const char *t_file);
void parser_deinit(parser_t *t_parser);

#endif  // PARSER_H_INCLUDED
