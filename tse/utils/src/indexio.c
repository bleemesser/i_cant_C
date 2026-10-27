#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "indexio.h"
#include "pageio.h"
#include "webpage.h"

static void fprint_doc(void *elementp, void *argp) {
    doc_t *docp = elementp;
    fprintf((FILE *)argp, " %u %u", docp->id, docp->count);
}

static void fprint_entry(void *elementp, void *argp) {
    FILE *indexf = argp;
    entry_t *ep = elementp;

    fprintf(indexf, "%s", ep->word);
    qapplyargs(ep->doc_queue, fprint_doc, indexf);
    fprintf(indexf, "\n");
}

int32_t indexsave(hashtable_t *index, const char *indexnm) {
    if (index == NULL || indexnm == NULL) {
        return 1;
    }

    FILE *indexf = fopen(indexnm, "w");
    if (indexf == NULL) {
        perror(indexnm);
        return 1;
    }

    happlyargs(index, fprint_entry, indexf);

    // apply functions cannot report failure,
    // can only check the stream
    int32_t error = ferror(indexf) != 0;

    if (fclose(indexf) != 0 || error) {
        return 1;
    }

    return 0;
}

bool entry_match(void *elementp, const void *keyp) {
    entry_t *entryip = elementp;
    return strcmp(entryip->word, keyp) == 0;
}

bool doc_match(void *elementp, const void *keyp) {
    doc_t *docip = elementp;
    const int *keyid = keyp;

    return docip->id == *keyid;
}

void free_entry(void *ep) {
    entry_t *entry = ep;
    free(entry->word);
    qapply(entry->doc_queue, free);
    qclose(entry->doc_queue);
    free(entry);
}

entry_t *entry_open(char *word, int len) {
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

doc_t *doc_open(int id) {
    doc_t *docp = malloc(sizeof(doc_t));
    if (docp == NULL) {
        return NULL;
    }
    docp->id = id;
    docp->count = 0;
    return docp;
}

int32_t normalize_word(char *word) {
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

bool index_page(hashtable_t *index, int id, char *dirname) {
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
