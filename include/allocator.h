#ifndef ALLOCATOR_H_INCLUDED
#define ALLOCATOR_H_INCLUDED

#include <stddef.h>

void *arena_allocator_alloc(void *t_arena, size_t t_size_in_bytes);
void arena_allocator_free(void *t_arena, void *t_ptr);
void *arena_allocator_realloc(void *t_arena, void *t_old_ptr,
                              size_t t_old_size_in_bytes,
                              size_t t_new_size_in_bytes);

#endif  // ALLOCATOR_H_INCLUDED
