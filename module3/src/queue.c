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