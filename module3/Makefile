CFLAGS= -Wall -pedantic -std=c11 -Iinclude -g

all: test

test: hash_test list_test queue_test

hash_test: src/hash.c include/hash.h src/hash_test.c
	gcc $(CFLAGS) -o hash_test src/hash.c src/hash_test.c


list_test: src/list.c include/list.h src/list_test.c
	gcc $(CFLAGS) -o list_test src/list_test.c


queue_test: src/queue.c include/queue.h src/queue_test.c
	gcc $(CFLAGS) -o queue_test src/queue.c src/queue_test.c


valgrind: list_test
	./valgrind.sh


clean:
	rm -f hash_test list_test queue_test
