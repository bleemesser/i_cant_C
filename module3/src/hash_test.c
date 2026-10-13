/*
 * hash_test.c --- tests for hash module
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "hash.c"
#include "queue.c"

static int failures = 0;

// count and report a failure if cond is false
static void check(bool cond, char *msg) {
    if (!cond) {
        printf("FAIL: %s\n", msg);
        failures++;
    }
}

// multiply element by 2 as an int, for testing happly
static void times_two(void *elementp) {
    if (elementp == NULL) {
        return;
    }

    *(int *)elementp *= 2;
}

/*
 * Compare elementp and keyp, assuming elementp is an int* and keyp is a
 * string such as "2020".
 *
 * The table only hashes keyp and passes it on untouched, so an element
 * does not need to store the key.
 *
 * Returns false if either pointer is NULL or the values do not match,
 * returns true on a match.
 */
static bool find_year_match(void *elementp, const void *keyp) {
    if (elementp == NULL || keyp == NULL) {
        return false;
    }

    return *(int *)elementp == atoi((const char *)keyp);
}

static void test_open_and_close_empty(void) {
    hashtable_t *htp = hopen(4);
    hash_impl_t *htip = htp;
    check(htp != NULL, "hopen returns a table");

    check(htip->size == 4, "table keeps its size");

    for (uint32_t i = 0; i < htip->size; i++) {
        check(htip->arrayp[i] == NULL, "bucket starts empty");
    }

    hclose(htp);  // expect no valgrind error
}

static void test_open_size_zero(void) {
    check(hopen(0) == NULL, "hopen rejects a size of 0");
}

static void test_close_null(void) {
    hclose(NULL);  // expect not to crash
}

static void test_close_nonempty(void) {
    hashtable_t *htp = hopen(4);
    int *yearp = malloc(sizeof(int));
    *yearp = 2020;

    check(hput(htp, yearp, "2020", 4) == 0, "hput adds an element");

    happly(htp, free);
    hclose(htp);  // expect no valgrind error
}

static void test_put_bad_args(void) {
    hashtable_t *htp = hopen(4);
    hash_impl_t *htip = htp;
    int *yearp = malloc(sizeof(int));
    *yearp = 2020;

    check(hput(NULL, yearp, "2020", 4) == 1, "hput rejects a NULL htp");

    check(hput(htp, NULL, "2020", 4) == 1, "hput rejects a NULL elementp");

    check(hput(htp, yearp, NULL, 4) == 1, "hput rejects a NULL key");

    check(hput(htp, yearp, "2020", 0) == 1, "hput rejects a keylen of 0");

    check(hput(htp, yearp, "2020", -4) == 1, "hput rejects a negative keylen");

    check(htip->arrayp[0] == NULL && htip->arrayp[3] == NULL,
          "a rejected hput opens no bucket");

    hclose(htp);
    free(yearp);
}

static void test_put_valid(void) {
    hashtable_t *htp = hopen(4);
    hash_impl_t *htip = htp;

    int *e1 = malloc(sizeof(int));
    int *e2 = malloc(sizeof(int));
    *e1 = 2020;
    *e2 = 2021;

    check(hput(htp, e1, "2020", 4) == 0, "hput adds first element");
    check(hput(htp, e2, "2021", 4) == 0, "hput adds second element");

    // "2020" and "2021" do not collide here, `test_search_one_bucket` checks collisions
    uint32_t b1 = SuperFastHash("2020", 4, htip->size);
    uint32_t b2 = SuperFastHash("2021", 4, htip->size);

    queue_impl_t *bucket1 = htip->arrayp[b1];
    queue_impl_t *bucket2 = htip->arrayp[b2];

    check(b1 != b2, "the two keys hash to different buckets");

    check(bucket1 != NULL && bucket1->head != NULL &&
              bucket1->head->elementp == e1 && bucket1->head->next == NULL,
          "hput stores element 1 in the bucket of its key");

    check(bucket2 != NULL && bucket2->head != NULL &&
              bucket2->head->elementp == e2 && bucket2->head->next == NULL,
          "hput stores element 2 in the bucket of its key");

    happly(htp, free);
    hclose(htp);
}

static void test_search_bad_args(void) {
    hashtable_t *htp = hopen(4);
    int *e1 = malloc(sizeof(int));
    *e1 = 2020;
    check(hput(htp, e1, "2020", 4) == 0, "element added to table");

    check(hsearch(NULL, find_year_match, "2020", 4) == NULL,
          "search on a NULL htp returns NULL");

    check(hsearch(htp, NULL, "2020", 4) == NULL,
          "search returns NULL if fn is NULL");

    check(hsearch(htp, find_year_match, NULL, 4) == NULL,
          "search returns NULL if key is NULL");

    check(hsearch(htp, find_year_match, "2020", 0) == NULL,
          "search rejects a keylen of 0");

    happly(htp, free);
    hclose(htp);
}

static void test_search_missing(void) {
    hashtable_t *htp = hopen(4);
    int *e1 = malloc(sizeof(int));
    *e1 = 2020;
    check(hput(htp, e1, "2020", 4) == 0, "element added to table");

    check(hsearch(htp, find_year_match, "2999", 4) == NULL,
          "search of missing key returns NULL");

    check(hsearch(htp, find_year_match, "202", 3) == NULL,
          "search of partial key returns NULL");

    check(hsearch(htp, find_year_match, "2020", 4) == e1,
          "search of correct key finds the element");

    happly(htp, free);
    hclose(htp);
}

static void test_search_empty_table(void) {
    hashtable_t *htp = hopen(4);

    check(hsearch(htp, find_year_match, "2020", 4) == NULL,
          "search of an empty table returns NULL");

    hclose(htp);
}

static void test_search_one_bucket(void) {
    hashtable_t *htp = hopen(1);  // force collisions
    int *e1 = malloc(sizeof(int));
    int *e2 = malloc(sizeof(int));
    int *e3 = malloc(sizeof(int));
    *e1 = 2018;
    *e2 = 2019;
    *e3 = 2020;
    check(hput(htp, e1, "2018", 4) == 0, "element added to table");
    check(hput(htp, e2, "2019", 4) == 0, "element added to table");
    check(hput(htp, e3, "2020", 4) == 0, "element added to table");

    check(hsearch(htp, find_year_match, "2018", 4) == e1,
          "search finds element at front of the bucket");

    check(hsearch(htp, find_year_match, "2019", 4) == e2,
          "search finds element in middle of the bucket");

    check(hsearch(htp, find_year_match, "2020", 4) == e3,
          "search finds element at end of the bucket");

    happly(htp, free);
    hclose(htp);
}

static void test_remove_bad_args(void) {
    hashtable_t *htp = hopen(4);
    int *e1 = malloc(sizeof(int));
    *e1 = 2020;
    check(hput(htp, e1, "2020", 4) == 0, "element added to table");

    check(hremove(NULL, find_year_match, "2020", 4) == NULL,
          "remove on a NULL htp returns NULL");

    check(hremove(htp, NULL, "2020", 4) == NULL,
          "remove returns NULL if fn is NULL");

    check(hremove(htp, find_year_match, NULL, 4) == NULL,
          "remove returns NULL if key is NULL");

    check(hremove(htp, find_year_match, "2020", -4) == NULL,
          "remove rejects negative keylen");

    check(hsearch(htp, find_year_match, "2020", 4) == e1,
          "a rejected remove keeps the element");

    happly(htp, free);
    hclose(htp);
}

static void test_remove_missing(void) {
    hashtable_t *htp = hopen(4);
    int *e1 = malloc(sizeof(int));
    *e1 = 2020;
    check(hput(htp, e1, "2020", 4) == 0, "element added to table");

    check(hremove(htp, find_year_match, "2999", 4) == NULL,
          "remove on an absent key returns NULL");

    check(hsearch(htp, find_year_match, "2020", 4) == e1,
          "remove on an absent key preserves state");

    happly(htp, free);
    hclose(htp);
}

static void test_remove_found(void) {
    hashtable_t *htp = hopen(4);
    hash_impl_t *htip = htp;
    int *e1 = malloc(sizeof(int));
    int *e2 = malloc(sizeof(int));
    *e1 = 2020;
    *e2 = 2018;
    check(hput(htp, e1, "2020", 4) == 0, "element added to table");
    check(hput(htp, e2, "2018", 4) == 0, "element added to table");

    // "2020" and "2018" do not collide here
    queue_impl_t *bucket1 = htip->arrayp[SuperFastHash("2020", 4, htip->size)];
    queue_impl_t *bucket2 = htip->arrayp[SuperFastHash("2018", 4, htip->size)];

    check(bucket1 != bucket2, "the two keys do not collide");

    check(hremove(htp, find_year_match, "2020", 4) == e1,
          "remove returns the element of the key");

    check(hsearch(htp, find_year_match, "2020", 4) == NULL,
          "removed element is not found again");

    check(bucket1 != NULL && bucket1->head == NULL,
          "the emptied bucket stays open, and hclose still frees it");

    check(bucket2 != NULL && bucket2->head->elementp == e2,
          "the bucket of the other key is untouched");

    free(e1);
    happly(htp, free);
    hclose(htp);
}

static void test_remove_shared_bucket(void) {
    hashtable_t *htp = hopen(1);  // force collisions
    int *e1 = malloc(sizeof(int));
    int *e2 = malloc(sizeof(int));
    int *e3 = malloc(sizeof(int));
    *e1 = 2018;
    *e2 = 2019;
    *e3 = 2020;
    check(hput(htp, e1, "2018", 4) == 0, "element added to table");
    check(hput(htp, e2, "2019", 4) == 0, "element added to table");
    check(hput(htp, e3, "2020", 4) == 0, "element added to table");

    check(hremove(htp, find_year_match, "2019", 4) == e2,
          "remove returns one element of a bucket holding several");

    check(hsearch(htp, find_year_match, "2018", 4) == e1,
          "element before the removed one is still found");

    check(hsearch(htp, find_year_match, "2020", 4) == e3,
          "element after the removed one is still found");

    free(e2);
    happly(htp, free);
    hclose(htp);
}

static void test_apply_null_htp(void) {
    happly(NULL, times_two);  // expect not to crash
}

static void test_apply_null_fn(void) {
    hashtable_t *htp = hopen(4);
    int *e1 = malloc(sizeof(int));
    *e1 = 2020;
    check(hput(htp, e1, "2020", 4) == 0, "element added to table");

    happly(htp, NULL);  // expect not to crash

    check(hsearch(htp, find_year_match, "2020", 4) == e1,
          "happly with a NULL fn leaves the element alone");

    happly(htp, free);
    hclose(htp);
}

static void test_apply_valid(void) {
    hashtable_t *htp = hopen(4);
    int *e1 = malloc(sizeof(int));
    int *e2 = malloc(sizeof(int));
    int *e3 = malloc(sizeof(int));
    *e1 = 2018;
    *e2 = 2019;
    *e3 = 2020;
    check(hput(htp, e1, "2018", 4) == 0, "element added to table");
    check(hput(htp, e2, "2019", 4) == 0, "element added to table");
    check(hput(htp, e3, "2020", 4) == 0, "element added to table");

    happly(htp, times_two);

    check(*e1 == 4036, "first element is updated in place");

    check(*e2 == 4038, "second element is updated in place");

    check(*e3 == 4040, "third element is updated in place");

    happly(htp, free);
    hclose(htp);
}

int main(void) {
    test_open_and_close_empty();
    test_open_size_zero();
    test_close_null();
    test_close_nonempty();
    test_put_bad_args();
    test_put_valid();
    test_search_bad_args();
    test_search_missing();
    test_search_empty_table();
    test_search_one_bucket();
    test_remove_bad_args();
    test_remove_missing();
    test_remove_found();
    test_remove_shared_bucket();
    test_apply_null_htp();
    test_apply_null_fn();
    test_apply_valid();

    if (failures > 0) {
        printf("%d checks failed\n", failures);
        exit(EXIT_FAILURE);
    }
    printf("all tests passed\n");
    exit(EXIT_SUCCESS);
}
