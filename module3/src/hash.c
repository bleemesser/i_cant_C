/*
 * hash.c -- implements a generic hash table as an indexed set of queues.
 *
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "hash.h"
#include "queue.h"

/*
 * SuperFastHash() -- produces a number between 0 and the tablesize-1.
 *
 * The following (rather complicated) code, has been taken from Paul
 * Hsieh's website under the terms of the BSD license. It's a hash
 * function used all over the place nowadays, including Google Sparse
 * Hash.
 *
 * Note: len is the length of the data in bytes
 */
#define get16bits(d) (*((const uint16_t *)(d)))

static uint32_t SuperFastHash(const char *data, int len, uint32_t tablesize) {
    uint32_t hash = len, tmp;
    int rem;

    if (len <= 0 || data == NULL) {
        return 0;
    }

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (; len > 0; len--) {
        hash += get16bits(data);
        tmp = (get16bits(data + 2) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        data += 2 * sizeof(uint16_t);
        hash += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
    case 3:
        hash += get16bits(data);
        hash ^= hash << 16;
        hash ^= data[sizeof(uint16_t)] << 18;
        hash += hash >> 11;
        break;
    case 2:
        hash += get16bits(data);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
    case 1:
        hash += *data;
        hash ^= hash << 10;
        hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;
    return hash % tablesize;
}

/*
 * hash.h hides the type of hashtable_t behind void,
 * this is the actual type it refers to
 */
typedef struct hash_impl {
    uint32_t size;
    queue_t **arrayp;
} hash_impl_t;

/*
 * Creates an emty hash table with hsize buckets.
 *
 * Buckets are NULL by default and are opened only
 * when an element is mapped to it.
 *
 * Returns NULL if hsize is 0 or if malloc fails.
 */
hashtable_t *hopen(uint32_t hsize) {
    if (hsize == 0) {
        return NULL;
    }

    hash_impl_t *htip = malloc(sizeof(hash_impl_t));

    if (htip == NULL) {
        return NULL;
    }

    htip->size = hsize;
    htip->arrayp = malloc(hsize * sizeof(queue_t *));

    if (htip->arrayp == NULL) {
        free(htip);  // free the table as well if bucket alloc fails
        return NULL;
    }

    for (uint32_t i = 0; i < hsize; i++) {
        htip->arrayp[i] = NULL;
    }

    return htip;
}

/*
 * Frees all buckets and the table itself.
 * Elements are NOT freed, those are owned by the caller.
 *
 * To free stored elements, use `happly(htp, free)`
 *
 *`htp` becomes invalid pointer after calling
 */
void hclose(hashtable_t *htp) {
    hash_impl_t *htip = htp;

    if (htip == NULL) {
        return;
    }

    for (uint32_t i = 0; i < htip->size; i++) {
        qclose(htip->arrayp[i]);  // does nothing if queue is NULL
    }

    free(htip->arrayp);
    free(htip);
}

/*
 * Place an element into the bucket that `key` hashes to.
 *
 * `ep` must not be NULL and `key` must point to `keylen` bytes that
 * identify the element. The key itself is not stored, caller must track.
 *
 * Will return 0 if successful.
 * Will return 1 if an invalid (null) argument is passed, if `keylen` is
 * not positive, or if malloc fails to open the bucket or create the entry.
 */
int32_t hput(hashtable_t *htp, void *ep, const char *key, int keylen) {
    hash_impl_t *htip = htp;

    if (htip == NULL || ep == NULL || key == NULL || keylen <= 0) {
        return 1;
    }

    uint32_t hash = SuperFastHash(key, keylen, htip->size);

    if (htip->arrayp[hash] == NULL) {
        htip->arrayp[hash] = qopen();
    }

    // if still NULL, then qopen() failed
    if (htip->arrayp[hash] == NULL) {
        return 1;
    }

    return qput(htip->arrayp[hash], ep);
}

/*
 * Apply a function to every element stored in the hashtable.
 *
 * `fn` receives an element pointer in turn and may modify the element
 * but must NOT free it.
 *
 * Does nothing if `htp` or `fn` is NULL.
 *
 * Does not return a value, so a failure within `fn` cannot be detected.
 */
void happly(hashtable_t *htp, void (*fn)(void *ep)) {
    hash_impl_t *htip = htp;

    if (htip == NULL || fn == NULL) {
        return;
    }

    for (uint32_t i = 0; i < htip->size; i++) {
        queue_t *qp = htip->arrayp[i];
        qapply(
            qp,
            fn);  // qapply can deal with NULL pointers (i.e. do nothing and return)
    }
}
