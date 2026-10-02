/*
 * list_test.c -- tests for the list module
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// included so 'front' is accessible
#include "list.c"

#define TESTYEAR 2030

static int failures = 0;

/*
 * allocate a car with the given plate and default year/price,
 * returns NULL if malloc fails
 */
static car_t *make_car(char *platep) {
    car_t *cp = malloc(sizeof(car_t));

    if (cp != NULL) {
        cp->next = NULL;
        // copy size-1 bytes so '\0' has space
        strncpy(cp->plate, platep, MAXREG - 1);
        cp->plate[MAXREG - 1] = '\0';
        cp->price = 1000.0;
        cp->year = 2020;
    }
    return cp;
}

// set the year to TESTYEAR, used for testing lapply
static void set_year(car_t *cp) {
    cp->year = TESTYEAR;
}

// count and report a failure if cond is false
static void check(bool cond, char *msg) {
    if (!cond) {
        printf("FAIL: %s\n", msg);
        failures++;
    }
}

static void test_put_null(void) {
    check(lput(NULL) == 1, "lput correctly rejects NULL");
}

static void test_put_empty(void) {
    car_t *ap = make_car("AAA");

    check(lput(ap) == 0, "lput to empty list returns 0");
    check(front == ap && ap->next == NULL, "lput to empty list sets front");

    free(ap);
    front = NULL;
}

static void test_put_nonempty(void) {
    car_t *ap = make_car("AAA");
    car_t *bp = make_car("BBB");

    front = ap;
    check(lput(bp) == 0, "lput to non-empty list returns 0");
    check(front == bp, "lput places car at front");
    check(bp->next == ap, "lput sets new car's next pointer");
    check(ap->next == NULL, "lput preserves old front's next pointer");

    free(ap);
    free(bp);
    front = NULL;
}

static void test_get_empty(void) {
    check(lget() == NULL, "lget from empty list returns NULL");
}

static void test_get_nonempty(void) {
    car_t *ap = make_car("AAA");
    car_t *bp = make_car("BBB");

    ap->next = bp;
    front = ap;
    check(lget() == ap, "lget returns front car");
    check(front == bp, "lget advances front");

    free(ap);
    free(bp);
    front = NULL;
}

static void test_apply_empty(void) {
    lapply(set_year);
    check(front == NULL,
          "lapply on empty list leaves it empty (and doesn't crash)");
}

static void test_apply_nonempty(void) {
    car_t *ap = make_car("AAA");
    car_t *bp = make_car("BBB");

    ap->next = bp;
    front = ap;
    // make_car set the year to 2020, expect lapply to update it
    lapply(set_year);
    check(ap->year == TESTYEAR && bp->year == TESTYEAR,
          "lapply works on all cars");

    free(ap);
    free(bp);
    front = NULL;
}

static void test_remove_empty(void) {
    check(lremove("AAA") == NULL, "lremove from empty list returns NULL");
}

static void test_remove_beginning(void) {
    car_t *ap = make_car("AAA");
    car_t *bp = make_car("BBB");
    car_t *cp = make_car("CCC");

    ap->next = bp;
    bp->next = cp;
    front = ap;
    check(lremove("AAA") == ap, "lremove at beginning returns car");
    check(front == bp && bp->next == cp, "lremove at beginning relinks list");

    free(ap);
    free(bp);
    free(cp);
    front = NULL;
}

static void test_remove_middle(void) {
    car_t *ap = make_car("AAA");
    car_t *bp = make_car("BBB");
    car_t *cp = make_car("CCC");

    ap->next = bp;
    bp->next = cp;
    front = ap;
    check(lremove("BBB") == bp, "lremove in middle returns car");
    check(front == ap && ap->next == cp, "lremove in middle relinks list");

    free(ap);
    free(bp);
    free(cp);
    front = NULL;
}

static void test_remove_end(void) {
    car_t *ap = make_car("AAA");
    car_t *bp = make_car("BBB");
    car_t *cp = make_car("CCC");

    ap->next = bp;
    bp->next = cp;
    front = ap;
    check(lremove("CCC") == cp, "lremove at end returns car");
    check(front == ap && ap->next == bp && bp->next == NULL,
          "lremove at end relinks list");

    free(ap);
    free(bp);
    free(cp);
    front = NULL;
}

static void test_remove_null(void) {
    check(lremove(NULL) == NULL, "lremove returns NULL if platep is NULL");
}

int main(void) {
    test_put_null();
    test_put_empty();
    test_put_nonempty();
    test_get_empty();
    test_get_nonempty();
    test_apply_empty();
    test_apply_nonempty();
    test_remove_empty();
    test_remove_beginning();
    test_remove_middle();
    test_remove_end();
    test_remove_null();

    if (failures > 0) {
        printf("%d checks failed\n", failures);
        exit(EXIT_FAILURE);
    }
    printf("all tests passed\n");
    exit(EXIT_SUCCESS);
}
