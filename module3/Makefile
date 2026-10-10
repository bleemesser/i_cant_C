CFLAGS= -Wall -pedantic -std=c11 -Iinclude -g

all: test

test: hash_test list_test queue_test

%.o: src/%.c include/%.h
	gcc $(CFLAGS) -c $<

%_test.o: src/%_test.c
	gcc $(CFLAGS) -c $<


# list_test.c includes list.c directly, so list.o must not be linked
list_test: src/list.c include/list.h src/list_test.c
	gcc $(CFLAGS) -o list_test src/list_test.c


queue_test: src/queue.c include/queue.h src/queue_test.c
	gcc $(CFLAGS) -o queue_test src/queue_test.c


hash_test: hash.o queue.o hash_test.o
	gcc $(CFLAGS) $^ -o $@


valgrind: list_test queue_test
	./valgrind.sh


clean:
	rm -f *.o hash_test list_test queue_test
