/*
 * queue.c -- implementation of the functions declared in queue.h
 *
 * The FIFO queue is a singly linked list of nodes combined with
 * a saved pointer to the head and tail.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "queue.h"

typedef struct node {
    struct node *next;
    void *elementp;
} node_t;

/*
 * queue.h hides the type of queue_t behind void,
 * this is the actual type it refers to
 */
typedef struct queue_impl {
    /* BOTH head and tail must be NULL if and only if the queue is empty!! */
    node_t *head;
    node_t *tail;
} queue_impl_t;

/*
 * Creates an empty queue.
 *
 * CAN fail if malloc fails, will return NULL in that case.
 * Otherwise returns ptr to the created queue.
 */
queue_t *qopen(void) {
    queue_impl_t *qip = malloc(sizeof(queue_impl_t));

    if (qip != NULL) {
        qip->head = NULL;
        qip->tail = NULL;
    }

    return qip;
}

/*
 * Frees all nodes, contained data, and the queue itself.
 *
 * `qp` becomes invalid ptr after calling.
 */
void qclose(queue_t *qp) {
    queue_impl_t *qip = qp;

    if (qip == NULL) {
        return;
    }

    node_t *np = qip->head;

    while (np != NULL) {
        node_t *nextp = np->next;

        free(np->elementp);
        free(np);

        np = nextp;
    }

    free(qip);
}

/*
 * Put an element at the end of the queue (enqueue).
 *
 * `elementp` may be a pointer to any type, but that pointer
 * must not be NULL.
 *
 * Will return 0 if successful.
 * Will return 1 if an invalid (null) argument is
 * passed or malloc fails to create the entry
 */
int32_t qput(queue_t *qp, void *elementp) {
    queue_impl_t *qip = qp;

    if (qip == NULL || elementp == NULL) {
        return 1;
    }

    node_t *np = malloc(sizeof(node_t));
    if (np == NULL) {
        return 1;
    }

    // since np is placed at the TAIL, it has no `next` yet
    np->next = NULL;
    np->elementp = elementp;

    if (qip->tail == NULL) {
        qip->head = np;
    } else {
        qip->tail->next = np;
    }
    qip->tail = np;

    return 0;
}

/*
 * Get the first item from the queue, removing it.
 *
 * Caller is responsible for freeing the returned data.
 *
 * Will return NULL if `qp` is NULL or if the queue is empty.
 */
void *qget(queue_t *qp) {
    queue_impl_t *qip = qp;

    if (qip == NULL) {
        return NULL;
    }

    if (qip->head == NULL) {
        return NULL;
    }

    node_t *np = qip->head;
    void *elementp = np->elementp;

    qip->head = np->next;

    if (qip->head == NULL) {
        qip->tail = NULL;
    }

    free(np);

    return elementp;
}

/*
 * Apply a function to every element of the queue from head to tail.
 *
 * `fn` receives an element pointer in turn and may modify the element
 * but must NOT free it.
 *
 * Does nothing if `qp` or `fn` is NULL.
 *
 * Does not return a value, so a failure within `fn` cannot be detected.
 */
void qapply(queue_t *qp, void (*fn)(void *elementp)) {
    queue_impl_t *qip = qp;

    if (qip == NULL || fn == NULL) {
        return;
    }

    node_t *np = qip->head;

    while (np != NULL) {
        // SAFETY: qput rejects NULL elements, so elementp
        // does not need a null check.
        fn(np->elementp);
        np = np->next;
    }
}

/*
 * Search the queue for the first element matching a key.
 *
 * `searchfn` is called with each element and `skeyp` from head to tail,
 * and must return true on a match.
 *
 * The element is NOT removed from the queue, so the caller must NOT free it.
 *
 * Will return a pointer to the first matching element,
 * or NULL if no element matches.
 *
 * Will return NULL if `qp` or `searchfn` is NULL.
 */
void *qsearch(queue_t *qp, bool (*searchfn)(void *elementp, const void *keyp),
              const void *skeyp) {
    queue_impl_t *qip = qp;

    if (qip == NULL || searchfn == NULL) {
        return NULL;
    }

    for (node_t *np = qip->head; np != NULL; np = np->next) {
        if (searchfn(np->elementp, skeyp)) {
            return np->elementp;
        }
    }

    return NULL;
}
