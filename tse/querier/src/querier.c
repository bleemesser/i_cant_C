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

typedef struct fancy_arg {
	hashtable_t *index;
	queue_t *rank_ctx_q;
} fancy_arg_t

static void print_usage(void) {
    printf("usage: querier <pagedir> <indexnm>\n");
}

static bool is_dir(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/*
 * Queue doc with its minimum count as rank if every other entry in the
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

    result_t *result = malloc(sizeof(result_t));
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
 * Process ors
 * WILL NEED TO FREE LATER
 */
static queue_t *process_ors(char **words, int word_count, bool *skip) {
	queue_t *groups = qopen();
	queue_t *gp_1 = qopen();
	if (qput(groups, gp_1) != 0) {
		return NULL;
	}
	queue_t **curr_q = &gp_1;
	for (i = 0; i < word_count; i++) {
		if (skip[i] == true) {
			continue;
		}
		else if (strcmp(words[i], "or") == 0) {
			if (i == 0 || i == word_count - 1) {
				return NULL; //or is placed bad
			}
			queue_t *new_q = qopen();
			if (qput(groups, new_q) != 0) {
				return NULL;
			}
			curr_q = &new_q;
		}
		else {
			if (qput(*curr_q, words[i]) != 0) {
				return NULL;
			}
		}
	}
	
	return groups;
}

/*
 * Process and
 * This is a singular and
 * UNFINISHED
 */
static void process_and(void *ep, void *argp) {
	queue_t *word_q = ep;
	fancy_arg_t *args = argp;
	char **words = NULL;
	char *word = NULL;
	int word_count = 0;
  int capacity = 0;
	
	while ((word = qget(word_q)) != NULL) {
		if (word_count >= capacity) {
			capacity = capacity == 0 ? 4 : 2 * capacity;
			char **new_list = realloc(words, capacity * sizeof(char *));
			if (!new_list) {
				exit(EXIT_FAILURE);
			}
			words = new_list;
		}
		
		words[word_count] = malloc(strlen(word) + 1);
		if (words[word_count] == NULL) {
			exit(EXIT_FAILURE);
		}
		strcpy(words[word_count], word);
		free(word);
		word_count++;
	}

	
	
}



/*
 * Print all docs that contain all unskipped words, highest rank first.
 */
static void print_ranked_docs(hashtable_t *index, const char *pagedir,
                              char **words, const bool *skip, int word_count) {
    entry_t **entries = malloc(word_count * sizeof(entry_t *));
    if (entries == NULL) {
        exit(EXIT_FAILURE);
    }

    int entry_count = 0;
    for (int i = 0; i < word_count; i++) {
        if (skip[i]) {
            continue;
        }
        int len = (int)strlen(words[i]) + 1;
        entry_t *entry = hsearch(index, entry_match, words[i], len);
        if (entry == NULL) {
            free(entries);
            printf("[no documents match]\n");
            return;
        }
        entries[entry_count++] = entry;
    }

    if (entry_count == 0) {
        free(entries);
        return;
    }

    queue_t *results = qopen();
    if (results == NULL) {
        exit(EXIT_FAILURE);
    }

    rank_ctx_t ctx = {entries, entry_count, results};
    qapplyargs(entries[0]->doc_queue, queue_ranked_doc, &ctx);
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

        for (int i = 0; i < word_count; i++) {
            int32_t norm_flag = normalize_word(words[i]);
            if (norm_flag == 1) {
                valid = false;
                printf("[invalid query]\n");
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
						if (strcmp(words[i], "and") == 0 ||
                       strcmp(words[i], "or") == 0) {
                skip[i] = false;
            }
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
