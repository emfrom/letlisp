#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>
#include <gmp.h>

/**
 * Memory
 */
void mem_startup();

void *gcx_malloc(size_t size);

void *gcx_realloc(void *ptr, size_t size);

void gcx_free(void *ptr);


#endif
