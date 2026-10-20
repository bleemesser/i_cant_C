/*
 * crawler.c -- crawl the provided site and save found pages to disk
 */

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hash.h"
#include "queue.h"
#include "webpage.h"

#define TABLE_SIZE 100
#define MAX_FILENAME_BYTES 1024

// Match a stored url against a search key, for hsearch.
static bool url_match(void *elementp, const void *keyp) {
    return strcmp(elementp, keyp) == 0;
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

// Print the cli format
static void print_usage(void) {
    printf("usage: crawler <seedurl> <pagedir> <maxdepth>\n");
}

/*
 * Parse maxdepth from the argument
 *
 * Returns true and stores the value if INT_MAX >= arg >= 0,
 * otherwise returns false.
 */
static bool parse_maxdepth(const char *arg, int *depthp) {
    if (arg == NULL) {
        return false;
    }

    errno = 0;
    char *end = NULL;
    long value = strtol(arg, &end, 10);
    if (errno != 0 || end == arg || *end != '\0' || value < 0 ||
        value > INT_MAX) {
        return false;
    }

    *depthp = (int)value;
    return true;
}

// Return true if `path` is a directory that exists and is writable
static bool is_writable_dir(const char *path) {
    struct stat st;

    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        return false;
    }

    return access(path, W_OK) == 0;
}

/*
 * Return a newly allocated copy of `url` in the one form the crawler uses,
 * or NULL if the copy cannot be allocated.
 *
 * Drops any #fragment and adds a trailing slash when the last path segment has
 * no '.', so that the server does not return 301 on the non-slash version.
 */
static char *canonical_url(const char *url) {
    const char *hash = strchr(url, '#');

    size_t len;
    if (hash != NULL) {
        len = (size_t)(hash - url);
    } else {
        len = strlen(url);
    }

    // find the final '/' of the path
    const char *slash = NULL;
    for (const char *p = url; p < url + len; p++) {
        if (*p == '/') {
            slash = p;
        }
    }

    bool add_slash = false;
    // if slash + 1 < url + len, then the segment after the final '/' is not empty,
    if (slash != NULL && slash + 1 < url + len) {
        size_t rest = (size_t)(url + len - slash);
        // leave '.' and query strings alone
        add_slash = (memchr(slash, '.', rest) == NULL &&
                     memchr(slash, '?', rest) == NULL);
    }
    add_slash =
        false;  // NOTE: this makes this function do nothing, remove it for dedup

    char *canon = malloc(len + (add_slash ? 2 : 1));
    if (canon == NULL) {
        return NULL;
    }

    memcpy(canon, url, len);
    if (add_slash) {
        canon[len++] = '/';
    }
    canon[len] = '\0';

    return canon;
}

/*
 * Add every internal link in `page` that has not been visited to `todo` at
 * one greater depth.
 *
 * Marks a link as visited even if its page cannot be allocated or queued,
 * to prevent loops.
 */
static void enqueue_links(queue_t *todo, hashtable_t *visited,
                          webpage_t *page) {
    int pos = 0;
    char *url = NULL;

    while ((pos = webpage_getNextURL(page, pos, &url)) > 0) {
        if (IsInternalURL(url)) {
            char *link = canonical_url(url);

            if (link != NULL && mark_visited(visited, link)) {
                webpage_t *child =
                    webpage_new(link, webpage_getDepth(page) + 1, NULL);

                if (child == NULL || qput(todo, child) != 0) {
                    if (child != NULL) {
                        webpage_delete(child);
                    }
                }
            }

            free(link);
        }

        free(url);
        url = NULL;
    }
}

// Free all contained entries in the queue and hash table, then close them
static void cleanup(queue_t *todo, hashtable_t *visited) {
    if (todo != NULL) {
        qapply(todo, webpage_delete);
        qclose(todo);
    }

    if (visited != NULL) {
        happly(visited, free);
        hclose(visited);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    char *seed_url = argv[1];
    char *page_dir = argv[2];
    int maxdepth;

    if (!parse_maxdepth(argv[3], &maxdepth)) {
        printf("error: maxdepth must be >= 0\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    if (!is_writable_dir(page_dir)) {
        printf("error: output dir does not exist or is not writable\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    if (!NormalizeURL(seed_url)) {
        printf("error: seed url is not valid\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    char *seed = canonical_url(seed_url);

    queue_t *todo = qopen();
    hashtable_t *visited = hopen(TABLE_SIZE);

    if (seed == NULL || todo == NULL || visited == NULL) {
        cleanup(todo, visited);
        exit(EXIT_FAILURE);
    }

    webpage_t *seed_page = webpage_new(seed, 0, NULL);
    bool queued = (seed_page != NULL && mark_visited(visited, seed) &&
                   qput(todo, seed_page) == 0);

    free(seed);

    if (!queued) {
        if (seed_page != NULL) {
            webpage_delete(seed_page);
        }
        cleanup(todo, visited);
        exit(EXIT_FAILURE);
    }

    int num_pages = 0;
    webpage_t *page = NULL;

    while ((page = qget(todo)) != NULL) {
        if (webpage_fetch(page)) {
            // have to save before finding links because getNextURL modifies the html
            if (pagesave(page, num_pages + 1, page_dir) == 0) {
                num_pages++;
            } else {
                printf("error: could not save %s\n", webpage_getURL(page));
            }

            if (webpage_getDepth(page) < maxdepth) {
                enqueue_links(todo, visited, page);
            }
        }

        webpage_delete(page);
    }

    printf("saved %d pages in %s\n", num_pages, page_dir);

    cleanup(todo, visited);
    exit(EXIT_SUCCESS);
}
