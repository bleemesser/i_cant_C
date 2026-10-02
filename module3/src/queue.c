/*
 * queue.c -- implementation of the functions declared in queue.h
 *
 * The FIFO queue is a singly linked list of nodes combined with
 * a saved pointer to the head and tail.
 */

#include <stdint.h>
#include <stdlib.h>

#include "queue.h"

typedef struct node {
    struct node *next;
    void *datap;
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

        free(np->datap);
        free(np);

        np = nextp;
    }

    free(qip);
}

/*
 * Put an element at the end of the queue (enqueue).
 *
 * `datap` may be a pointer to any type, but that pointer
 * must not be NULL.
 *
 * Will return 0 if successful.
 * Will return 1 if an invalid (null) argument is
 * passed or malloc fails to create the entry
 */
int32_t qput(queue_t *qp, void *datap) {
    queue_impl_t *qip = qp;

    if (qip == NULL || datap == NULL) {
        return 1;
    }

    node_t *np = malloc(sizeof(node_t));
    if (np == NULL) {
        return 1;
    }

    // since np is placed at the TAIL, it has no `next` yet
    np->next = NULL;
    np->datap = datap;

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

    node_t *np = qip->head;
    void *datap = np->datap;

    qip->head = np->next;

    if (qip->head == NULL) {
        qip->tail = NULL;
    }

    free(np);

    return datap;
}

