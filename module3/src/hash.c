/*
 * hash.c -- implements a generic hash table as an indexed set of queues.
 *
 */
#include <stddef.h>
#include <stdint.h>

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
 * Stores a size and a pointer to an array of queue pointers
 */
typedef struct hash_impl {
	uint32_t size;
	queue_t **arrayp;
} hash_impl_t;


/*
 * Create empty hashset
 * All queue pointers NULL
 *
 * Returns NULL if any malloc fails
 * Returns ptr to hashtable otherwise.
 */
hashtable_t *hopen(uint32_t hsize) {
	hash_impl_t *htip = malloc(sizeof(hash_impl_t));

	if (htip != NULL) {
		 htip->size = hsize;
		 htip->arrayp = malloc(hsize * sizeof(queue_t*));

		 if (htip->arrayp == NULL) {
			 return NULL;
		 }

		 for (uint32_t i = 0; i < hsize; i++) {
			 htip->arrayp[i] = NULL;
		 }
	}

	return htip;
}

/*
 * Close all queues and then the hashtable
 *
 *`htp` becomes invalid pointer after calling
 */
void hclose(hashtable_t *htp) {
	hash_impl_t *htip = htp;

	if (htip == NULL) {
		return;
	}

	for (uint32_t i = 0; i < htip->size; i++) {
		qclose(htip->arrayp[i]);
	}

	free(htip->arrayp);
	free(htip);
}

/*
 * Put entry in hashtable given the designated key
 *
 * Returns 0 if successful
 * Returns 1 if invalid arguments
 *        or if qopen/qput failed
 *
 * User should remember what `key` was used to insert `ep`,
 * otherwise it may not be retrievable
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

	//if still NULL, then qopen() failed
	if (htip->arrayp[hash] == NULL) {
		return 1;
	}

	int32_t check = qput(htip->arrayp[hash], ep);

	return check;
}
