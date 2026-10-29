/*
 * indexer.c -- indexes crawled pages
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "hash.h"
#include "indexio.h"

static void print_usage(void) {
    printf("usage: indexer <pagedir> <indexnm>\n");
}

static bool is_dir(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    char *page_dir = argv[1];
    char *index_nm = argv[2];

    if (!is_dir(page_dir)) {
        printf("error: %s is not a directory\n", page_dir);
        print_usage();
        exit(EXIT_FAILURE);
    }

    hashtable_t *index = hopen(INDEX_LOAD_HTABLE_SIZE);
    if (index == NULL) {
        printf("error: could not create index table\n");
        exit(EXIT_FAILURE);
    }

    // the crawler numbers pages 1..N so stop at first missing id
    int num_pages = 0;
    while (index_page(index, num_pages + 1, page_dir)) {
        num_pages++;
    }

    int status = EXIT_FAILURE;
    if (num_pages == 0) {
        printf("error: no pages found in %s\n", page_dir);
        // ^ this may not be an error once the crawler/indexer run together
    } else if (index_save(index, index_nm) != 0) {
        printf("error: could not save index to %s\n", index_nm);
    } else {
        printf("indexed %d pages from %s in %s\n", num_pages, page_dir,
               index_nm);
        status = EXIT_SUCCESS;
    }

    happly(index, free_entry);
    hclose(index);
    exit(status);
}
