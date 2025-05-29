#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <sched.h>
#include <stdbool.h>

#include "memory.h"

// TODO: Remove placeholders
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

typedef struct {
  uint128_t id;
  hash_t hash;
  
} *value;


static inline bool uint128_cmp(const uint128_t *a, const uint128_t *b) {
  //Force 128 comparison
  return a->w == b->w;
}

/**
 * Hashtable
 *
 * Quick hack, but might be fast :)
 */



typedef struct hashtable_entry_s *hashtable_entry;
typedef atomic_flag lock_t;

struct hashtable_entry_s {
  value value;
  
  atomic_flag lock;
};

const value EMPTY = NULL;
const value DEAD = (value)UINTPTR_MAX;

#define HASHMASK 0x3FFFFF
#define HASHTABLE_SIZE (HASHMASK + 1)

// One table to rule them all
hashtable_entry hashtable = NULL;



// TODO: yield to microthread instead 
static inline void lock_lock(lock_t *lock) {
    while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire))
      sched_yield();
}

static inline void lock_unlock(lock_t *lock) {
    atomic_flag_clear_explicit(lock, memory_order_release);
}


void hashtable_init() {
  assert(hashtable == NULL);
  
  //Storage
  hashtable = gcx_malloc(HASHTABLE_SIZE * sizeof(struct hashtable_entry_s));
  memset(hashtable, 0, HASHTABLE_SIZE * sizeof(struct hashtable_entry_s));

  //Sanity
  assert(hashtable[0].value == NULL);
}

void hashtable_destroy() {

  gcx_free(hashtable);

  hashtable = NULL;
}

// Open with linear probing
void hashtable_set(value value) {
  assert(value != EMPTY &&
	 value != DEAD);
  
  hash_t hash_position;
  hash_position = value->hash;

  hashtable_entry entry;
  entry = hashtable + value->hash;

  
  //Lock 
  lock_lock(&entry->lock);
  
  // Probe
  while(entry->value != EMPTY &&
	 entry->value != DEAD &&
	!uint128_cmp(&value->id, &entry->value->id)) {
    hash_position = (hash_position + 1) % HASHTABLE_SIZE;
    entry = hashtable + hash_position;
    
    lock_lock(&entry->lock);
  }

  entry->value = value;
  
  // all done 
  lock_unlock(&entry->lock);
  return;
}

void hashtable_delete(uint64_t id) {
  uint32_t hash = hashtable_hash(id);
  pthread_rwlock_wrlock(locks + hash);

  hashtable_entry entry = &hashtable[hash];

  //First?
  if (entry->id == id) {
    if (entry->next) {
      hashtable_entry next = entry->next;
      entry->id = next->id;
      entry->value = next->value;
      entry->next = next->next;
      gcx_free(next);
    } else {
      // No chain — just clear the slot
      entry->id = 0;
      entry->value = NULL;
      entry->next = NULL;
    }

    pthread_rwlock_unlock(locks + hash);
    return;
  }

  // Scan chain
  hashtable_entry prev = entry;

  while (entry) {
    if (entry->id == id) {
      prev->next = entry->next;
      gcx_free(entry);
      pthread_rwlock_unlock(locks + hash);
      return;
    }
    prev = entry;
    entry = entry->next;
  }

  pthread_rwlock_unlock(locks + hash);
}

void *hashtable_get(uint64_t id) {
  uint32_t hash = hashtable_hash(id);
  pthread_rwlock_rdlock(locks + hash);

  hashtable_entry entry = &hashtable[hash];
  while (entry != NULL) {
    if (entry->id == id) {
      void *value = entry->value;
      pthread_rwlock_unlock(locks + hash);
      return value;
    }
    entry = entry->next;
  }

  pthread_rwlock_unlock(locks + hash);
  return NULL; // Not found
}

#ifdef UNIT_TEST

#include <unistd.h>
#include <fcntl.h>


#define COUNT 10000
#define COLLISIONS 100

uint64_t find_hash_collision(uint64_t original) {
  uint32_t target_hash = hashtable_hash(original);
    for (uint64_t candidate = original + 1; ; candidate++) {
        if (hashtable_hash(candidate) == target_hash) {
            return candidate;
        }
    }
}

int main() {
  printf("Test: reading %d uint64_t from /dev/random: ", COUNT);

  uint64_t *data = malloc(COUNT * sizeof(uint64_t));
  assert(data);

  int fd = open("/dev/random", O_RDONLY);
  if (fd < 0) {
    perror("open /dev/random");
    exit(EXIT_FAILURE);
  }

  ssize_t bytes_needed = COUNT * sizeof(uint64_t);
  ssize_t offset = 0;

  while (offset < bytes_needed) {
    ssize_t r = read(fd, (uint8_t *)data + offset, bytes_needed - offset);
    if (r < 0) {
      perror("read");
      exit(EXIT_FAILURE);
    }
    offset += r;
  }

  close(fd);
  printf("✔\n");

  printf("Hashtable initialising: ");
  hashtable_init();
  printf("✔\n");

  printf("Adding entries: ");
  for (uint64_t i = 1; i < COUNT; ++i) {
    hashtable_set(data[i], (void *)i); 
  }
  printf("✔\n");

  printf("Verifying entries: ");
  for (uint64_t i = 1; i < COUNT; ++i) {
    void *value = hashtable_get(data[i]);
    if (value != (void *)i) {
      fprintf(stderr, "❌ Verification failed for key %llu\n", (unsigned long long)data[i]);
      exit(EXIT_FAILURE);
    }
  }
  printf("✔\n");

  printf("Deleting entries: ");
  for (int i = 1; i < COUNT; ++i) {
    hashtable_delete(data[i]);
  }
  printf("✔\n");


  printf("Verifying deletions (get): ");
  for (int i = 1; i < COUNT; ++i) {
    if (hashtable_get(data[i]) != NULL) {
      fprintf(stderr, "❌ Deletion verification (get) failed for key %llu\n", (unsigned long long)data[i]);
      exit(EXIT_FAILURE);
    }
  }
  printf("✔\n");

  printf("Verifying deletions (memory): ");
  for (uint64_t i = 1; i < HASHTABLE_SIZE; ++i) 
    if(hashtable[i].id ||
       hashtable[i].value ||
       hashtable[i].next) {
      fprintf(stderr, "❌ Deletion verification (mem)failed for key %llu\n", (unsigned long long)data[i]);
      exit(EXIT_FAILURE);
    }
  printf("✔\n");
  
  printf("Redeleting nonexistant entries: ");
  for (int i = 1; i < COUNT; ++i) {
    hashtable_delete(data[i]);
  }
  printf("✔\n");


  printf("Readding entries: ");
  for (uint64_t i = 1; i < COUNT; ++i) {
    hashtable_set(data[i], (void *)i); 
  }
  printf("✔\n");

  printf("Finding collisions: ");
  uint64_t *collisions = malloc(COLLISIONS * sizeof(uint64_t));
  assert(collisions);

  for (uint64_t i = 1; i < COLLISIONS; i++) 
    collisions[i] = find_hash_collision(data[i]);
  printf("✔\n");

  printf("Adding collisions: ");
  for (uint64_t i = 1; i < COLLISIONS; i++)
    hashtable_set(data[i], (void *)i); 
  printf("✔\n");

  printf("Removing collisions tail first: ");
  for (uint64_t i = 1; i < COLLISIONS << 2; i++) {
    hashtable_delete(collisions[i]);
    hashtable_delete(data[i]);
  }
  printf("✔\n");

  printf("Removing collisions head first: ");
  for (uint64_t i = COLLISIONS << 2; i < COLLISIONS; i++) {
    hashtable_delete(data[i]);
    hashtable_delete(collisions[i]);
  }
  printf("✔\n");

  printf("Destroying hashtable: ");
  hashtable_destroy();
  printf("✔\n");

  free(collisions);
  free(data);
  printf("\n✔ Tests completed successfully ✔\n");
}


#endif
