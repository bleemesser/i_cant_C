/*
 * list.c -- implementation of the functions declared in list.h
 */

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

car_t *lget(void) {
    if (front == NULL) {
        return NULL;
    }

    car_t *first = front;
    front = front->next;

    return first;
}

void lapply(void (*fn)(car_t *cp)) {
    // note: header does not provide a way to signal failure if fn==NULL
    // or if fn has a failure of some kind :(

    car_t *current = front;
    while (current != NULL) {
        fn(current);
        current = current->next;
    }
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
