#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdint.h>

// todo: remove placeholders
#warning "PLACEHOLDERS FOR U128 and value"
struct uint128_s {
  uint64_t lsb;
  uint64_t msb;
};

typedef union {
  __uint128_t whole;
  __uint128_t w;
  struct uint128_s halfs;
  struct uint128_s h;
} uint128_t;

typedef uint32_t hash_t;

typedef struct value_s {
  uint128_t id;
  hash_t hash;
} *value;

void hashtable_init();

void hashtable_destroy();

void hashtable_set(value new);

void hashtable_delete(value delete);

value hashtable_get(value find);

#endif
