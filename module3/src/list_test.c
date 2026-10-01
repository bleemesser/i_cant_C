#include <bsd/string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"

static int failures = 0;

static car_t *make_car(const char *plate, double price, int year);
static car_t *must_make_car(const char *plate);
static void put_three(car_t *cars[3]);
static void check(bool cond, const char *msg);
static void check_remove(car_t *expected, const char *msg);

static void test_put_null(void) {
    check(lput(NULL) != 0, "lput(NULL) returns nonzero");
}

static void test_put_empty(void) {
    car_t *a = must_make_car("AAA");

    check(lput(a) == 0, "lput to empty list returns 0");
    check(a->next == NULL, "next == NULL is set correctly");
    check_remove(a, "lremove finds the only car");
}

static void test_put_nonempty(void) {
    car_t *a = must_make_car("AAA");
    car_t* b = must_make_car("BBB");

    check(lput(a) == 0, "first lput returns 0");
    check(lput(b) == 0, "lput to non-empty list returns 0");
    check(b->next == a, "lput places car at front");

    check_remove(a, "lremove finds first car put");
    check_remove(b, "lremove finds second car put");
}

static void test_get_empty(void) {
	  check(lget() == NULL, "get on empty list returns NULL");
}

static void test_get_nonempty(void) {
	  car_t *a = must_make_car("AAA");
	  car_t *b = must_make_car("BBB");

	  check(lput(a) == 0, "first lput returns 0");
    check(lput(b) == 0, "lput to non-empty list returns 0");

	  car_t *get1 = lget();
	  car_t *get2 = lget();
	
    check(get1  == b, "lget returns first item");
	  check(get2 == a, "lget returns first item again");

	  free(a);
	  free(b);
}

static void test_function(car_t *cp) {
	cp->year = 2030;
}

static void test_apply_empty(void) {
	lapply(test_function);
	check(lget() == NULL, "nothing happened, list still empty");
}

static void test_apply_nonempty(void) {
	car_t *cars[3];

	put_three(cars);

	lapply(test_function);

	car_t *a = lget();
	car_t *b = lget();
	car_t *c = lget();

	check(a->year == 2030, "applied to first item");
	check(b->year == 2030, "applied to second item");
	check(c->year == 2030, "applied to third item");

	free(a);
	free(b);
	free(c);
}

int main(void) {
    test_put_null();
    test_put_empty();
    test_put_nonempty();

    test_get_empty();
		test_get_nonempty();

		test_apply_empty();
		test_apply_nonempty();
		
    if (failures > 0) {
        printf("%d tests failed\n", failures);
        exit(EXIT_FAILURE);
    }
    printf("all tests passed\n");
    exit(EXIT_SUCCESS);
}

/*
 * Allocate a car with the given plate, price, and year.
 * The `next` field is initialized to NULL.
 *
 * `price` and `year` must be >= 0.
 *
 * Returns NULL if an argument is invalid, the plate does not fit in
 * MAXREG bytes, or calloc fails. Caller must free the result.
 */
static car_t *make_car(const char *plate, double price, int year) {
    if (plate == NULL || price < 0 || year < 0) {
        return NULL;
    }

    car_t *c = calloc(1, sizeof(car_t));

    if (c == NULL || strlcpy(c->plate, plate, MAXREG) >= MAXREG) {
        free(c);
        return NULL;
    }

    c->price = price;
    c->year = year;

    return c;
}

/*
 * Wrap make_car with a plate-only creation that
 * exits the program if a failure occurs.
 *
 * For testing purposes, make_car MUST work.
 */
static car_t *must_make_car(const char *plate) {
    car_t *c = make_car(plate, 1000.0, 2020);
    if (c == NULL) {
        printf("make_car(\"%s\") failed\n", plate);
        exit(EXIT_FAILURE);
    }

    return c;
}

/*
 * Fills the given cars array with [0]AAA, [1]BBB, [2]CCC
 * and adds them to the linked list such that front -> AAA -> BBB -> CCC
 */
static void put_three(car_t *cars[3]) {
    cars[0] = must_make_car("AAA");
    cars[1] = must_make_car("BBB");
    cars[2] = must_make_car("CCC");

    for (int i = 2; i >= 0; i--) {
        check(lput(cars[i]) == 0, "lput returns 0");
    }
}

// Increment failures and print msg if cond is false
static void check(bool cond, const char *msg) {
    if (!cond) {
        printf("FAIL: %s\n", msg);
        failures++;
    }
}

// Remove expected by plate, check result, and free it
static void check_remove(car_t* expected, const char* msg) {
    car_t* got = lremove(expected->plate);
    check(got == expected, msg);
    check(got == NULL || got->next == NULL, "removed car has next == NULL");
    free(got);
    
}
