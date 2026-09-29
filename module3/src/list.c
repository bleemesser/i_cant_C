#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <bsd/string.h>

#include "list.h"

static car_t* front = NULL;

static car_t* make_car(const char* plate, double price, int year);

int32_t lput(car_t *cp) {
    if (front != NULL) {        
    }
}

car_t *lget() {
}

void lapply(void (*fn)(car_t *cp)) {
}

car_t *lremove(char *platep) {
}

/*
 * Allocate a car with the given plate, price, and year.
 * The `next` field is initialized to NULL.
 *
 * `price` and `year` must be >= 0.
 *
 * Returns NULL if an argument is invalid, the plate does not fit in
 * MAXREG bytes, or calloc fails. Caller must free the result.
 */
static car_t* make_car(const char* plate, double price, int year) {
    if (plate == NULL || price < 0 || year < 0) {
        return NULL;
    }

    car_t* c = calloc(1, sizeof(car_t));

    if (c == NULL || strlcpy(c->plate, plate, MAXREG) >= MAXREG) {
        free(c);
        return NULL;
    }

    c->price = price;
    c->year = year;

    return c;
}
