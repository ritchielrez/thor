#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "libraries/rit_str.h"

/// @internal
INTERNAL_DEF inline char *utils_shift_args_p(int *t_argc, char ***t_argv) {
  if (*t_argc == 0) return NULL;
  char *ret = (*t_argv)[0];
  (*t_argv) = *t_argv + 1;
  (*t_argc)--;
  return ret;
}

/// @internal
INTERNAL_DEF inline char *utils_shift_args_with_location(const char *file,
                                                         int line, int *t_argc,
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

static inline void utils_putd(int num) { printf("%d\n", num); }

static inline struct rstr utils_read_file(const char *t_file,
                                          rstr_allocator *t_allocator) {
  FILE *fp = fopen(t_file, "r");
  rstr(stream_rstr, rsv_lit(""), t_allocator);
  rstr_getstream(fp, stream_rstr, t_allocator);
  return stream_rstr;
}

#endif  // UTILS_H_INCLUDED
