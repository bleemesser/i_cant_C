#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "indexio.h"
#include "queue.h"

#define DELIM " \t\r\n\v\f"
#define MAX_PATH_BYTES 512
#define MAX_URL_BYTES 512

typedef struct result {
    uint32_t id;
    uint32_t rank;
} result_t;

typedef struct rank_ctx {
    entry_t **entries;
    int entry_count;
    queue_t *results;
} rank_ctx_t;

static void print_usage(void) {
    printf("usage: querier <pagedir> <indexnm>\n");
}

static bool is_dir(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool result_match(void *ep, const void *keyp) {
    return ((result_t *)ep)->id == *(const uint32_t *)keyp;
}

/*
 * Add the doc's minimum count to its rank if every other entry in the
 * context also contains it
 */
static void queue_ranked_doc(void *ep, void *argp) {
    doc_t *doc = ep;
    rank_ctx_t *ctx = argp;

    int id = (int)doc->id;
    uint32_t rank = doc->count;
    for (int i = 1; i < ctx->entry_count; i++) {
        doc_t *other = qsearch(ctx->entries[i]->doc_queue, doc_match, &id);
        if (other == NULL) {
            return;
        }
        if (other->count < rank) {
            rank = other->count;
        }
    }

    result_t *result = qsearch(ctx->results, result_match, &doc->id);
    if (result != NULL) {
        result->rank += rank;
        return;
    }
    result = malloc(sizeof(result_t));
    if (result == NULL) {
        exit(EXIT_FAILURE);
    }
    result->id = doc->id;
    result->rank = rank;
    if (qput(ctx->results, result) != 0) {
        exit(EXIT_FAILURE);
    }
}

/*
 * Return true if a ranks above b.
 */
static bool outranks(const result_t *a, const result_t *b) {
    return a->rank > b->rank || (a->rank == b->rank && a->id < b->id);
}

/*
 * Point *argp at ep if *argp is NULL or ep outranks it.
 */
static void keep_higher_ranked(void *ep, void *argp) {
    result_t *result = ep;
    result_t **top = argp;
    if (*top == NULL || outranks(result, *top)) {
        *top = result;
    }
}

static bool pointer_match(void *ep, const void *keyp) {
    return ep == keyp;
}

/*
 * Read the URL from the first line of <pagedir>/<id>
 */
static bool load_url(const char *pagedir, uint32_t id, char *url, size_t size) {
    char filename[MAX_PATH_BYTES];
    // 12 bytes = "4294967295" + '/' + '\0'
    if (strlen(pagedir) + 12 >= sizeof(filename)) {
        return false;
    }
    sprintf(filename, "%s/%u", pagedir, id);

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        return false;
    }

    bool ok = fgets(url, (int)size, fp) != NULL;
    fclose(fp);
    if (!ok) {
        return false;
    }

    size_t len = strlen(url);
    if (len > 0 && url[len - 1] == '\n') {
        url[len - 1] = '\0';
    }
    return true;
}

/*
 * Print docs ranked by the sum over "or" groups of each group's minimum
 * word count, highest rank first.
 */
static void print_ranked_docs(hashtable_t *index, const char *pagedir,
                              char **words, const bool *skip, int word_count) {
    entry_t **entries = malloc(word_count * sizeof(entry_t *));
    queue_t *results = qopen();
    if (entries == NULL || results == NULL) {
        exit(EXIT_FAILURE);
    }

    rank_ctx_t ctx = {entries, 0, results};
    bool missing = false;
    for (int i = 0; i <= word_count; i++) {
        if (i == word_count || strcmp(words[i], "or") == 0) {
            if (!missing && ctx.entry_count > 0) {
                qapplyargs(entries[0]->doc_queue, queue_ranked_doc, &ctx);
            }
            ctx.entry_count = 0;
            missing = false;
        } else if (!skip[i]) {
            int len = (int)strlen(words[i]) + 1;
            entry_t *entry = hsearch(index, entry_match, words[i], len);
            if (entry == NULL) {
                missing = true;
            } else {
                entries[ctx.entry_count++] = entry;
            }
        }
    }
    free(entries);

    result_t *top = NULL;
    qapplyargs(results, keep_higher_ranked, &top);
    if (top == NULL) {
        printf("[no documents match]\n");
    }
    while (top != NULL) {
        qremove(results, pointer_match, top);

        char url[MAX_URL_BYTES];
        if (!load_url(pagedir, top->id, url, sizeof(url))) {
            strcpy(url, "[url not found]");
        }
        printf("rank: %u: doc: %u : %s\n", top->rank, top->id, url);

        free(top);
        top = NULL;
        qapplyargs(results, keep_higher_ranked, &top);
    }
    qclose(results);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    char *pagedir = argv[1];
    char *index_nm = argv[2];

    if (!is_dir(pagedir)) {
        printf("error: %s is not a directory\n", pagedir);
        print_usage();
        exit(EXIT_FAILURE);
    }

    hashtable_t *index = index_load(index_nm);
    if (index == NULL) {
        printf("error: cannot load index %s\n", index_nm);
        print_usage();
        exit(EXIT_FAILURE);
    }

    printf("Querier:\n");
    printf("> ");

    char line[1024];

    while (fgets(line, sizeof(line), stdin) != NULL) {
        char **words = NULL;
        int word_count = 0;
        int capacity = 0;

        char *token = strtok(line, DELIM);

        while (token != NULL) {
            if (word_count >= capacity) {
                capacity = capacity == 0 ? 4 : 2 * capacity;
                char **new_list = realloc(words, capacity * sizeof(char *));
                if (!new_list) {
                    exit(EXIT_FAILURE);
                }
                words = new_list;
            }

            words[word_count] = malloc(strlen(token) + 1);
            if (words[word_count] == NULL) {
                exit(EXIT_FAILURE);
            }
            strcpy(words[word_count], token);
            word_count++;

            token = strtok(NULL, DELIM);
        }

        bool valid = true;
        bool *skip = calloc(word_count, sizeof(bool));
        if (skip == NULL && word_count > 0) {
            exit(EXIT_FAILURE);
        }

        // an operator is invalid at the start/end or after another operator
        bool prev_op = true;
        for (int i = 0; i < word_count; i++) {
            int32_t norm_flag = normalize_word(words[i]);
            if (norm_flag == 1) {
                valid = false;
                break;
            }
            if (norm_flag == 2) {
                // normalize_word does not lowercase words it rejects for length,
                // convert here so the print looks nice
                for (char *c = words[i]; *c != '\0'; c++) {
                    *c = (char)tolower((unsigned char)*c);
                }
                skip[i] = true;
            }
            bool op = strcmp(words[i], "and") == 0 || strcmp(words[i], "or") == 0;
            if (op) {
                skip[i] = true;
                if (prev_op) {
                    valid = false;
                    break;
                }
            }
            prev_op = op;
        }
        if (prev_op && word_count > 0) {
            valid = false;
            printf("[invalid query]\n");

        }

        if (valid && word_count > 0) {
            for (int i = 0; i < word_count; i++) {
                printf("%s%s", words[i], i + 1 < word_count ? " " : "\n");
            }
            print_ranked_docs(index, pagedir, words, skip, word_count);
        }

        printf("> ");

        for (int i = 0; i < word_count; i++) {
            free(words[i]);
        }
        free(words);
        free(skip);
    }

    printf("\n");

    happly(index, free_entry);
    hclose(index);

    return 0;
}
