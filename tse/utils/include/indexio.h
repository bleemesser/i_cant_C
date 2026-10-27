#pragma once

#include <stdint.h>

#include "hash.h"
#include "queue.h"

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
 * Saves the index to file `indexnm` in the cwd.
 *
 * For each word in the index, the file contains one line:
 * <word> <docID1> <count1> ... <docIDn> <countn>
 *
 * Caller MUST ensure `index` is a hashtable of char* `word` -> `entry_t`
 *
 * Returns 0 on success, 1 on failure.
 */
int32_t indexsave(hashtable_t *index);

hashtable_t *indexload(void);

/*
 * Match stored entry against search key (word) for hsearch
 */
bool entry_match(void *elementp, const void *keyp);

/*
 * Match stored doc against search key (id) for qsearch
 */
bool doc_match(void *elementp, const void *keyp);

/*
 * Free entry function to pass into happly
 * Also frees the queue and docs in the queue
 */
void free_entry(void *ep);

/*
 * Initialize an entry
 * Requires a word and len
 * Returns an entryp or NULL if failure
 */
entry_t *entry_open(char *word, int len);

/*
 * Initialize a doc
 * Requires doc id
 * Returns a docp or NULL if failure
 */
doc_t *doc_open(int id);

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
int32_t normalize_word(char *word);

/*
 * Index a page
 * Requires index pointer, document id, and directory name
 * Returns false if failure, true if success
 */
bool index_page(hashtable_t *index, int id, char *dirname);
