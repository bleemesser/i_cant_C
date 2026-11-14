#pragma once

/*
 * lqueue.h -- thread-safe wrapper for queue
 */

#include <stdbool.h>
#include <stdint.h>

typedef void lqueue_t;

// Create an empty locked queue, or return NULL on failure
lqueue_t *lqopen(void);

/*
 * Free the queue and all internal structures,
 * elements are NOT freed and must be manually handled.
 */
void lqclose(lqueue_t *lqp);

/*
 * Put an element at the end of the queue.
 *
 * Return 0 on success, 1 on invalid argument or malloc failure.
 *
 * `elementp` may be a pointer to any type (and may mix within a given queue)
 * but the pointer must not be NULL.
 */
int32_t lqput(lqueue_t *lqp, void *elementp);

/*
 * Remove and return a pointer to the first item in the queue.
 *
 * Return NULL if `qp` is NULL or if the queue is empty.
 */
void *lqget(lqueue_t *lqp);

/*
 * Apply `fn` to every element in the queue, ordered head to tail.
 *
 * `fn` receives an element pointer and may modify the element, but must
 * NOT free it.
 *
 * This function does nothing if `qp` or `fn` is NULL.
 */
void lqapply(lqueue_t *lqp, void (*fn)(void *elementp));

/*
 * Return a pointer to the first element for which `searchfn` returns true using `skeyp`.
 * If no element is selected, return NULL. The element is NOT removed from the queue.
 */
void *lqsearch(lqueue_t *lqp,
               bool (*searchfn)(void *elementp, const void *keyp),
               const void *skeyp);

/*
 * Return a pointer to the first element for which `searchfn` returns true using `skeyp`.
 * The returned element is removed from the queue. If no element is selected, return NULL.
 */
void *lqremove(lqueue_t *lqp,
               bool (*searchfn)(void *elementp, const void *keyp),
               const void *skeyp);
