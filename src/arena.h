/* [1] arena.h - Header for simple memory arena allocator. */
#pragma once
#include <stddef.h>

/* [2] Reset the arena to empty (all allocations invalidated) */
void arena_reset(void);

/* [3] Allocate a block of memory from the arena. Returns pointer or NULL if not enough space. */
void *arena_alloc(size_t size);

/* [4] Get the amount of memory used in the arena (in bytes) */
size_t arena_get_used(void);
