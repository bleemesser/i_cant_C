/*
 * indexer.c -- indexes crawled pages
 */

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "indexio.h"

bool parse_id(const char *arg, int *intp) {
    if (arg == NULL) {
        return false;
    }

    errno = 0;
    char *end = NULL;
    long value = strtol(arg, &end, 10);
    if (errno != 0 || end == arg || *end != '\0' || value < 1 ||
        value > INT_MAX) {
        return false;
    }

    *intp = (int)value;
    return true;
}

int main(int argc, char *argv[]) {
    int max_id;
    if (argc != 2) {
        printf("usage: indexer <max_id>\n");
        exit(EXIT_FAILURE);
    }
    if (!parse_id(argv[1], &max_id)) {
        printf("error: max_id must be >= 1\n");
        exit(EXIT_FAILURE);
    }

    hashtable_t *index = hopen(1000);

    for (int id = 1; id <= max_id; id++) {
        index_page(index, id, "pages");
    }

    indexsave(index);

    happly(index, free_entry);
    hclose(index);
}
