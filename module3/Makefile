CFLAGS= -Wall -pedantic -std=c11 -Iinclude -g
LDLIBS= -lbsd

all: test

test: hash_test list_test queue_test

hash_test: src/hash.c include/hash.h
	gcc $(CFLAGS) -o hash_test src/hash.c src/hash_test.c


list_test: src/list.c include/list.h
	gcc $(CFLAGS) -o list_test src/list.c src/list_test.c $(LDLIBS)


queue_test: src/queue.c include/queue.h
	gcc $(CFLAGS) -o queue_test src/queue.c src/queue_test.c


clean:
	rm -f hash_test list_test queue_test
