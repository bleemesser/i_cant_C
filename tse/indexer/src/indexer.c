/*
 * indexer.c -- indexes crawled pages
 */

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pageio.h"
#include "webpage.h"
#include "hash.h"
#include "queue.h"

//global variable for counting cuz idk how else to do it
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
	if (docip != NULL) word_count += docip->count;
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
		free(entryp);
		return NULL;
	}

	//entryp->word initialization
	entryp->word = malloc(len);
	if (entryp->word == NULL) {
		free(entryp->word);
		free(entryp);
		return NULL;
	}
	strcpy(entryp->word, word);

	//entryp->doc_queue initialization
	entryp->doc_queue = qopen();
	if (entryp->doc_queue == NULL) {
		qclose(entryp->doc_queue);
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
		free(docp);
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


int main(void) {
    webpage_t *p1 = pageload(1, "pages");
    if (p1 == NULL) {
        printf("page 1 loaded as null\n");
				return 1;
    }

		//instantiate index hashtable
		hashtable_t *index = hopen(1000);

		
    int pos = 0;
    char *word = NULL;
		int id = 1;
		
    while ((pos = webpage_getNextWord(p1, pos, &word)) > 0) {
        if (normalize_word(word) == 0) {
					  int len = (int)strlen(word) + 1;
						entry_t *entryp = hsearch(index, entry_match, word, len);

						//if entryp == NULL, need to malloc and put in index
						if (entryp == NULL) {
						  entryp = entry_open(word, len);

							if (entryp == NULL) {
								free(word);
								printf("failure to create entry\n");
								continue;
							}

							hput(index, entryp, word, len);
						}
						
						//check for doc
						doc_t *docp = qsearch(entryp->doc_queue, doc_match, &id);
						
						if (docp == NULL) {
							docp = doc_open(id);
							qput(entryp->doc_queue, docp);
						}

						docp->count++;
        }
        free(word);
    }

		word_count = 0;
		happly(index, count_words_entry);

		printf("%d words stored\n", word_count);
		
		//free stuff
		happly(index, free_entry);
		hclose(index);
		webpage_delete(p1);
}
