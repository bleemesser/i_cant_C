/*
 * crawler.c -- crawl the provided site and save found pages to disk
 */

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lhash.h"
#include "lqueue.h"
#include "webpage.h"

#define TABLE_SIZE 100
#define MAX_FILENAME_BYTES 1024

typedef struct crawl_state {
    lqueue_t *todo;
    lhashtable_t *visited;
    char *page_dir;
    int maxdepth;
    // guard for busy, num_pages, stop, and the visited check-then-insert
    pthread_mutex_t lock;
    // broadcast for when a thread finishes a page or stop is set
    pthread_cond_t page_done;
    int busy;
    int num_pages;
    // set to make workers exit after their current page
    bool stop;
} crawl_state_t;

// Match a stored url against a search key, for lhsearch.
static bool url_match(void *elementp, const void *keyp) {
    return strcmp(elementp, keyp) == 0;
}

/*
 * Record a copy of `url` as visited.
 *
 * Returns false if `url` was already visited or the copy could not be stored.
 * Otherwise returns true.
 */
static bool mark_visited(crawl_state_t *st, const char *url) {
    int len = (int)strlen(url) + 1;  // include \0
    bool marked = false;

    // lock so two threads cannot both miss the search and insert
    pthread_mutex_lock(&st->lock);

    if (lhsearch(st->visited, url_match, url, len) == NULL) {
        char *copy = malloc(len);

        if (copy != NULL) {
            strcpy(copy, url);

            if (lhput(st->visited, copy, copy, len) == 0) {
                marked = true;
            } else {
                free(copy);
            }
        }
    }

    pthread_mutex_unlock(&st->lock);
    return marked;
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

    bool error = ferror(fp) != 0;

    if (fclose(fp) == EOF || error) {
        return 1;
    }

    return 0;
}

// Print the cli format
static void print_usage(void) {
    printf("usage: crawler <seedurl> <pagedir> <maxdepth> <numthreads>\n");
}

/*
 * Parse an integer from the argument
 *
 * Returns true and stores the value if INT_MAX >= arg >= min,
 * otherwise returns false.
 */
static bool parse_int(const char *arg, int min, int *valuep) {
    if (arg == NULL) {
        return false;
    }

    errno = 0;
    char *end = NULL;
    long value = strtol(arg, &end, 10);
    if (errno != 0 || end == arg || *end != '\0' || value < min ||
        value > INT_MAX) {
        return false;
    }

    *valuep = (int)value;
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
static void enqueue_links(crawl_state_t *st, webpage_t *page) {
    int pos = 0;
    char *url = NULL;

    while ((pos = webpage_getNextURL(page, pos, &url)) > 0) {
        if (IsInternalURL(url)) {
            char *link = canonical_url(url);

            if (link != NULL && mark_visited(st, link)) {
                webpage_t *child =
                    webpage_new(link, webpage_getDepth(page) + 1, NULL);

                if (child == NULL || lqput(st->todo, child) != 0) {
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

// Save a fetched page under the next available id
static void save_page(crawl_state_t *st, webpage_t *page) {
    // lock during save so there is no gap or duplication/overwriting
    pthread_mutex_lock(&st->lock);

    if (pagesave(page, st->num_pages + 1, st->page_dir) == 0) {
        st->num_pages++;
    } else {
        printf("error: could not save %s\n", webpage_getURL(page));
    }

    pthread_mutex_unlock(&st->lock);
}

/*
 * Take the next page from the queue and mark the caller as busy.
 *
 * Block while the queue is empty and another thread is busy, since that
 * thread can still add links to queue. Return NULL once the queue is empty
 * and all threads idle, or once stop is set. Caller must decrement busy and
 * broadcast page_done after it finishes the page.
 */
static webpage_t *next_page(crawl_state_t *st) {
    webpage_t *page = NULL;

    pthread_mutex_lock(&st->lock);

    while (!st->stop && (page = lqget(st->todo)) == NULL && st->busy > 0) {
        pthread_cond_wait(&st->page_done, &st->lock);
    }

    if (page != NULL) {
        st->busy++;
    }

    pthread_mutex_unlock(&st->lock);
    return page;
}

// Crawl pages from the shared queue until it is empty
static void *crawl_worker(void *arg) {
    crawl_state_t *st = arg;
    webpage_t *page = NULL;

    while ((page = next_page(st)) != NULL) {
        if (webpage_fetch(page)) {
            // have to save before finding links because getNextURL modifies the html
            save_page(st, page);

            if (webpage_getDepth(page) < st->maxdepth) {
                enqueue_links(st, page);
            }
        }

        webpage_delete(page);

        pthread_mutex_lock(&st->lock);
        st->busy--;
        pthread_cond_broadcast(&st->page_done);
        pthread_mutex_unlock(&st->lock);
    }

    return NULL;
}

// Free all shared state, including the queue and table entries
static void cleanup(crawl_state_t *st) {
    if (st->todo != NULL) {
        lqapply(st->todo, webpage_delete);
        lqclose(st->todo);
    }

    if (st->visited != NULL) {
        lhapply(st->visited, free);
        lhclose(st->visited);
    }

    pthread_cond_destroy(&st->page_done);
    pthread_mutex_destroy(&st->lock);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    char *seed_url = argv[1];
    char *page_dir = argv[2];
    int maxdepth;
    int num_threads;

    if (!parse_int(argv[3], 0, &maxdepth)) {
        printf("error: maxdepth must be >= 0\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    if (!parse_int(argv[4], 1, &num_threads)) {
        printf("error: numthreads must be >= 1\n");
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

    crawl_state_t st = {
        .todo = NULL,
        .visited = NULL,
        .page_dir = page_dir,
        .maxdepth = maxdepth,
        .busy = 0,
        .num_pages = 0,
        .stop = false,
    };

    if (pthread_mutex_init(&st.lock, NULL) != 0) {
        printf("error: could not create mutex\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&st.page_done, NULL) != 0) {
        printf("error: could not create condition variable\n");
        pthread_mutex_destroy(&st.lock);
        exit(EXIT_FAILURE);
    }

    st.todo = lqopen();
    st.visited = lhopen(TABLE_SIZE);

    char *seed = canonical_url(seed_url);
    pthread_t *threads = malloc(sizeof(pthread_t) * (size_t)num_threads);

    if (seed == NULL || threads == NULL || st.todo == NULL ||
        st.visited == NULL) {
        free(seed);
        free(threads);
        cleanup(&st);
        exit(EXIT_FAILURE);
    }

    webpage_t *seed_page = webpage_new(seed, 0, NULL);
    bool queued = (seed_page != NULL && mark_visited(&st, seed) &&
                   lqput(st.todo, seed_page) == 0);

    free(seed);

    if (!queued) {
        if (seed_page != NULL) {
            webpage_delete(seed_page);
        }
        free(threads);
        cleanup(&st);
        exit(EXIT_FAILURE);
    }

    int started;
    for (started = 0; started < num_threads; started++) {
        if (pthread_create(&threads[started], NULL, crawl_worker, &st) != 0) {
            printf("error: could not create thread %d\n", started + 1);

            pthread_mutex_lock(&st.lock);
            st.stop = true;
            pthread_cond_broadcast(&st.page_done);
            pthread_mutex_unlock(&st.lock);
            break;
        }
    }

    for (int i = 0; i < started; i++) {
        pthread_join(threads[i], NULL);
    }

    bool ok = !st.stop;
    if (ok) {
        printf("saved %d pages in %s using %d threads\n", st.num_pages,
               page_dir, started);
    }

    free(threads);
    cleanup(&st);
    exit(ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
