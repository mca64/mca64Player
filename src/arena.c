#include <stdint.h>
#include "arena.h"

#define ARENA_SIZE (128 * 1024)
static uint8_t arena_buf[ARENA_SIZE];
static size_t arena_offset = 0;
static size_t mem_used = 0;

void arena_reset(void) {
    arena_offset = 0;
    mem_used = 0;
}
void *arena_alloc(size_t size) {
    if (size == 0) return NULL;
    const size_t align = 8;
    size_t off = (arena_offset + (align - 1)) & ~(align - 1);
    if (off + size > ARENA_SIZE) return NULL;
    void *p = &arena_buf[off];
    arena_offset = off + size;
    mem_used = arena_offset;
    return p;
}
size_t arena_get_used(void) { return mem_used; }
