// LICENSE
// See end of the file for license information.
#ifndef RIT_DYN_ARR_H_INCLUDED
#define RIT_DYN_ARR_H_INCLUDED

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__GNUC__)
#define gettype typeof
#elif defined(_MSC_VER)
#define gettype __typeof__
// Disable MSVC warning 4702: unreachable code
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

#define DEFAULT_ARR_CAP 16

#ifndef RIT_STR_H_INCLUDED
/// @brief Custom allocator interface.
/// Functions allocating memory takes a custom allocator based off this
/// interface as a parameter.
typedef struct {
  void *(*alloc)(void *, size_t);
  void (*free)(void *, void *);
  void *(*realloc)(void *, void *, size_t, size_t);
  void *m_ctx;  // The arena, stack or etc where the memory would be allocated,
                // NULL if none
} rda_allocator;
#else
typedef rstr_allocator rda_allocator;
#endif  // RIT_STR_H_INCLUDED

/// @brief Dynamic array struct
#define rda_struct(t_type) \
  struct {                 \
    size_t m_size;         \
    size_t m_capacity;     \
    size_t m_objsize;      \
    t_type *m_data;        \
  }

#define rda_size(t_rda) (t_rda).m_size

#define rda_capacity(t_rda) (t_rda).m_capacity

#define rda_init(t_rda, t_size, t_objsize, t_allocator)                   \
  do {                                                                    \
    size_t capacity =                                                     \
        DEFAULT_ARR_CAP < t_size * 2 ? t_size * 2 : DEFAULT_ARR_CAP;      \
    t_rda.m_data =                                                        \
        (t_allocator)->alloc((t_allocator)->m_ctx, capacity * t_objsize); \
    if (!t_rda.m_data) {                                                  \
      fprintf(stderr, "Error: allocation failed, file: %s, line: %d\n",   \
              __FILE__, __LINE__);                                        \
      exit(EXIT_FAILURE);                                                 \
    }                                                                     \
    t_rda.m_size = t_size;                                                \
    t_rda.m_capacity = capacity;                                          \
    t_rda.m_objsize = t_objsize;                                          \
  } while (0)

/// @brief Set the capacity of a array.
#define rda_reserve(t_rda, t_new_capacity, t_allocator)                   \
  if (t_new_capacity > rda_capacity(t_rda)) {                             \
    t_rda.m_data = (t_allocator)                                          \
                       ->realloc((t_allocator)->m_ctx, t_rda.m_data,      \
                                 rda_capacity(t_rda) * t_rda.m_objsize,   \
                                 t_new_capacity * t_rda.m_objsize);       \
    if (!t_rda.m_data) {                                                  \
      fprintf(stderr, "Error: reallocation failed, file: %s, line: %d\n", \
              __FILE__, __LINE__);                                        \
      exit(EXIT_FAILURE);                                                 \
    }                                                                     \
    t_rda.m_capacity = t_new_capacity;                                    \
  }

#define rda_swap(t_rda, t_rda_other)              \
  do {                                            \
    size_t tmp_size = rda_size(t_rda);            \
    t_rda.m_size = rda_size(t_rda_other);         \
    t_rda_other.m_size = tmp_size;                \
    size_t tmp_capacity = rda_capacity(t_rda);    \
    t_rda.m_capacity = rda_capacity(t_rda_other); \
    t_rda_other.m_capacity = tmp_capacity;        \
    void *tmp_data = rda_data(t_rda);             \
    t_rda.m_data = rda_data(t_rda_other);         \
    t_rda_other.m_data = tmp_data;                \
  } while (0)

/// @brief Makes a non-binding request to make the capacity of a array equal to
/// its size. In this library this is definied as a no-op function.
#define rda_shrink_to_fit(t_rda) (void)t_rda

/// @brief Returns a pointer to the internal data of the rda struct.
#define rda_data(t_rstr) (t_rstr).m_data

#define rda_free(t_rda, t_allocator) \
  (t_allocator)->free((t_allocator)->m_ctx, t_rda.m_data)

/// @param Check if a array is empty.
#define rda_empty(t_rda) rda_size(t_rda) == 0

/// @brief Empty out a array.
#define rda_clear(t_rda) t_rda.m_size = 0

/// @brief Create a rda.
#define rda(t_type, t_rda, t_size, t_allocator) \
  rda_struct(t_type) t_rda = {};                \
  rda_init(t_rda, t_size, sizeof(t_type), t_allocator)

/// @param t_rda Where to copy
/// @param t_rda_other What to copy
/// @param t_size The size of subarray of t_rda_other
#define rda_cp(t_type, t_rda, t_rda_other, t_index, t_size, t_allocator)       \
  rda_struct(t_type) t_rda = {};                                               \
  if (t_index > rda_size(t_rda_other)) {                                       \
    fprintf(stderr,                                                            \
            "Error: starting index of subarray out of bounds of the array, "   \
            "file: %s, line: %d\n",                                            \
            __FILE__, __LINE__);                                               \
    exit(EXIT_FAILURE);                                                        \
  } else if (t_index + t_size > rda_size(t_rda_other)) {                       \
    fprintf(stderr,                                                            \
            "Error: size of subarray greater than the array, file: %s, line: " \
            "%d\n",                                                            \
            __FILE__, __LINE__);                                               \
    exit(EXIT_FAILURE);                                                        \
  }                                                                            \
  rda_init(t_rda, t_size - t_index, sizeof(t_type), t_allocator);              \
  for (size_t i = 0, j = t_index; i < t_size; i++, j++) {                      \
    rda_at(t_rda, i) = rda_at(t_rda_other, j);                                 \
  }

#define rda_ret_ptr_at_index(t_rda, t_index)                                 \
  ((((t_index) >= rda_size((t_rda))))                                        \
       ? (fprintf(stderr,                                                    \
                  "Error: array index out of bounds, index: %zu, file: %s, " \
                  "line: %d\n",                                              \
                  t_index, __FILE__, __LINE__),                              \
          exit(EXIT_FAILURE), &((t_rda).m_data[(t_index)]))                  \
       : &((t_rda).m_data[(t_index)]))

#define rda_at(t_rda, t_index) (*(rda_ret_ptr_at_index(t_rda, t_index)))

/// @brief Get the pointer to the first element of an array
#define rda_begin(t_rda) (&(t_rda.m_data[0]))
/// @brief Get the pointer to the past-the-end element of an array
#define rda_end(t_rda) (&(t_rda.m_data[rda_size(t_rda)]))

/// @brief Get the first element of an array
#define rda_front(t_rda) (t_rda.m_data[0])
/// @brief Get the last element of an array
#define rda_back(t_rda) (t_rda.m_data[rda_size(t_rda) - 1])

#define rda_push_back(t_rda, t_val, t_allocator)                \
  if (rda_capacity(t_rda) <= rda_size(t_rda) + 1) {             \
    rda_reserve(t_rda, (rda_size(t_rda) + 1) * 2, t_allocator); \
  }                                                             \
  t_rda.m_data[rda_size(t_rda)] = t_val;                        \
  t_rda.m_size++

#define rda_pop_back(t_rda) t_rda.m_size--

#define rda_append_val(t_rda, t_size, t_val, t_allocator) \
  for (size_t i = 1; i <= t_size; i++) {                  \
    rda_push_back(t_rda, t_val, t_allocator);             \
  }

#define rda_append_arr(t_rda, t_arr, t_allocator)                 \
  for (size_t i = 0; i < sizeof(t_arr) / sizeof(t_arr[0]); i++) { \
    rda_push_back(t_rda, t_arr[i], t_allocator);                  \
  }

#define rda_append_rda(t_rda, t_rda_other, t_allocator)        \
  for (size_t i = 0; i < rda_size(t_rda_other); i++) {         \
    rda_push_back(t_rda, rda_at(t_rda_other, i), t_allocator); \
  }

/// @brief Append variable number of values at the end of an array
#define rda_append(t_rda, t_allocator, t_val1, ...) \
  rda_append_arr(t_rda, ((gettype(t_val1)[]){t_val1, __VA_ARGS__}), t_allocator)

/// @brief Remove elements from the end of the array
#define rda_remove(t_rda, t_count)        \
  for (size_t i = 1; i <= t_count; i++) { \
    rda_pop_back(t_rda);                  \
  }

/// @brief Changes the number of characters stored
#define rda_resize(t_rda, t_size, t_val, t_allocator) \
  rda_clear(t_rda);                                   \
  rda_append_val(t_rda, t_size, t_val, t_allocator)

/// @brief Insert characters in the array at t_index.
#define rda_insert(t_rda, t_index, t_size, t_val, t_allocator)         \
  do {                                                                 \
    rda_append_val(t_rda, t_size, t_val, t_allocator);                 \
    for (size_t i = rda_size(t_rda) - 1; i >= t_index + t_size; i--) { \
      rda_at(t_rda, i) = rda_at(t_rda, i - t_size);                    \
      rda_at(t_rda, i - t_size) = t_val;                               \
    }                                                                  \
  } while (0)

/// @brief Remove characters in the array at t_index
#define rda_erase(t_rda, t_index, t_size)                         \
  do {                                                            \
    for (size_t i = t_index + t_size; i < rda_size(t_rda); i++) { \
      rda_at(t_rda, i - t_size) = rda_at(t_rda, i);               \
    }                                                             \
    rda_remove(t_rda, t_size);                                    \
  } while (0)

/// @brief Assign t_count number of t_val's in an array
#define rda_assign_val(t_rda, t_val, t_count, t_allocator) \
  rda_clear(t_rda);                                        \
  rda_append_val(t_rda, t_val, t_count, t_allocator)

#define rda_assign_arr(t_rda, t_arr, t_allocator) \
  rda_clear(t_rda);                               \
  rda_append_arr(t_rda, t_arr, t_allocator)

#define rda_assign(t_rda, t_allocator, t_val1, ...) \
  rda_clear(t_rda);                                 \
  rda_append(t_rda, t_allocator, t_val1, __VA_ARGS__)

#define rda_for_each(t_it, t_rda)                                            \
  for (gettype(rda_begin(t_rda)) it = rda_begin(t_rda); it < rda_end(t_rda); \
       it++)

#endif  // RIT_DYN_ARR_H_INCLUDED

/*
The MIT License (MIT)

Copyright 2024 Ritchiel Reza

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
