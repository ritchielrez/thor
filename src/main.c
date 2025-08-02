#include <stdio.h>
#include <stdlib.h>

#define ARENA_ALLOCATOR_IMPLEMENTATION
#define TOKENIZER_IMPLEMENTATION
#define PARSER_IMPLEMENTATION
#include "defines.h"
#include "external/arena_allocator.h"
#include "parser.h"
#include "utils.h"

void *arena_allocator_alloc(void *t_arena, size_t t_size_in_bytes) {
  return arena_alloc((Arena *)t_arena, t_size_in_bytes);
}
void arena_allocator_free(void *t_arena, void *t_ptr) {
  (void)t_ptr;
  (void)t_arena;
}
void *arena_allocator_realloc(void *t_arena, void *t_old_ptr,
                              size_t t_old_size_in_bytes,
                              size_t t_new_size_in_bytes) {
  return arena_realloc((Arena *)t_arena, t_old_ptr, t_old_size_in_bytes,
                       t_new_size_in_bytes);
}

Arena arena;
rstr_allocator allocator = {arena_allocator_alloc, arena_allocator_free,
                            arena_allocator_realloc, &arena};

void help_msg(const char *subcmd, const char *utils_prg_name) {
  if (subcmd == NULL) {
    printf("Usage: %s <subcommand> [args]\n", utils_prg_name);
    printf("subcommands:\n");
    printf("    com     Compile .th file\n");
    printf("    run     Compile and run .th file\n");
    printf("    help    Print this help usage information\n");
  } else if (!strcmp(subcmd, "com")) {
    printf("No help information avalaible for the \"com\" subcommand\n");
  } else if (!strcmp(subcmd, "run")) {
    printf("No help information avalaible for the \"run\" subcommand\n");
  } else {
    fprintf(stderr, "Error: unknown subcommand %s\n", subcmd);
    exit(1);
  }
}

int main(int argc, char **argv) {
  arena = arena_init(0);
#ifdef BUILD_WINDOWS
  char buf[BIN_NAME_MAX_SZ];
  char *prg = utils_prg_name(&argc, &argv, buf, BIN_NAME_MAX_SZ);
#endif

#ifdef BUILD_LINUX
  char *prg = utils_shift_args(&argc, &argv);
#endif

  if (argc < 1) {
    help_msg(utils_shift_args_p(&argc, &argv), prg);
    fprintf(stderr, "Error: no subcommand provided\n");
    return 1;
  }

  char *subcmd = utils_shift_args(&argc, &argv);
  if (!strcmp(subcmd, "help")) {
    help_msg(utils_shift_args_p(&argc, &argv), prg);
  } else if (!strcmp(subcmd, "com")) {
    parser_create(parser);
    parse_prg(&parser, argv[0]);
  } else if (!strcmp(subcmd, "run")) {
    assert(0 && "run subcommand is not implemented");
  } else {
    help_msg(utils_shift_args_p(&argc, &argv), prg);
    fprintf(stderr, "Error: unknown subcommand %s\n", subcmd);
    return 1;
  }
}
