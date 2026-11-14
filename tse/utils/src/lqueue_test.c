
/*
 * lqueue_test.c --- tests lqueue.c
 */

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "lqueue.h"

#define NPUTS 20000

static int failures = 0;

// count and report a failure if cond is false
static void check(bool cond, char *msg) {
    if (!cond) {
        printf("FAIL: %s\n", msg);
        failures++;
    }
}

static void times_two(void *elementp) {
    *(int *)elementp *= 2;
}

static bool int_equals(void *elementp, const void *keyp) {
    return *(int *)elementp == *(const int *)keyp;
}

static void test_null_queue(void) {
    int e = 1;

    lqclose(NULL);
    lqapply(NULL, times_two);
    check(lqput(NULL, &e) != 0, "lqput rejects a NULL queue");
    check(lqget(NULL) == NULL, "lqget on a NULL queue returns NULL");
    check(lqsearch(NULL, int_equals, &e) == NULL,
          "lqsearch on a NULL queue returns NULL");
    check(lqremove(NULL, int_equals, &e) == NULL,
          "lqremove on a NULL queue returns NULL");
    check(e == 1, "nothing touched the element");
}

static void test_put_get(void) {
    lqueue_t *lqp = lqopen();
    int e1 = 1;
    int e2 = 2;

    check(lqp != NULL, "lqopen returns a queue");
    check(lqget(lqp) == NULL, "a new queue is empty");
    check(lqput(lqp, NULL) != 0, "lqput rejects a NULL element");
    check(lqput(lqp, &e1) == 0, "lqput accepts the first element");
    check(lqput(lqp, &e2) == 0, "lqput accepts the second element");
    check(lqget(lqp) == &e1, "lqget returns the first element put");
    check(lqget(lqp) == &e2, "lqget returns the second element put");
    check(lqget(lqp) == NULL, "lqget empties the queue");

    lqclose(lqp);
}

static void test_apply_search_remove(void) {
    lqueue_t *lqp = lqopen();
    int elements[] = {1, 2, 3};
    int present = 4;
    int missing = 3;

    for (int i = 0; i < 3; i++) {
        lqput(lqp, &elements[i]);
    }

    lqapply(lqp, times_two);
    check(elements[0] == 2 && elements[1] == 4 && elements[2] == 6,
          "lqapply reaches every element");
    check(lqsearch(lqp, int_equals, &present) == &elements[1],
          "lqsearch finds a matching element");
    check(lqsearch(lqp, int_equals, &missing) == NULL,
          "lqsearch returns NULL when nothing matches");
    check(lqremove(lqp, int_equals, &present) == &elements[1],
          "lqremove returns the matching element");
    check(lqremove(lqp, int_equals, &present) == NULL,
          "lqremove takes the element out");
    check(lqremove(lqp, int_equals, &missing) == NULL,
          "lqremove returns NULL when nothing matches");

    // queue is [0,2] since remove pulled index 1
    check(lqget(lqp) == &elements[0], "lqget returns the first element left");
    check(lqget(lqp) == &elements[2], "lqget returns the last element left");
    check(lqget(lqp) == NULL, "lqremove took out only the matching element");

    lqclose(lqp);
}

typedef struct worker {
    lqueue_t *lqp;
    int values[NPUTS];
    long sum;
    int misses;
} worker_t;

// Put NPUTS elements, then get NPUTS elements.
static void *put_then_get(void *argp) {
    worker_t *w = argp;

    for (int i = 0; i < NPUTS; i++) {
        w->values[i] = i + 1;
        lqput(w->lqp, &w->values[i]);
    }

    for (int i = 0; i < NPUTS; i++) {
        int *ep = lqget(w->lqp);

        if (ep == NULL) {
            w->misses++;
        } else {
            w->sum += *ep;
        }
    }

    return NULL;
}

static void test_two_threads(void) {
    lqueue_t *lqp = lqopen();
    worker_t *a = calloc(1, sizeof(worker_t));
    worker_t *b = calloc(1, sizeof(worker_t));
    pthread_t ta;
    pthread_t tb;

    a->lqp = lqp;
    b->lqp = lqp;
    pthread_create(&ta, NULL, put_then_get, a);
    pthread_create(&tb, NULL, put_then_get, b);
    pthread_join(ta, NULL);
    pthread_join(tb, NULL);

    // each thread put 1 + 2 + ... + N = N(N+1)/2, times 2
    long expected = (long)NPUTS * (NPUTS + 1);

    check(a->misses == 0 && b->misses == 0, "all gets succeeded");
    check(a->sum + b->sum == expected, "sum matches expectation");
    check(lqget(lqp) == NULL, "queue is empty at the end");

    free(a);
    free(b);
    lqclose(lqp);
}

int main(void) {
    test_null_queue();
    test_put_get();
    test_apply_search_remove();
    test_two_threads();

    if (failures > 0) {
        printf("%d checks failed\n", failures);
        exit(EXIT_FAILURE);
    }
    printf("all tests passed\n");
    exit(EXIT_SUCCESS);
}
