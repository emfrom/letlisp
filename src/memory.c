#include <gc/gc.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#include <string.h>

#include "memory.h"


void *mem_realloc_cb(void *ptr, size_t oldsize, size_t newsize) {
#if 1
  ptr = gcx_realloc(ptr, newsize);

#else
  //Try is memory corruption is a problem (again)
  void *new = gcx_malloc(newsize);

  memcpy(new, ptr, (oldsize < newsize) ? oldsize : newsize);

  return new;
#endif
}

void mem_free_cb(void *, size_t oldsize) {
  return;
}

/**
 * Memory
 */
void mem_startup() {
  GC_INIT();

  //Numbers
  mp_set_memory_functions(gcx_malloc,
			  mem_realloc_cb,
			  mem_free_cb); 
}

void *gcx_malloc(size_t size) {
  void *ptr = GC_MALLOC(size);
  if(!ptr) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }

  return ptr;
}

void *gcx_realloc(void *ptr, size_t size) {
  ptr = GC_REALLOC(ptr, size);
  if(!ptr) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }
  
  return ptr;
}

void gcx_free(void *ptr) {
  return;
}
