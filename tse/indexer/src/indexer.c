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

    int pos = 0;
    char *word = NULL;

    while ((pos = webpage_getNextWord(p1, pos, &word)) > 0) {
        if (normalize_word(word) == 0) {
            printf("%s\n", word);
        }
        free(word);
    }
    printf("done\n");
}
