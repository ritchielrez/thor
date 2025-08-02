#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#define RIT_STR_IMPLEMENTATION
#include "external/rit_str.h"

static inline char *utils_shift_args_p(int *t_argc, char ***t_argv) {
  if (*t_argc == 0) return NULL;
  char *ret = (*t_argv)[0];
  (*t_argv) = *t_argv + 1;
  (*t_argc)--;
  return ret;
}

static inline char *utils_shift_args_with_location(const char *file, int line,
                                                   int *t_argc,
                                                   char ***t_argv) {
  char *ret = utils_shift_args_p(t_argc, t_argv);
  if (!ret) {
    fprintf(stderr, "No t_arguments left to shift, file: %s, line: %d\n", file,
            line);
    exit(1);
  }
  return ret;
}

#define utils_shift_args(t_argc, t_argv) \
  utils_shift_args_with_location(__FILE__, __LINE__, t_argc, t_argv)

static inline char *utils_prg_name(int *t_argc, char ***t_argv, char *buffer,
                                   size_t buffer_sz) {
#ifdef BUILD_LINUX
    (void)t_argc;
    (void)t_argv;
    (void)buffer;
    (void)buffer_sz;
    printf("utils_prg_name() need not to be called in Linux.\n");
    return *t_argv[0];
#endif

#ifdef BUILD_WINDOWS
  char *prg = utils_shift_args(t_argc, t_argv);
  size_t prg_sz = strlen(prg);

  buffer[buffer_sz - 1] = '\0';
  s64 j = buffer_sz - 2;
  for (s64 i = prg_sz - 1; i >= 0 && prg[i] != '\\'; i--, j--) {
    assert(j >= 0 && "Buffer is too small to hold the name of the program");
    buffer[j] = prg[i];
  }
  return buffer + j + 1;
#endif
}

static inline void utils_putd(int num) { printf("%d\n", num); }

static inline struct rstr utils_read_file(const char *t_file,
                                   rstr_allocator *t_allocator) {
  FILE *fp = fopen(t_file, "r");
  rstr(stream_rstr, rsv_lit(""), t_allocator);
  rstr_getstream(fp, stream_rstr, t_allocator);
  return stream_rstr;
}

#endif  // UTILS_H_INCLUDED
