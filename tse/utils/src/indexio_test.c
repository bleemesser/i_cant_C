/*
 * indexio_test.c --- test for indexio.c
 *
 * Description: loads a known index file, saves it, and loads it back
 *
 */

#include "hash.h"
#include "indexio.h"
#include "queue.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// true if `word` appears `count` times in doc `id`
static bool has_count(hashtable_t *index, char *word, int id, uint32_t count) {
	entry_t *entryp = hsearch(index, entry_match, word, (int)strlen(word) + 1);
	if (entryp == NULL) {
		return false;
	}
	doc_t *docp = qsearch(entryp->doc_queue, doc_match, &id);
	return docp != NULL && docp->count == count;
}

// count failed checks against the known index contents
static int check(hashtable_t *index, char *name) {
	int fails = 0;
	if (!has_count(index, "apple", 1, 2) || !has_count(index, "apple", 3, 1)) {
		fails++;
		printf("%s: wrong apple\n", name);
	}
	if (!has_count(index, "banana", 2, 5)) {
		fails++;
		printf("%s: wrong banana\n", name);
	}
	if (hsearch(index, entry_match, "cherry", 7) != NULL) {
		fails++;
		printf("%s: found cherry\n", name);
	}
	return fails;
}

int32_t main(void) {
	int fails = 0;
	char in[] = "indexio_test_in";
	char out[] = "indexio_test_out";

	FILE *f = fopen(in, "w");
	if (f == NULL) {
		printf("failure to write %s\n", in);
		exit(EXIT_FAILURE);
	}
	fprintf(f, "apple 1 2 3 1\nbanana 2 5\n");
	fclose(f);

	hashtable_t *index_1 = index_load(in);
	if (index_1 == NULL) {
		printf("failure to load\n");
		exit(EXIT_FAILURE);
	}
	fails += check(index_1, "index_1");

	if (index_save(index_1, out) != 0) {
		printf("failure to save\n");
		exit(EXIT_FAILURE);
	}

	// load what index_save wrote
	hashtable_t *index_2 = index_load(out);
	if (index_2 == NULL) {
		printf("failure to reload\n");
		exit(EXIT_FAILURE);
	}
	fails += check(index_2, "index_2");

	if (index_load("indexio_test_missing") != NULL) {
		fails++;
		printf("loaded a missing file\n");
	}

	happly(index_1, free_entry);
	hclose(index_1);
	happly(index_2, free_entry);
	hclose(index_2);
	remove(in);
	remove(out);

	if (fails == 0) {
		printf("all tests passed\n");
		exit(EXIT_SUCCESS);
	}

	printf("%d failures\n", fails);
	exit(EXIT_FAILURE);
}
