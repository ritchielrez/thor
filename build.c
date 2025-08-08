#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARENA_ALLOCATOR_IMPLEMENTATION
#define RIT_STR_IMPLEMENTATION
#include "include/build.h"
#include "include/defines.h"
#include "include/libraries/arena_allocator.h"
#include "include/libraries/rit_dyn_arr.h"
#include "include/libraries/rit_str.h"
#include "include/utils.h"

#if defined(__clang__)
char *cc = "clang";
#elif defined(__GNUC__) || defined(__GNUG__)
char *cc = "gcc";
#elif defined(_MSC_VER)
char *cc = "cl";
#else
#error Unknown compiler
#endif

#ifdef _MSC_VER
// Disable MSVC warning 4702: unreachable code
#pragma warning(push)
#pragma warning(disable : 4702)
#endif  // _MSC_VER

#define nullptr (void *)0

char *target = "thor";
char *include_dir = "./include/";

#define SRC_FILES_LEN 4
char *src_files[SRC_FILES_LEN] = {"./src/allocator.c", "./src/main.c",
                                  "./src/parser.c", "./src/tokenizer.c"};

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
Arena arena = {nullptr, nullptr};
rstr_allocator allocator = {arena_allocator_alloc, arena_allocator_free,
                            arena_allocator_realloc, &arena};

void help_msg(const char *subcmd, const char *utils_prg_name) {
  if (subcmd == NULL) {
    printf("Usage: %s <subcommand> [args]\n", utils_prg_name);
    printf("subcommands:\n");
    printf("    com     Compile %s\n", target);
    printf("    run     Compile and run %s\n", target);
    printf("    help    Print help information for command and subcommands\n");
  } else if (!strcmp(subcmd, "com")) {
    printf("No help information avalaible for the \"com\" subcommand\n");
  } else if (!strcmp(subcmd, "run")) {
    printf("Usage: %s run -- [args]\n", utils_prg_name);
    printf("args:\n");
    printf(
        "   The arguments that need to be passed as command line arguments to "
        "%s",
        target);
  } else {
    fprintf(stderr, "Error: unknown subcommand %s", subcmd);
  }
}

void com_prg() {
  cmd(cflags, &allocator);
  if (!strcmp(cc, "clang") || !strcmp(cc, "gcc")) {
    cmd_append(cflags, &allocator, "-g", "-Wall", "-Wextra",
               "-Wno-unknown-pragmas", /* "-fsanitize=address",  */ "-o",
               target);
  } else {
    char outflag[BIN_NAME_MAX_SZ + 8];
    sprintf(outflag, "-Fe%s.exe", target);
    cmd_append(cflags, &allocator, "-EHsc", "-nologo", "-W4", "-Zi", "-MTd",
               "-D_CRT_SECURE_NO_WARNINGS",
               "-DDEBUG", /* "-fsanitize=address", */
               outflag);
  }
  cmd(build_cmd, &allocator);
  cmd_push_back(build_cmd, cc, &allocator);
  cmd_append_cmd(build_cmd, cflags, &allocator);
  for (size_t i = 0; i < SRC_FILES_LEN; ++i) {
    cmd_push_back(build_cmd, src_files[i], &allocator);
  }
  printf("[INFO] Compiling source code...\n[INFO] cmd: ");
  cmd_append(build_cmd, &allocator, "-I", include_dir);
  rda_for_each(it, build_cmd) { printf("%s ", *it); }
  putchar('\n');
  bool proc = cmd_run_sync(build_cmd);
  if (!proc) {
    fprintf(stderr, "Error: cmd_run_sync() failed, file: %s, line: %d\n",
            __FILE__, __LINE__);
    exit(1);
  }
}

void com_and_run_prg(int *argc, char ***argv) {
  com_prg();
  cmd_t run_cmd;
  // Whatever size is specified, double the size is actually allocated.
  // So we are specifying the half of the actual size of the object
  cmd_init(run_cmd, (BIN_NAME_MAX_SZ + 8) / 2, &allocator);
  // Not recommended to change the size of a cmd manually if you do not know
  // what you are doing
  run_cmd.m_size = 0;
  char run_exe[BIN_NAME_MAX_SZ + 8];
#if defined(BUILD_WINDOWS)
  sprintf(run_exe, "%s.exe", target);
  cmd_append(run_cmd, &allocator, run_exe);
#else
  sprintf(run_exe, "./%s", target);
  cmd_append(run_cmd, &allocator, run_exe);
#endif
  while (*argc > 0) {
    char *arg = utils_shift_args(argc, argv);
    cmd_append(run_cmd, &allocator, arg);
  }
  printf("[INFO] Running executable...\n");
  bool proc = cmd_run_sync(run_cmd);
  if (!proc) {
    fprintf(stderr, "Error: cmd_run_sync() failed, file: %s, line: %d\n",
            __FILE__, __LINE__);
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

  char *subcommand = utils_shift_args(&argc, &argv);
  if (!strcmp(subcommand, "help")) {
    help_msg(utils_shift_args_p(&argc, &argv), prg);
    rda(int, test, 0, &allocator);
    rda_append(test, &allocator, 1, 2, 3, 4);
  } else if (!strcmp(subcommand, "com")) {
    // TODO: implement an option to turn on/off the sanitizer for compilation.
    com_prg();
  } else if (!strcmp(subcommand, "run")) {
    char *arg = utils_shift_args_p(&argc, &argv);
    if (arg && strcmp(arg, "--")) {
      fprintf(stderr, "Error: no valid argument provided to run subcommand\n");
    }
    com_and_run_prg(&argc, &argv);
  } else {
    help_msg(utils_shift_args_p(&argc, &argv), prg);
    fprintf(stderr, "Error: unknown subcommand %s\n", subcommand);
    return 1;
  }
  arena_free(&arena);
}
