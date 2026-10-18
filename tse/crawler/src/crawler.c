/*
 * crawler.c -- implements a webpage crawler using a hash table and queue.
 *
 * Steps Completed:
 *      2. Fetch One Page
 *      3. Queue of Webpages
 *      4. Hash Table of URLs
 *      5. Save One Page
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "hash.h"
#include "queue.h"
#include "webpage.h"

#define ROOT_URL "https://thayer.github.io/engs50/"
#define TABLE_SIZE 100
#define PAGES_DIR "pages"
#define MAX_FILENAME_BYTES 1024

// Match a stored url against a search key, for hsearch.
static bool url_match(void *elementp, const void *keyp) {
    return strcmp(elementp, keyp) == 0;
}

// Print the depth and url of a webpage, for qapply.
static void print_page(void *elementp) {
    webpage_t *page = elementp;
    printf("%d %s\n", webpage_getDepth(page), webpage_getURL(page));
}

/*
 * Record a copy of `url` as visited.
 *
 * Returns false if `url` was already visited or the copy could not be stored.
 * Otherwise returns true.
 */
static bool mark_visited(hashtable_t *visited, const char *url) {
    int len = (int)strlen(url) + 1;  // include \0

    if (hsearch(visited, url_match, url, len) != NULL) {
        return false;
    }

    char *copy = malloc(len);
    if (copy == NULL) {
        return false;
    }
    strcpy(copy, url);

    if (hput(visited, copy, copy, len) != 0) {
        free(copy);
        return false;
    }

    return true;
}

/*
 * Write a fetched page to the file <dirname>/<id>
 * The directory must already exist, `id` must be positive (>0), and the
 * page must have a url and fetched html.
 *
 * FILE FORMAT:
 * ```
 * <url>
 * <depth>
 * <length of html>
 * <html content>
 * ```
 *
 * Returns 0 on success, nonzero on error.
 */
static int32_t pagesave(webpage_t *pagep, int32_t id, char *dirname) {
    if (pagep == NULL || dirname == NULL || webpage_getURL(pagep) == NULL ||
        webpage_getHTML(pagep) == NULL) {
        return 1;
    }

    if (id <= 0) {
        return 1;
    }

    struct stat st;
    if (stat(dirname, &st) != 0 || !S_ISDIR(st.st_mode)) {
        return 1;
    }

    // 12 bytes = "2147483647" + '/' + '\0'
    if (strlen(dirname) + 12 >= MAX_FILENAME_BYTES) {
        return 1;
    }

    char filename[MAX_FILENAME_BYTES];
    sprintf(filename, "%s/%d", dirname, id);

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        return 1;
    }

    // getNextURL compresses the html and does not update the internal
    // html_len field, so webpage_getHTMLlen will return the wrong value
    // need to measure the shortened string instead
    int html_len_bytes = (int)strlen(webpage_getHTML(pagep));

    fprintf(fp, "%s\n%d\n%d\n", webpage_getURL(pagep), webpage_getDepth(pagep),
         html_len_bytes);

    fwrite(webpage_getHTML(pagep), sizeof(char), html_len_bytes, fp);

    if (ferror(fp) || fclose(fp) == EOF) {
        return 1;
    }

    return 0;
}

int main(void) {
    webpage_t *root_page = webpage_new(ROOT_URL, 0, NULL);

    if (root_page == NULL || !webpage_fetch(root_page)) {
        webpage_delete(root_page);
        exit(EXIT_FAILURE);
    }

    queue_t *fetch_queue = qopen();
    hashtable_t *visited_tb = hopen(TABLE_SIZE);

    if (fetch_queue == NULL || visited_tb == NULL ||
        qput(fetch_queue, root_page) != 0 ||
        !mark_visited(visited_tb, ROOT_URL)) {
        qclose(fetch_queue);
        hclose(visited_tb);
        webpage_delete(root_page);
        exit(EXIT_FAILURE);
    }

    // must write raw seed page before the url search compresses the html
    bool ok = (pagesave(root_page, 1, PAGES_DIR) == 0);
    if (!ok) {
        printf("pagesave failed\n");
    }

    int pos = 0;
    char *url = NULL;

    while ((pos = webpage_getNextURL(root_page, pos, &url)) > 0) {
        if (IsInternalURL(url) && mark_visited(visited_tb, url)) {
            webpage_t *page =
                webpage_new(url, webpage_getDepth(root_page) + 1, NULL);

            if (qput(fetch_queue, page) != 0) {
                webpage_delete(page);
            }
        }

        free(url);
        url = NULL;
    }

    qapply(fetch_queue, print_page);

    qapply(fetch_queue, webpage_delete);
    qclose(fetch_queue);

    happly(visited_tb, free);
    hclose(visited_tb);

    exit(ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
