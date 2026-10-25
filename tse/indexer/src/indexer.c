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
 * Step 3 structure
 * contains Word and Count
 */
typedef struct entry {
	char *word;
	uint32_t count;
} entry_t;

/*
 * Match stored entry against search key for hsearch
 */
static bool entry_match(void *elementp, const void *keyp) {
	entry_t *entryip = elementp;
	return strcmp(entryip->word, keyp) == 0;
}

/*
 * Increase word_count function
 */
static void count_words(void *ep) {
	if (ep != NULL) word_count++;
}

/*
 * Free entry function
 */
static void free_entry(void *ep) {
	entry_t *entry = ep;
	free(entry->word);
	free(entry);
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
    }

		//instantiate index hashtable
		hashtable_t *index = hopen(1000);

		
    int pos = 0;
    char *word = NULL;

    while ((pos = webpage_getNextWord(p1, pos, &word)) > 0) {
        if (normalize_word(word) == 0) {
						int len = (int)strlen(word) + 1;

						entry_t *entryp = hsearch(index, entry_match, word, len);

						//if entryp == NULL, need to malloc and put in index
						if (entryp == NULL) {
							entryp = malloc(sizeof(entry_t));
							if (entryp == NULL) {
								free(entryp);
								printf("malloc failure");
								continue;
							}

							entryp->word = malloc(len);
							
							if (entryp->word == NULL) {
								free(entryp);
								free(word);
								printf("malloc failure");
								continue;
							}
							
							strcpy(entryp->word, word);

							entryp->count = 0;

							hput(index, entryp, word, len);
						}
						
						//increase count
						entryp->count++;
        }
        free(word);
    }

		word_count = 0;
		happly(index, count_words);

		//free stuff
		happly(index, free_entry);
		hclose(index);
		
		printf("%d words stored", word_count);
}
