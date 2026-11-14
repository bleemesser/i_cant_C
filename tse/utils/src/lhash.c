/*
 * lhash.c -- implements a locked hashtable using generic hash table and mutex
 *
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>

#include "hash.h"
#include "lhash.h"

typedef struct lhash_impl {
	hashtable_t *htp;
	pthread_mutex_t lock;
} lhash_impl_t;
	

/*
 * Creates an empty hash table with hsize buckets.
 *
 * Buckets are NULL by default and are opened only
 * when an element is mapped to it.
 *
 * Also creates a mutex
 *
 * Returns NULL if hsize is 0 or if malloc fails.
 */
lhashtable_t *lhopen(uint32_t hsize) {

	if (hsize == 0) {
		return NULL;
	}

	lhash_impl_t *lhtip = malloc(sizeof(lhash_impl_t));

	if (lhtip == NULL) {
		return NULL;
	}
	
	lhtip->htp = hopen(hsize);

	if (lhtip->htp == NULL) {
		free(lhtip);
		return NULL;
	}

	if (pthread_mutex_init(&lhtip->lock, NULL) != 0) {
		hclose(lhtip->htp);
		free(lhtip);
		return NULL;
	}

	return lhtip;
}

/*
 * Closes the lhashtable
 *
 * Elements are NOT freed, those are owned by the caller.
 *
 * To free stored elements, use `lhapply(lhtp, free)`
 *
 *`lhtp` becomes invalid pointer after calling
 */
void lhclose(lhashtable_t *lhtp) {

	lhash_impl_t *lhtip = lhtp;

	if (lhtip == NULL) {
		return;
	}

	pthread_mutex_lock(&lhtip->lock);

	hclose(lhtip->htp);

	pthread_mutex_unlock(&lhtip->lock);

	pthread_mutex_destroy(&lhtip->lock);
	free(lhtip);
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
int32_t lhput(lhashtable_t *lhtp, void *ep, const char *key, int keylen) {

	lhash_impl_t *lhtip = lhtp;

	if (lhtip == NULL) {
		return 1;
	}

	pthread_mutex_lock(&lhtip->lock);

	int32_t status = hput(lhtip->htp, ep, key, keylen);
	
	pthread_mutex_unlock(&lhtip->lock);

	return status;
}

/*
 * Apply a function to every element stored in the hashtable.
 *
 * `fn` receives an element pointer in turn and may modify the element
 * but must NOT free it.
 *
 * Does nothing if `lhtp`, `lhtip->htp`, or `fn` is NULL.
 *
 * Does not return a value, so a failure within `fn` cannot be detected.
 */
void lhapply(lhashtable_t *lhtp, void (*fn)(void* ep)) {

	lhash_impl_t *lhtip = lhtp;

	if (lhtip == NULL) {
		return;
	}

	pthread_mutex_lock(&lhtip->lock);

	happly(lhtip->htp, fn);
	
	pthread_mutex_unlock(&lhtip->lock);

}

/*
 * Apply a function to every element stored in the hashtable,
 * passing `arg` unchanged to each call.
 *
 * Same rules as lhapply.
 */
void lhapplyargs(lhashtable_t *lhtp, void (*fn)(void *ep, void *arg), void *arg) {

	lhash_impl_t *lhtip = lhtp;

	if (lhtip == NULL) {
		return;
	}

	pthread_mutex_lock(&lhtip->lock);

	happlyargs(lhtip->htp, fn, arg);
	
	pthread_mutex_unlock(&lhtip->lock);
		
}

/*
 * Search the bucket that `key` hashes to for the element matching `key`.
 *
 * `searchfn` is called with each element of that bucket and `key`,
 * and must return true on a match. The table does not interpret `key`,
 * it only hashes it and passes it to `searchfn` unchanged, so how an
 * element is matched to a key is up to `searchfn`.
 *
 * The element is NOT removed from the table, so the caller must NOT free it.
 *
 * Will return a pointer to the first matching element, or NULL if no element matches.
 * Will also return NULL if a null pointer is passed or `keylen` is not positive.
 */
void *lhsearch(lhashtable_t *lhtp,
							 bool (*searchfn)(void* elementp, const void* searchkeyp),
							 const char *key,
							 int32_t keylen) {

	lhash_impl_t *lhtip = lhtp;

	if (lhtip == NULL) {
		return NULL;
	}

	pthread_mutex_lock(&lhtip->lock);

	void *matchp = hsearch(lhtip->htp, searchfn, key, keylen);
	
	pthread_mutex_unlock(&lhtip->lock);

	return matchp;
}

/*
 * Search the bucket that `key` hashes to for the element matching `key`,
 * REMOVE it from the table, and return a pointer to it.
 *
 * `searchfn` is called with each element of that bucket and `key`,
 * and must return true on a match.
 *
 * The caller owns the returned element and must free it.
 * The queue node is freed.
 *
 * Will return NULL if `lhtp`, `lhtip->htp`, `searchfn` or `key` is NULL,
 * if `keylen` is not positive, or if no element matches.
 */
void *lhremove(lhashtable_t *lhtp,
							 bool (*searchfn)(void* elementp, const void* searchkeyp),
							 const char *key,
							 int32_t keylen) {

	lhash_impl_t *lhtip = lhtp;

	if (lhtip == NULL) {
		return NULL;
	}

	pthread_mutex_lock(&lhtip->lock);

	void *ep = hremove(lhtip->htp, searchfn, key, keylen);
	
	pthread_mutex_unlock(&lhtip->lock);

	return ep;
}
