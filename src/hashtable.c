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
#include "hashtable.h"

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

const value EMPTY_SLOT = NULL;
static struct value_s tombstone;
const value DEAD_SLOT = &tombstone;


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

static inline int uint128_equal(const uint128_t *a, const uint128_t *b) {
  //Force 128 comparison
  return a->w == b->w;
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
void hashtable_set(value new) {
  assert(new != EMPTY_SLOT &&
	 new != DEAD_SLOT);
  
  hash_t index;
  index = new->hash % HASHTABLE_SIZE;

  hashtable_entry entry;
  entry = hashtable + index;

  
  //Lock 
  lock_lock(&entry->lock);
  
  // Probe
  while(entry->value != EMPTY_SLOT &&
	entry->value != DEAD_SLOT &&
	!uint128_equal(&new->id, &entry->value->id)) {

    lock_unlock(&entry->lock);
    index = (index + 1) % HASHTABLE_SIZE;
    entry = hashtable + index;
    
    lock_lock(&entry->lock);
  }

  entry->value= new;
  
  // all done 
  lock_unlock(&entry->lock);
  return;
}

void hashtable_delete(value delete) {
  assert(delete != EMPTY_SLOT &&
	 delete != DEAD_SLOT);
  
  hash_t index;
  index = delete->hash % HASHTABLE_SIZE;

  hashtable_entry entry;
  entry = hashtable + index;

  
  //Lock 
  lock_lock(&entry->lock);
  
  // Probe
  while(entry->value != EMPTY_SLOT &&
	(entry->value == DEAD_SLOT || !uint128_equal(&delete->id, &entry->value->id))) {
    lock_unlock(&entry->lock);
    index = (index + 1) % HASHTABLE_SIZE;
    entry = hashtable + index;
    
    lock_lock(&entry->lock);
  }

  if(entry->value != EMPTY_SLOT)
    entry->value = DEAD_SLOT;
  
  lock_unlock(&entry->lock);
  
  return;
}

value hashtable_get(value find) {
  assert(find != EMPTY_SLOT &&
	 find != DEAD_SLOT);
  
  hash_t index;
  index = find->hash % HASHTABLE_SIZE;

  hashtable_entry entry;
  entry = hashtable + index;

  
  //Lock 
  lock_lock(&entry->lock);
  
  // Probe
  while(entry->value != EMPTY_SLOT &&
 	(entry->value == DEAD_SLOT || !uint128_equal(&find->id, &entry->value->id))) {

    lock_unlock(&entry->lock);
    index = (index + 1) % HASHTABLE_SIZE;
    entry = hashtable + index;
    
    lock_lock(&entry->lock);
  }

  register value retval = entry->value;
  
  lock_unlock(&entry->lock);

  //Will be NULL for EMPTY_SLOT
  //TODO: declare properly in include file
  return retval;
}

 
#ifdef UNIT_TEST
  
 
#include <unistd.h>
#include <fcntl.h>


#define COUNT 10000
static void print_uint128(uint128_t id) {
  printf("0x%016llx%016llx", (unsigned long long)id.h.msb, (unsigned long long)id.h.lsb);
}

int main() {
  printf("Test: reading %d uint64_t from /dev/random: ", COUNT);

  ssize_t bytes_needed = COUNT * sizeof(struct value_s);
  value data = malloc(bytes_needed);
  assert(data);

  int fd = open("/dev/random", O_RDONLY);
  if (fd < 0) {
    perror("open /dev/random");
    exit(EXIT_FAILURE);
  }

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
  for (uint64_t i = 0; i < COUNT; ++i) {
    hashtable_set(&data[i]); 
  }
  printf("✔\n");

  printf("Verifying entries: ");
  for (uint64_t i = 0; i < COUNT; ++i) {
    value found = hashtable_get(&data[i]);
    if (found == EMPTY_SLOT || found == DEAD_SLOT) {
      fprintf(stderr, "❌ Verification failed: entry missing for key ");
      print_uint128(data[i].id);
      fprintf(stderr, "\n");
      exit(EXIT_FAILURE);
    }
    if (!uint128_equal(&found->id, &data[i].id)) {
      fprintf(stderr, "❌ Verification failed: wrong key found for ");
      print_uint128(data[i].id);
      fprintf(stderr, "\n");
      exit(EXIT_FAILURE);
    }
  }
  printf("✔\n");

  printf("Deleting entries: ");
  for (int i = 0; i < COUNT; ++i) {
    hashtable_delete(&data[i]);
  }
  printf("✔\n");

  printf("Verifying deletions: ");
  for (uint64_t i = 0; i < COUNT; ++i) {
    value found = hashtable_get(&data[i]);
    if (found != EMPTY_SLOT && found != DEAD_SLOT) {
      fprintf(stderr, "❌ Deletion verification (get) failed for key ");
      print_uint128(data[i].id);
      fprintf(stderr, "\n");
      exit(EXIT_FAILURE);
    }
  }
  printf("✔\n");

  printf("Verifying deletions (memory): ");
  for (uint64_t i = 0; i < HASHTABLE_SIZE; ++i)
    if (hashtable[i].value &&
	hashtable[i].value != DEAD_SLOT) {
      fprintf(stderr, "❌ Deletion verification (mem)failed for key ");
      print_uint128(hashtable[i].value->id);
      fprintf(stderr, "\n");
      exit(EXIT_FAILURE);
    }
  printf("✔\n");

  printf("Creating collisions: ");
  for (uint64_t i = 0; i + 1 < COUNT; i += 2) {
    data[i].hash = data[i + 1].hash;
  }
  printf("✔\n");

  printf("Adding collisions: ");
  for (uint64_t i = 0; i < COUNT; i++) {
    hashtable_set(&data[i]);
  }
  printf("✔\n");

  printf("Removing collisions tail first: ");
  for (uint64_t i = 0; i + 1 < COUNT >> 1; i++) {
    hashtable_delete(&data[i+1]);
    hashtable_delete(&data[i]);
  }
  printf("✔\n");

  printf("Readding collisions: ");
  for (uint64_t i = 0; i + 1 < COUNT >> 1; i++) {
    hashtable_set(&data[i]);
  }
  printf("✔\n");

  printf("Removing collisions head first: ");
  for (uint64_t i = 0; i < COUNT; i++) {
    hashtable_delete(&data[i]);
  }
  printf("✔\n");

  printf("Verifying deletions (get): ");
  for (uint64_t i = 0; i < COUNT; ++i) {
    value found = hashtable_get(&data[i]);
    if (found != EMPTY_SLOT && found != DEAD_SLOT) {
      fprintf(stderr, "❌ Deletion verification (get) failed for key ");
      print_uint128(data[i].id);
      fprintf(stderr, "\n");
      exit(EXIT_FAILURE);
    }
  }
  printf("✔\n");

    printf("Verifying deletions (memory): ");
  for (uint64_t i = 0; i < HASHTABLE_SIZE; ++i)
    if (hashtable[i].value &&
	hashtable[i].value != DEAD_SLOT) {
      fprintf(stderr, "❌ Deletion verification (mem)failed for key ");
      print_uint128(hashtable[i].value->id);
      fprintf(stderr, "\n");
      exit(EXIT_FAILURE);
    }
  printf("✔\n");



  printf("Redeleting non-existent entries: ");
  for (uint64_t i = 0; i < COUNT; ++i) {
    hashtable_delete(&data[i]);
  }
  printf("✔\n");

  printf("Destroying hashtable: ");
  hashtable_destroy();
  printf("✔\n");

  free(data);
  printf("\n✔ Tests completed successfully ✔\n");
}


#endif
