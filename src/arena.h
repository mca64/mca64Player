// arena.h
#pragma once
#include <stddef.h>
void arena_reset(void);
void *arena_alloc(size_t size);
size_t arena_get_used(void);
