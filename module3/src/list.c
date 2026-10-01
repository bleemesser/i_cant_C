#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "list.h"

static car_t *front = NULL;

int32_t lput(car_t *cp) {
    if (cp == NULL) {
        return 1;
    }

    cp->next = front;
    front = cp;

    return 0;
}

car_t *lget() {
}

void lapply(void (*fn)(car_t *cp)) {
}

car_t *lremove(char *platep) {
    if (platep == NULL) {
        return NULL;
    }

    car_t **current = &front;

    while (*current != NULL && strcmp((*current)->plate, platep) != 0) {
        current = &(*current)->next;
    }

    car_t *found = *current;
    if (found != NULL) {
        *current = found->next;
        found->next = NULL;
    }

    return found;
}

