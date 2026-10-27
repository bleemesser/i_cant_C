/*
 * indexer.c -- indexes crawled pages
 */

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "pageio.h"
#include "queue.h"
#include "webpage.h"

static int word_count;

/*
 * Doc for doc_queue
 * contains doc_id and count
 */
typedef struct doc {
    uint32_t id;
    uint32_t count;
} doc_t;

/*
 * Hashtable entry structure
 * Contains word and doc_queue
 */
typedef struct entry {
    char *word;
    queue_t *doc_queue;
} entry_t;

/*
 * Match stored entry against search key (word) for hsearch
 */
static bool entry_match(void *elementp, const void *keyp) {
    entry_t *entryip = elementp;
    return strcmp(entryip->word, keyp) == 0;
}

/*
 * Match stored doc against search key (id) for qsearch
 */
static bool doc_match(void *elementp, const void *keyp) {
    doc_t *docip = elementp;
    const int *keyid = keyp;

    return docip->id == *keyid;
}

/*
 * Increase word_count function, for docs
 */
static void count_words_doc(void *ep) {
    doc_t *docip = ep;
    if (docip != NULL) {
        word_count += docip->count;
    }
}

/*
 * Increase word_count function, for entries
 */
static void count_words_entry(void *ep) {
    entry_t *entryip = ep;
    qapply(entryip->doc_queue, count_words_doc);
}

/*
 * Free entry function to pass into happly
 * Also frees the queue and docs in the queue
 */
static void free_entry(void *ep) {
    entry_t *entry = ep;
    free(entry->word);
    qapply(entry->doc_queue, free);
    qclose(entry->doc_queue);
    free(entry);
}

/*
 * Initialize an entry
 * Requires a word and len
 * Returns an entryp or NULL if failure
 */
static entry_t *entry_open(char *word, int len) {
    entry_t *entryp = malloc(sizeof(entry_t));
    if (entryp == NULL) {
        return NULL;
    }

    // entryp->word initialization
    entryp->word = malloc(len);
    if (entryp->word == NULL) {
        free(entryp);
        return NULL;
    }
    strcpy(entryp->word, word);

    // entryp->doc_queue initialization
    entryp->doc_queue = qopen();
    if (entryp->doc_queue == NULL) {
        free(entryp->word);
        free(entryp);
        return NULL;
    }

    return entryp;
}

/*
 * Initialize a doc
 * Requires doc id
 * Returns a docp or NULL if failure
 */
static doc_t *doc_open(int id) {
    doc_t *docp = malloc(sizeof(doc_t));
    if (docp == NULL) {
        return NULL;
    }
    docp->id = id;
    docp->count = 0;
    return docp;
}

/*
 * Convert a word to lowercase in-place.
 *
 * Returns 0 if the word was successfully converted.
 * Returns 1 if the word does not meet these criteria:
 * - The word contains non-alphabetic characters
 * - The word is < 3 characters in length
 * - `word` is NULL
 *
 * If 1 is returned, the word is not modifed.
 */
static int32_t normalize_word(char *word) {
    if (word == NULL) {
        return 1;
    }

    size_t len = strlen(word);
    if (len < 3) {
        return 1;
    }

    for (size_t i = 0; i < len; i++) {
        if (!isalpha((unsigned char)word[i])) {
            return 1;
        }
    }

    for (size_t i = 0; i < len; i++) {
        word[i] = (char)tolower((unsigned char)word[i]);
    }

    return 0;
}

/*
 * Index a page
 * Requires index pointer, document id, and directory name
 * Returns false if failure, true if success
 */
static bool index_page(hashtable_t *index, int id, char *dirname) {
    webpage_t *p1 = pageload(id, dirname);
    if (p1 == NULL) {
        printf("page %d loaded as null\n", id);
        webpage_delete(p1);
        return false;
    }

    int pos = 0;
    char *word = NULL;

    while ((pos = webpage_getNextWord(p1, pos, &word)) > 0) {
        if (normalize_word(word) == 0) {
            int len = (int)strlen(word) + 1;
            entry_t *entryp = hsearch(index, entry_match, word, len);

            // if entryp == NULL, need to malloc and put in index
            if (entryp == NULL) {
                entryp = entry_open(word, len);

                if (entryp == NULL) {
                    free(word);
                    webpage_delete(p1);
                    printf("failure to create entry\n");
                    return false;
                }

                hput(index, entryp, word, len);
            }

            // check for doc
            doc_t *docp = qsearch(entryp->doc_queue, doc_match, &id);

            if (docp == NULL) {
                docp = doc_open(id);

                if (docp == NULL) {
                    free(word);
                    webpage_delete(p1);
                    printf("failure to create doc\n");
                    return false;
                }

                qput(entryp->doc_queue, docp);
            }

            docp->count++;
        }
        free(word);
    }

    webpage_delete(p1);
    return true;
}

static bool parse_id(const char *arg, int *intp) {
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

    word_count = 0;
    happly(index, count_words_entry);

    printf("%d words stored\n", word_count);

    happly(index, free_entry);
    hclose(index);
}
