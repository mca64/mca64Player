/* [1] arena.c - Simple memory arena allocator for temporary allocations. */
#include <stdint.h>
#include "arena.h"

#define ARENA_SIZE (128 * 1024) /* [2] Total arena size in bytes (128 KB) */
static uint8_t arena_buf[ARENA_SIZE];    /* [3] Arena buffer */
static size_t arena_offset = 0;          /* [4] Current offset in the arena */
static size_t mem_used = 0;              /* [5] Amount of memory used */

/* [6] Reset the arena to empty (all allocations invalidated) */
void arena_reset(void) {
    arena_offset = 0;
    mem_used = 0;
}

/* [7] Allocate a block of memory from the arena. Returns pointer or NULL if not enough space. */
void *arena_alloc(size_t size) {
    if (size == 0) return NULL;
    const size_t align = 8; /* [7.1] Align allocations to 8 bytes */
    size_t off = (arena_offset + (align - 1)) & ~(align - 1);
    if (off + size > ARENA_SIZE) return NULL;
    void *p = &arena_buf[off];
    arena_offset = off + size;
    mem_used = arena_offset;
    return p;
}

/* [8] Get the amount of memory used in the arena (in bytes) */
size_t arena_get_used(void) {
    return mem_used;
}
