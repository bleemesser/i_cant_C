#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "indexio.h"

#define DELIM " \t\r\n\v\f"

int main(void) {
    const char *index_nm = "test_index";
    hashtable_t *index = index_load(index_nm);
    if (index == NULL) {
        fprintf(stderr, "querier: cannot load index '%s'\n", index_nm);
        return EXIT_FAILURE;
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
            strcpy(words[word_count], token);
            word_count++;

            token = strtok(NULL, DELIM);
        }

        bool valid = true;
        bool *skip = calloc(word_count, sizeof(bool));

        for (int i = 0; i < word_count; i++) {
            int32_t norm_flag = normalize_word(words[i]);
            if (norm_flag == 1) {
                valid = false;
                printf("[invalid query]\n");
                break;
            } else if (norm_flag == 2 || strcmp(words[i], "and") == 0 ||
                       strcmp(words[i], "or") == 0) {
                skip[i] = true;
            }
        }

        if (valid) {
            int id = 1;  // right now 1 is the only doc
            uint32_t min = UINT32_MAX;
            bool searched = false;
            for (int i = 0; i < word_count; i++) {
                if (!skip[i]) {
                    searched = true;
                    int len = (int)strlen(words[i]) + 1;
                    entry_t *entry = hsearch(index, entry_match, words[i], len);
                    if (entry == NULL) {
                        printf("%s:0 ", words[i]);
                        min = 0;
                        continue;
                    }
                    doc_t *doc = qsearch(entry->doc_queue, doc_match, &id);
                    if (doc == NULL) {
                        printf("%s:0 ", words[i]);
                        min = 0;
                        continue;
                    }
                    uint32_t count = doc->count;
                    if (count < min) {
                        min = count;
                    }

                    printf("%s:%u ", words[i], count);
                }
            }
            if (searched) {
                printf("-- %u\n", min);
            }
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
