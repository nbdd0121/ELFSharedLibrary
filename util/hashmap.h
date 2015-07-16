#ifndef NORLIT_LIB_UTIL_HASHMAP_H
#define NORLIT_LIB_UTIL_HASHMAP_H

#include <stdbool.h>

typedef int (*comparator_t)(const void *, const void *);
typedef int (*hash_t)(const void *);
typedef struct hashmap_t hashmap_t;
typedef struct {
    void *first;
    void *second;
} pair_t;

int string_hash(const void *);
int string_comparator(const void *, const void *);
hashmap_t *hashmap_new_string(int size);
hashmap_t *hashmap_new(hash_t, comparator_t, int);
void *hashmap_put(hashmap_t *, const void *, void *);
void *hashmap_get(hashmap_t *, const void *);
void *hashmap_remove(hashmap_t *, const void *);
void hashmap_dispose(hashmap_t *);
pair_t *hashmap_iterator(hashmap_t *hm);
pair_t *hashmap_next(pair_t *it);

#endif