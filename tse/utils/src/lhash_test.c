/*
 * lhash_test.c --- tests for lhash module
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "lhash.h"

static int failures = 0;

// arguments for thread
typedef struct {
    lhashtable_t *lhtp;
    int start_year;
    int count;
	  int scans;
} arg_t;


// count and report a failure if cond is false
static void check(bool cond, char *msg) {
    if (!cond) {
        printf("FAIL: %s\n", msg);
        failures++;
    }
}


// count each call, for testing lhapplyargs
static void count_call(void *elementp, void *argp) {
    (void)elementp;
    int *countp = argp;
    (*countp)++;
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

// putting function for threading
static void *put_lots(void *argp) {
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 250000000L };
	arg_t *argip = argp;
	char key[16];
	for (int i = 0; i < argip->count; i++) {
    int *yearp = malloc(sizeof(int));
		*yearp = argip->start_year + i;
		snprintf(key, sizeof(key), "%d", *yearp);
		lhput(argip->lhtp, yearp, key, 4);
		nanosleep(&ts, NULL);
  }
  return NULL;
}

// removing function for threading
static void *remove_lots(void *argp) {
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 250000000L };
	arg_t *argip = argp;
	char key[16];
	for (int i = 0; i < argip->count; i++) {
    snprintf(key, sizeof(key), "%d", argip->start_year + i);
		int *removedp = lhremove(argip->lhtp, find_year_match, key, 4);
		free(removedp);
		nanosleep(&ts, NULL);
	}
	return NULL;
}

// repeated counting function for threading
static void *keep_counting(void *argp) {
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 250000000L };
	arg_t *argip = argp;
	int curr_count = 0;
	argip->scans = 0;

	while (curr_count < argip->count) {
		curr_count = 0;
		lhapplyargs(argip->lhtp, count_call, &curr_count);
		argip->scans++;
		nanosleep(&ts, NULL);
	}
	return NULL;
}

static void test_dual_put_and_remove(void) {
    lhashtable_t *lhtp = lhopen(8);
    pthread_t t1, t2;

    
    arg_t a1 = { lhtp, 1000, 10, 0 };
    arg_t a2 = { lhtp, 1500, 10, 0 };

    pthread_create(&t1, NULL, put_lots, &a1);
    pthread_create(&t2, NULL, put_lots, &a2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    int total = 0;
    lhapplyargs(lhtp, count_call, &total);
    check(total == 20, "dual lhput inserts all 20 elements");


    pthread_create(&t1, NULL, remove_lots, &a1);
    pthread_create(&t2, NULL, remove_lots, &a2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    total = 0;
    lhapplyargs(lhtp, count_call, &total);
    check(total == 0, "dual lhremove removes all 20 elements");

    lhclose(lhtp);
}

static void test_dual_put_and_count(void) {
    lhashtable_t *lhtp = lhopen(8);
    pthread_t changer, counter;

    arg_t chng_arg = { lhtp, 1000, 10, 0};
    arg_t cnt_arg = { lhtp, 1000, 10, 0};

    pthread_create(&changer, NULL, put_lots, &chng_arg);
    pthread_create(&counter, NULL, keep_counting, &cnt_arg);

    pthread_join(changer, NULL);
    pthread_join(counter, NULL);

    check(cnt_arg.scans >= 1, "counter scanned table while changer inserted");
    printf("Counter scanned %d times before reaching 10 elements\n", cnt_arg.scans);

    lhapply(lhtp, free);
		
    lhclose(lhtp);
}


int main(void) {
	  test_dual_put_and_remove();
    test_dual_put_and_count();

		if (failures > 0) {
        printf("%d checks failed\n", failures);
        exit(EXIT_FAILURE);
    }

    printf("all tests passed\n");
    exit(EXIT_SUCCESS);
}
