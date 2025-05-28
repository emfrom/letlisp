#include <assert.h>
#include <pthread.h>
#include <search.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nmmintrin.h> //TODO: Do this properly

#include "memory.h"

//TODO: BUG: Check all thread function return values

/**
 * Hashtable
 *
 * Quick hack, but might be fast :)
 */

typedef struct hashtable_entry_s *hashtable_entry;
struct hashtable_entry_s {
  uint64_t id;
  void *value;
  hashtable_entry next;
};

#define HASHMASK 0x3FFFFF
#define HASHTABLE_SIZE (HASHMASK + 1)

// One table to rule them all
hashtable_entry hashtable = NULL;
pthread_rwlock_t *locks = NULL;


// Use intel builtin
// TODO: Test that these give the same value
#if 1
#define hashtable_hash(id) (_mm_crc32_u64(0, (id)) & HASHMASK)
#else

//Fallback (GPT says it does the same)
#define hashtable_hash(id) (crc32c_u64(0, (id)) & HASHMASK)

uint32_t crc32c_u64(uint32_t crc, uint64_t data) {
  static const uint32_t table[256] = {
      0x00000000, 0xF26B8303, 0xE13B70F7, 0x1350F3F4, 0xC79A971F, 0x35F1141C,
      0x26A1E7E8, 0xD4CA64EB, 0x8AD958CF, 0x78B2DBCC, 0x6BE22838, 0x9989AB3B,
      0x4D43CFD0, 0xBF284CD3, 0xAC78BF27, 0x5E133C24, 0x105EC76F, 0xE235446C,
      0xF165B798, 0x030E349B, 0xD7C45070, 0x25AFD373, 0x36FF2087, 0xC494A384,
      0x9A879FA0, 0x68EC1CA3, 0x7BBCEF57, 0x89D76C54, 0x5D1D08BF, 0xAF768BBC,
      0xBC267848, 0x4E4DFB4B, 0x20BD8EDE, 0xD2D60DDD, 0xC186FE29, 0x33ED7D2A,
      0xE72719C1, 0x154C9AC2, 0x061C6936, 0xF477EA35, 0xAA64D611, 0x580F5512,
      0x4B5FA6E6, 0xB93425E5, 0x6DFE410E, 0x9F95C20D, 0x8CC531F9, 0x7EAEB2FA,
      0x30E349B1, 0xC288CAB2, 0xD1D83946, 0x23B3BA45, 0xF779DEAE, 0x05125DAD,
      0x1642ae59, 0xe4292d5a, 0xBA3A117E, 0x4851927D, 0x5B016189, 0xA96AE28A,
      0x7DA08661, 0x8FCB0562, 0x9C9BF696, 0x6EF07595, 0x417B1DBC, 0xB3109EBF,
      0xA0406D4B, 0x522BEE48, 0x86E18AA3, 0x748A09A0, 0x67DAFA54, 0x95B17957,
      0xCBA24573, 0x39C9C670, 0x2A993584, 0xD8F2B687, 0x0C38D26C, 0xFE53516F,
      0xED03A29B, 0x1F682198, 0x5125DAD3, 0xA34E59D0, 0xB01EAA24, 0x42752927,
      0x96BF4DCC, 0x64D4CECF, 0x77843D3B, 0x85EFBE38, 0xDBFC821C, 0x2997011F,
      0x3AC7F2EB, 0xC8AC71E8, 0x1C661503, 0xEE0D9600, 0xFD5D65F4, 0x0F36E6F7,
      0x61C69362, 0x93AD1061, 0x80FDE395, 0x72966096, 0xA65C047D, 0x5437877E,
      0x4767748A, 0xB50CF789, 0xEB1FCBAD, 0x197448AE, 0x0A24BB5A, 0xF84F3859,
      0x2C855CB2, 0xDEEEDFB1, 0xCDBE2C45, 0x3FD5AF46, 0x7198540D, 0x83F3D70E,
      0x90A324FA, 0x62C8A7F9, 0xB602C312, 0x44694011, 0x5739B3E5, 0xA55230E6,
      0xFB410CC2, 0x092A8FC1, 0x1A7A7C35, 0xE811FF36, 0x3CDB9BDD, 0xCEB018DE,
      0xDDE0EB2A, 0x2F8B6829, 0x82F63B78, 0x709DB87B, 0x63CD4B8F, 0x91A6C88C,
      0x456CAC67, 0xB7072F64, 0xA457DC90, 0x563C5F93, 0x082F63B7, 0xFA44E0B4,
      0xE9141340, 0x1B7F9043, 0xCFB5F4A8, 0x3DDE77AB, 0x2E8E845F, 0xDCE5075C,
      0x92A8FC17, 0x60C37F14, 0x73938CE0, 0x81F80FE3, 0x55326B08, 0xA759E80B,
      0xB4091BFF, 0x466298FC, 0x1871A4D8, 0xEA1A27DB, 0xF94AD42F, 0x0B21572C,
      0xDFEB33C7, 0x2D80B0C4, 0x3ED04330, 0xCCBBC033, 0xA24BB5A6, 0x502036A5,
      0x4370C551, 0xB11B4652, 0x65D122B9, 0x97BAA1BA, 0x84EA524E, 0x7681D14D,
      0x2892ED69, 0xDAF96E6A, 0xC9A99D9E, 0x3BC21E9D, 0xEF087A76, 0x1D63F975,
      0x0E330A81, 0xFC588982, 0xB21572C9, 0x407EF1CA, 0x532E023E, 0xA145813D,
      0x758FE5D6, 0x87E466D5, 0x94B49521, 0x66DF1622, 0x38CC2A06, 0xCAA7A905,
      0xD9F75AF1, 0x2B9CD9F2, 0xFF56BD19, 0x0D3D3E1A, 0x1E6DCDEE, 0xEC064EED,
      0xC38D26C4, 0x31E6A5C7, 0x22B65633, 0xD0DDD530, 0x0417B1DB, 0xF67C32D8,
      0xE52CC12C, 0x1747422F, 0x49547E0B, 0xBB3FFD08, 0xA86F0EFC, 0x5A048DFF,
      0x8ECEE914, 0x7CA56A17, 0x6FF599E3, 0x9D9E1AE0, 0xD3D3E1AB, 0x21B862A8,
      0x32E8915C, 0xC083125F, 0x144976B4, 0xE622F5B7, 0xF5720643, 0x07198540,
      0x590AB964, 0xAB613A67, 0xB831C993, 0x4A5A4A90, 0x9E902E7B, 0x6CFBAD78,
      0x7FAB5E8C, 0x8DC0DD8F, 0xE330A81A, 0x115B2B19, 0x020BD8ED, 0xF0605BEE,
      0x24AA3F05, 0xD6C1BC06, 0xC5914FF2, 0x37FACCF1, 0x69E9F0D5, 0x9B8273D6,
      0x88D28022, 0x7AB90321, 0xAE7367CA, 0x5C18E4C9, 0x4F48173D, 0xBD23943E,
      0xF36E6F75, 0x0105EC76, 0x12551F82, 0xE03E9C81, 0x34F4F86A, 0xC69F7B69,
      0xD5CF889D, 0x27A40B9E, 0x79B737BA, 0x8BDCB4B9, 0x988C474D, 0x6AE7C44E,
      0xBE2DA0A5, 0x4C4623A6, 0x5F16D052, 0xAD7D5351};

  // byte-by-byte (LSB first)
  for (int i = 0; i < 8; i++) {
    uint8_t byte = data & 0xFF;
    crc = table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    data >>= 8;
  }

  return crc;
}
#endif

void hashtable_init() {
  assert(hashtable == NULL &&
	 locks == NULL);
  
  //Storage
  hashtable = gcx_malloc(HASHTABLE_SIZE * sizeof(struct hashtable_entry_s));
  memset(hashtable, 0, HASHTABLE_SIZE * sizeof(struct hashtable_entry_s));

  //Locks
  locks = gcx_malloc(HASHTABLE_SIZE * sizeof(pthread_rwlock_t));
  memset(locks, 0, HASHTABLE_SIZE * sizeof(pthread_rwlock_t));
  for (uint32_t i = 0; i < HASHTABLE_SIZE; i++)
    if (0 != pthread_rwlock_init(locks + i, NULL)) {
      fprintf(stderr, "Fatal: failed to initialise mutexes\n");
      exit(EXIT_FAILURE);
    }

  //Sanity
  assert(hashtable[0].id == 0 &&
	 hashtable[0].value == NULL &&
	 hashtable[0].next == NULL);
}


void hashtable_destroy() {
  for (uint32_t i = 0; i < HASHTABLE_SIZE; i++) {
    pthread_rwlock_destroy(&locks[i]);

    hashtable_entry entry = hashtable[i].next;
    while (entry) {
      hashtable_entry next = entry->next;
      gcx_free(entry);
      entry = next;
    }
  }

  gcx_free(locks);
  gcx_free(hashtable);

  locks = NULL;
  hashtable = NULL;
}

// Use prepend logic on collisions
void hashtable_set(uint64_t id, void *value) {
  uint32_t hash;
  hash = hashtable_hash(id);

  assert(id); //TODO: Replace with better check, or use something else as 0 (dead,free,single,list?)
  
  pthread_rwlock_wrlock(locks + hash);

  hashtable_entry entry;
  entry= &hashtable[hash];

  // Empty slot
  if (!entry->id)
    entry->id = id;

  // Check list
  for(; entry != NULL; entry = entry->next)
    if (entry->id == id) {
      // assign
      entry->value = value;

      // Release lock and exit
      pthread_rwlock_unlock(locks + hash);
      return;
    }

  // Prepend logic!
  entry = &hashtable[hash];
  
  hashtable_entry old;
  old = gcx_malloc(sizeof(struct hashtable_entry_s));

  old->id = entry->id;
  old->value = entry->value;
  old->next = entry->next;
  
  entry->id = id;
  entry->value = value;
  entry->next = old;

  // Release lock and exit
  pthread_rwlock_unlock(locks + hash);
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
