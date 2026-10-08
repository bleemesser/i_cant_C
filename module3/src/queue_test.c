/* 
 * queue_test.c --- tests for queue module
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

static int failures = 0;

// count and report a failure if cond is false
static void check(bool cond, char *msg) {
    if (!cond) {
        printf("FAIL: %s\n", msg);
        failures++;
    }
}

static void test_open(void);

static void test_close_null(void);

static void test_close_something(void);

static void test_put_null_qip(void);

static void test_put_null_element(void);

static void test_put_valid(void);

static void test_get_null(void);

static void test_get_empty(void);

static void test_get_valid(void);

static void test_apply_null_qp(void);

static void test_apply_null_fn(void);

static void test_apply_valid(void);

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
