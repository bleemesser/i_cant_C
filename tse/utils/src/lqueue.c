/*
 * lqueue.c -- implementation of the interface in lqueue.h
 */

#include <pthread.h>
#include <stdlib.h>

#include "lqueue.h"
#include "queue.h"

typedef struct lqueue_impl {
    queue_t *qp;
    pthread_mutex_t lock;
} lqueue_impl_t;

lqueue_t *lqopen(void) {
    lqueue_impl_t *lip = malloc(sizeof(lqueue_impl_t));

    if (lip == NULL) {
        return NULL;
    }

    lip->qp = qopen();

    if (lip->qp == NULL || pthread_mutex_init(&lip->lock, NULL) != 0) {
        qclose(lip->qp);
        free(lip);
        return NULL;
    }

    return lip;
}

void lqclose(lqueue_t *lqp) {
    lqueue_impl_t *lip = lqp;

    if (lip == NULL) {
        return;
    }

    qclose(lip->qp);
    pthread_mutex_destroy(&lip->lock);
    free(lip);
}

int32_t lqput(lqueue_t *lqp, void *elementp) {
    lqueue_impl_t *lip = lqp;

    if (lip == NULL) {
        return 1;
    }

    pthread_mutex_lock(&lip->lock);
    int32_t result = qput(lip->qp, elementp);
    pthread_mutex_unlock(&lip->lock);

    return result;
}

void *lqget(lqueue_t *lqp) {
    lqueue_impl_t *lip = lqp;

    if (lip == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&lip->lock);
    void *elementp = qget(lip->qp);
    pthread_mutex_unlock(&lip->lock);

    return elementp;
}

void lqapply(lqueue_t *lqp, void (*fn)(void *elementp)) {
    lqueue_impl_t *lip = lqp;

    if (lip == NULL) {
        return;
    }

    pthread_mutex_lock(&lip->lock);
    qapply(lip->qp, fn);
    pthread_mutex_unlock(&lip->lock);
}

void *lqsearch(lqueue_t *lqp,
               bool (*searchfn)(void *elementp, const void *keyp),
               const void *skeyp) {
    lqueue_impl_t *lip = lqp;

    if (lip == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&lip->lock);
    void *elementp = qsearch(lip->qp, searchfn, skeyp);
    pthread_mutex_unlock(&lip->lock);

    return elementp;
}

void *lqremove(lqueue_t *lqp,
               bool (*searchfn)(void *elementp, const void *keyp),
               const void *skeyp) {
    lqueue_impl_t *lip = lqp;

    if (lip == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&lip->lock);
    void *elementp = qremove(lip->qp, searchfn, skeyp);
    pthread_mutex_unlock(&lip->lock);

    return elementp;
}
