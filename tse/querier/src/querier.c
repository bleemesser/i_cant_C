#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "indexio.h"

#define DELIM " \t\r\n\v\f"

int main(void) {
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

        for (int i = 0; i < word_count; i++) {
            if (normalize_word(words[i]) != 0) {
                valid = false;
                printf("[invalid query]\n");
                break;
            }
        }

        if (valid) {
            for (int i = 0; i < word_count; i++) {
                printf("%s ", words[i]);
            }
            printf("\n");
        }

        printf("> ");

        for (int i = 0; i < word_count; i++) {
            free(words[i]);
        }
        free(words);
    }

    printf("\n");

    return 0;
}
