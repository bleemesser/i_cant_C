/*
 * queue_test.c --- tests for queue module
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// needed to access head/tail
#include "queue.c"

static int failures = 0;

// count and report a failure if cond is false
static void check(bool cond, char *msg) {
    if (!cond) {
        printf("FAIL: %s\n", msg);
        failures++;
    }
}

// multiply element by 2 as an int, for testing qapply
static void times_two(void* elementp) {
    *(int*)elementp *= 2;
}

static void test_open_and_close_empty(void) {
    queue_t *qp = qopen();
    queue_impl_t *qip = qp;
    check(qp != NULL, "qopen returns a queue");
    check(qip->head == NULL, "queue head initializedd empty");
    check(qip->tail == NULL, "queue tail initialized empty");

    qclose(qp);  // expect no valgrind error
}

static void test_close_null(void) {
    qclose(NULL);  // expect not to crash
}

static void test_close_nonempty(void) {
    int *yearp = malloc(sizeof(int));
    queue_t *qp = qopen();
    queue_impl_t *qip = qp;

    *yearp = 2020;
    check(qput(qp, yearp) == 0, "qput accepts a valid element");
    check(qip->head->elementp == yearp, "element placed at head");
    check(qip->tail->elementp == yearp, "tail pointed at head at size 1");

    qclose(qp);  // expect no valgrind error
}

static void test_put_null_qp(void) {
    int *yearp = malloc(sizeof(int));
    *yearp = 2020;

    check(qput(NULL, yearp) == 1, "qput rejects a NULL qp");
}

static void test_put_null_element(void) {
    queue_t *qp = qopen();

    check(qp != NULL, "qopen returns a queue");

    check(qput(qp, NULL) == 1, "qput rejects a NULL elementp");

    qclose(qp);
}

static void test_put_valid(void) {
    queue_t *qp = qopen();
    queue_impl_t *qip = qp;
    check(qp != NULL, "qopen returns a queue");

    int *e1 = malloc(sizeof(int));
    int *e2 = malloc(sizeof(int));
    *e1 = 2020;
    *e2 = 2021;

    check(qput(qp, e1) == 0, "put adds first element");
    check(qput(qp, e2) == 0, "put adds second element");

    check(qip->head->elementp == e1, "elem 1 placed at head");
    check(qip->head->next == qip->tail, "head points to tail");
    check(qip->tail->elementp == e2, "elem 2 placed at tail");
    check(qip->tail->next == NULL, "tail points to NULL");

    qclose(qp);
}

static void test_get_null(void) {
    check(qget(NULL) == NULL, "get on NULL returns NULL");
}

static void test_get_empty(void) {
    queue_t *qp = qopen();
    check(qp != NULL, "qopen returns a queue");

    check(qget(qp) == NULL, "get on empty queue returns NULL");

    qclose(qp);
}

static void test_get_valid(void) {
    queue_t *qp = qopen();
    queue_impl_t *qip = qp;
    check(qp != NULL, "qopen returns a queue");

    int *e1 = malloc(sizeof(int));
    int *e2 = malloc(sizeof(int));
    *e1 = 2020;
    *e2 = 2021;

    check(qput(qp, e1) == 0, "put adds first element");
    check(qput(qp, e2) == 0, "put adds second element");

    check(qget(qp) == e1, "item added first is returned first");
    check(qget(qp) == e2, "second item returned second");

    check(qip->head == NULL, "get properly clears elements");
    check(qip->tail == NULL, "get fixes tail");

    qclose(qp);
}

static void test_apply_null_qp(void) {
    qapply(NULL, times_two); // expect not to crash ¯\_(ツ)_/¯
}

static void test_apply_null_fn(void) {
    queue_t* qp = qopen();
    check(qp != NULL, "qopen returns a queue");

    qapply(qp, NULL); // don't crash pls
}

static void test_apply_valid(void) {
    queue_t *qp = qopen();
    check(qp != NULL, "qopen returns a queue");

    int *e1 = malloc(sizeof(int));
    int *e2 = malloc(sizeof(int));
    *e1 = 2020;
    *e2 = 2021;

    check(qput(qp, e1) == 0, "put adds first element");
    check(qput(qp, e2) == 0, "put adds second element");

    qapply(qp, times_two);

    check(qget(qp) == e1 && *e1 == 4040, "first element is updated in-place");
    check(qget(qp) == e2 && *e2 == 4042, "second element is updated in-place");

    qclose(qp);
}

static void test_search_null_qp(void);

static void test_search_null_fn(void);

static void test_search_front(void);

static void test_search_back(void);

static void test_search_middle(void);

static void test_remove_null_qp(void);

static void test_remove_null_fn(void);

static void test_remove_front(void);

static void test_remove_back(void);

static void test_remove_middle(void);

static void test_concat_null_q1(void);

static void test_concat_null_q2(void);

static void test_concat_empty_q1(void);

static void test_concat_empty_q2(void);

static void test_concat_nonempty(void);

int main(void) {
    test_open_and_close_empty();
    test_close_null();
    test_close_nonempty();
    test_put_null_qp();
    test_put_null_element();
    test_put_valid();
    test_get_null();
    test_get_empty();
    test_get_valid();
    test_apply_null_qp();
    test_apply_null_fn();
    test_apply_valid();

    if (failures > 0) {
        printf("%d checks failed\n", failures);
        exit(EXIT_FAILURE);
    }
    printf("all tests passed\n");
    exit(EXIT_SUCCESS);
}
