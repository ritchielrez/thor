#include <stdio.h>
#include <stdlib.h>

#include "allocator.h"
#include "defines.h"
#include "generator.h"
#include "libraries/arena_allocator.h"
#include "parser.h"
#include "tokenizer.h"
#include "utils.h"

Arena arena = {nullptr, nullptr};
rstr_allocator allocator = {arena_allocator_alloc, arena_allocator_free,
                            arena_allocator_realloc, &arena};

void help_msg(const char *subcmd, const char *utils_prg_name) {
  if (subcmd == nullptr) {
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
  char *prg = utils_shift_args(&argc, &argv);

  if (argc < 1) {
    help_msg(utils_shift_args_p(&argc, &argv), prg);
    fprintf(stderr, "Error: no subcommand provided\n");
    return 1;
  }

  char *subcmd = utils_shift_args(&argc, &argv);
  if (!strcmp(subcmd, "help")) {
    help_msg(utils_shift_args_p(&argc, &argv), prg);
  } else if (!strcmp(subcmd, "com")) {
    parser_create(parser, "examples/variables.th");
    parse(&parser);
    parser_deinit(&parser);
  } else if (!strcmp(subcmd, "run")) {
    assert(0 && "run subcommand is not implemented");
  } else {
    help_msg(utils_shift_args_p(&argc, &argv), prg);
    fprintf(stderr, "Error: unknown subcommand %s\n", subcmd);
    return 1;
  }
}
