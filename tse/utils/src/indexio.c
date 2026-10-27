#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "indexio.h"
#include "pageio.h"
#include "webpage.h"

// allocate an entry holding a copy of `word`
static entry_t *entry_open(const char *word) {
    entry_t *entryp = malloc(sizeof(entry_t));
    if (entryp == NULL) {
        return NULL;
    }

    entryp->word = malloc(strlen(word) + 1);
    if (entryp->word == NULL) {
        free(entryp);
        return NULL;
    }
    strcpy(entryp->word, word);

    entryp->doc_queue = qopen();
    if (entryp->doc_queue == NULL) {
        free(entryp->word);
        free(entryp);
        return NULL;
    }

    return entryp;
}

// create a new doc with `id` and a count of 0
static doc_t *doc_open(uint32_t id) {
    doc_t *docp = malloc(sizeof(doc_t));
    if (docp == NULL) {
        return NULL;
    }
    docp->id = id;
    docp->count = 0;
    return docp;
}

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

int32_t index_save(hashtable_t *index, const char *indexnm) {
    if (index == NULL || indexnm == NULL) {
        return 1;
    }

    FILE *indexf = fopen(indexnm, "w");
    if (indexf == NULL) {
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

// read the <docID> <count> pairs that follow `word` into a new entry
static bool load_entry(FILE *indexf, hashtable_t *index, char *word) {
    entry_t *entryp = entry_open(word);
    if (entryp == NULL) {
        return false;
    }

    uint32_t id, count;
    while (fscanf(indexf, "%u %u", &id, &count) == 2) {
        doc_t *docp = doc_open(id);
        if (docp == NULL) {
            free_entry(entryp);
            return false;
        }
        docp->count = count;
        if (qput(entryp->doc_queue, docp) != 0) {
            free(docp);
            free_entry(entryp);
            return false;
        }
    }

    int len = (int)strlen(word) + 1;
    if (hput(index, entryp, word, len) != 0) {
        free_entry(entryp);
        return false;
    }

    return true;
}

hashtable_t *index_load(const char *indexnm) {
    if (indexnm == NULL) {
        return NULL;
    }

    FILE *indexf = fopen(indexnm, "r");
    if (indexf == NULL) {
        return NULL;
    }

    hashtable_t *index = hopen(INDEX_LOAD_HTABLE_SIZE);
    if (index == NULL) {
        fclose(indexf);
        return NULL;
    }

    // normalize_word rejects words over 127 chars
    // %127s reads at most 127 chars
    char word[128];
    while (fscanf(indexf, "%127s", word) == 1) {
        // load_entry stops at the start of the next line's word or EOF
        if (!load_entry(indexf, index, word)) {
            fclose(indexf);
            happly(index, free_entry);
            hclose(index);
            return NULL;
        }
    }

    fclose(indexf);
    return index;
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

int32_t normalize_word(char *word) {
    if (word == NULL) {
        return 1;
    }

    size_t len = strlen(word);
    if (len < 3 || len > 127) {
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
                entryp = entry_open(word);

                if (entryp == NULL) {
                    free(word);
                    webpage_delete(p1);
                    return false;
                }
                if (hput(index, entryp, word, len) != 0) {
                    free_entry(entryp);
                    free(word);
                    webpage_delete(p1);
                    return false;
                }
            }

            // check for doc
            doc_t *docp = qsearch(entryp->doc_queue, doc_match, &id);

            if (docp == NULL) {
                docp = doc_open(id);

                if (docp == NULL || qput(entryp->doc_queue, docp) != 0) {
                    free(docp);
                    free(word);
                    webpage_delete(p1);
                    return false;
                }
            }

            docp->count++;
        }
        free(word);
    }

    webpage_delete(p1);
    return true;
}
