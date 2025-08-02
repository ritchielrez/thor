// LICENSE
// See end of the file for license information.

#ifndef ARENA_ALLOCATOR_INCLUDED
#define ARENA_ALLOCATOR_INCLUDED

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define empty_buffer_ptr (void *)-1
#define DEFAULT_CHUNK_MAX_COUNT 8192

typedef struct Buffer Buffer;
typedef struct Arena Arena;

/// @brief Allocates an array in an arena
/// @param arena The arena where data gets allocated
/// @param type The type of the array
/// @param count The number of elements in the array
#define arena_alloc_arr(arena, type, count) \
  ((type *)arena_alloc(arena, sizeof(type) * count))

/// @brief Allocates a struct in an arena
/// @param arena The arena where data gets allocated
/// @param type The struct
#define arena_alloc_struct(arena, type) \
  ((type *)arena_alloc(arena, sizeof(type)))

// Disable warning C4200: nonstandard extension used: zero-sized array in
// struct/union in MSVC
#pragma warning(push)
#pragma warning(disable : 4200)

/// @brief Holds data as uintptr(usually 8 byte) chunks.
///
/// Buffers are just like nodes in a linked list
struct Buffer {
  Buffer *m_next;
  size_t m_chunk_max_count;
  size_t m_chunk_current_count;
  /// holds the actual memory chunks, where user data is stored
  uintptr_t m_data[];
};

// Enable the warning again
#pragma warning(pop)

/// @brief Arena is just a growing list of buffers.
///
/// An arena usually looks like this:
/// m_begin -> next buffer -> active buffer
struct Arena {
  /// contains the starting buffer
  Buffer *m_begin;
  /// contains the active buffer
  Buffer *m_active;
  /// contains the max chunk count of arena
  size_t m_chunk_max_count;
};

static inline Arena arena_init(size_t t_chunk_max_count) {
  return (Arena){
      .m_begin = empty_buffer_ptr,
      .m_active = empty_buffer_ptr,
      .m_chunk_max_count = t_chunk_max_count > DEFAULT_CHUNK_MAX_COUNT
                               ? t_chunk_max_count
                               : DEFAULT_CHUNK_MAX_COUNT};
}

/// @brief Creates a new buffer, where chunks of bytes are allocated
/// @param t_chunk_count Maximum number of chunks the buffer can hold
/// @return Buffer*
Buffer *buffer_new(size_t t_chunk_count);

/// @brief Frees up a buffer
/// @param t_buffer The buffer to be freed
/// @return void
inline void buffer_free(Buffer *t_buffer) { free(t_buffer); }

/// @brief Allocate some data inside an arena.
///
/// The allocated data are stored in a buffer.
/// If the data is too big, a new buffer will be created.
///
/// @param t_arena The arena where data gets allocated
/// @param t_size_in_bytes The requested number of bytes to be allocated
/// @return void*
void *arena_alloc(Arena *t_arena, size_t t_size_in_bytes);

/// @brief Resize some old data insdie an arena
///
/// The allocated data are stored in a buffer.
/// If the data is too big, a new buffer will be created.
///
/// @param t_arena The arena where data gets allocated
/// @param t_old_ptr The old ptr where the data is held
/// @param t_old_size_in_bytes The size of the old pointer
/// @param t_new_size_in_bytes The size of the new pointer
/// @return void*
void *arena_realloc(Arena *t_arena, void *t_old_ptr, size_t t_old_size_in_bytes,
                    size_t t_new_size_in_bytes);

/// @brief Resets the allocated chunk count of an arena
/// @param t_arena The arena that will be resetted
/// @return void
void arena_reset(Arena *t_arena);

/// @brief Frees up an arena
/// @param t_arena The arena that will be freed
/// @return void
void arena_free(Arena *t_arena);

#endif  // ARENA_ALLOCATOR_INCLUDED
#ifdef ARENA_ALLOCATOR_IMPLEMENTATION

Buffer *buffer_new(size_t t_chunk_count) {
  size_t size_in_bytes = sizeof(Buffer) + sizeof(uintptr_t) * t_chunk_count;
  Buffer *new_buffer = malloc(size_in_bytes);

  new_buffer->m_next = empty_buffer_ptr;
  new_buffer->m_chunk_max_count = t_chunk_count;
  new_buffer->m_chunk_current_count = 0;

  return new_buffer;
}

void *arena_alloc(Arena *t_arena, size_t t_size_in_bytes) {
  if (t_arena == NULL) {
    fprintf(stderr, "Error, no valid arena was provided\n");
    exit(EXIT_FAILURE);
  }
  Arena *arena = (Arena *)t_arena;
  if (arena->m_begin == NULL) {
    fprintf(stderr, "Error, buffers are freed in the provided arena\n");
  }

  // To understand the following code, you need to have proper knowledge about
  // memory alignment. Align the requsted size to 8 bytes
  t_size_in_bytes = t_size_in_bytes + (sizeof(uintptr_t) - 1);
  size_t chunk_count = t_size_in_bytes / sizeof(uintptr_t);

  if (arena->m_active == empty_buffer_ptr) {
    // If there is no active buffer in an arena, there also should not be a
    // starting buffer
    assert(arena->m_begin == empty_buffer_ptr);
    size_t chunk_max_count = arena->m_chunk_max_count;
    if (chunk_max_count < chunk_count) chunk_max_count = chunk_count;
    arena->m_active = buffer_new(chunk_max_count);
    arena->m_begin = arena->m_active;
  }

  if (arena->m_active->m_chunk_current_count + chunk_count >
      arena->m_active->m_chunk_max_count) {
    size_t chunk_max_count = arena->m_chunk_max_count;
    if (chunk_max_count < chunk_count) chunk_max_count = chunk_count;
    arena->m_active->m_next = buffer_new(chunk_max_count);
    arena->m_active = arena->m_active->m_next;
  }

  void *result =
      &(arena->m_active->m_data[arena->m_active->m_chunk_current_count]);
  arena->m_active->m_chunk_current_count += chunk_count;
  return result;
}

void *arena_realloc(Arena *t_arena, void *t_old_ptr, size_t t_old_size_in_bytes,
                    size_t t_new_size_in_bytes) {
  if (t_arena == NULL) {
    fprintf(stderr, "Error, no valid arena was provided\n");
    exit(EXIT_FAILURE);
  }

  t_old_size_in_bytes = t_old_size_in_bytes + (sizeof(uintptr_t) - 1);
  size_t old_chunk_count = t_old_size_in_bytes / sizeof(uintptr_t);
  t_new_size_in_bytes = t_new_size_in_bytes + (sizeof(uintptr_t) - 1);
  size_t new_chunk_count = t_new_size_in_bytes / sizeof(uintptr_t);
  if (old_chunk_count >= new_chunk_count) {
    return t_old_ptr;
  }

  void *result = arena_alloc(t_arena, t_new_size_in_bytes);
  char *old_ptr_bytes = t_old_ptr;
  char *new_ptr_bytes = result;

  for (size_t i = 0; i < t_old_size_in_bytes; ++i) {
    new_ptr_bytes[i] = old_ptr_bytes[i];
  }

  return result;
}

void arena_reset(Arena *t_arena) {
  if (t_arena == NULL) {
    fprintf(stderr, "Error, no valid arena was provided\n");
    exit(EXIT_FAILURE);
  }

  Buffer *current_buffer = t_arena->m_begin;
  while (current_buffer != empty_buffer_ptr) {
    current_buffer->m_chunk_current_count = 0;
    current_buffer = current_buffer->m_next;
  }
}

void arena_free(Arena *t_arena) {
  if (t_arena == NULL) {
    fprintf(stderr, "Error, no valid arena was provided\n");
    exit(EXIT_FAILURE);
  }
  Arena *arena = (Arena *)t_arena;

  Buffer *current_buffer = arena->m_begin;
  while (current_buffer != empty_buffer_ptr) {
    Buffer *next_buffer = current_buffer->m_next;
    current_buffer->m_chunk_max_count = 0;
    current_buffer->m_chunk_current_count = 0;
    free(current_buffer);
    current_buffer = next_buffer;
  }

  // Assigning null value to freed pointers is a good practice
  // because this ensures that by accessing any freed pointers
  // does not cause undefined behaviours, even though accessing
  // null values do cause them too, it is more easily debuggable.
  arena->m_begin = NULL;
  arena->m_active = NULL;
}

#endif  // ARENA_ALLOCATOR_IMPLEMENTATION

/*
The MIT License (MIT)

Copyright 2024 Ritchiel Reza

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
