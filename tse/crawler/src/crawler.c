/*
 * Crawler
 * Steps Completed:
 *      2. Fetch One Page
 *      3. Queue of Webpages
 * Step to be graded:
 *      4. Hash Table of URLs
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "queue.h"
#include "webpage.h"

static void print_url(void *wp) {
    webpage_t *wip = wp;
    printf("%s\n", webpage_getURL(wip));
}

static bool search_url(void *urlp, const void *keyp) {
    return (strcmp(urlp, keyp) == 0);
}

int main(void) {
    // root page creation
    webpage_t *root_page =
        webpage_new("https://thayer.github.io/engs50/", 0, NULL);

    if (!webpage_fetch(root_page)) {
        webpage_delete(root_page);
        printf("Fetch failed");
        exit(EXIT_FAILURE);
    }

    // queue creation
    queue_t *qp = qopen();
    if (qput(qp, root_page) != 0) {
        printf("qput failed\n");
    }

    // hashtable creation
    hashtable_t *htp = hopen(100);

    char *root_url = webpage_getURL(root_page);
    int length = (int)strlen(root_url) + 1;
    char *root_url_copy =
        malloc(length);  // gotta make a copy to make the freeing work
    strcpy(root_url_copy, root_url);
    if (hput(htp, root_url_copy, root_url_copy, length) != 0) {
        printf("hput failed\n");
    }

    // go through and get urls
    int pos = 0;
    char *result;

    while ((pos = webpage_getNextURL(root_page, pos, &result)) > 0) {
        if (IsInternalURL(result)) {
            int keylen = (int)strlen(result) + 1;

            if (hsearch(htp, search_url, result, keylen) == NULL) {
                // create copy of result; result has to be freed every loop
                char *key = malloc(keylen);
                if (key == NULL) {
                    printf("allocation failure");
                    exit(EXIT_FAILURE);
                }
                strcpy(key, result);

                if (hput(htp, key, key, keylen) == 0) {
                    webpage_t *new_page = webpage_new(key, 0, NULL);
                    if (qput(qp, new_page) != 0) {
                        printf("qput failed\n");
                    }
                } else {
                    printf("hput failed\n");
                }
            }
        }
        free(result);
    }

    // print queue of webpages
    qapply(qp, print_url);

    // delete everything
    happly(htp, free);
    qapply(qp, webpage_delete);
    hclose(htp);
    qclose(qp);

    exit(EXIT_SUCCESS);
}
