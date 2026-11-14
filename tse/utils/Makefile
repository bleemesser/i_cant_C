
.PHONY: all test valgrind clean

CFLAGS= -Wall -pedantic -std=c11 -Iinclude -g
OFILES=queue.o lqueue.o hash.o webpage.o pageio.o indexio.o lhash.o
LIB=../lib/libutils.a

all: $(LIB)

$(LIB): $(OFILES)
	mkdir -p ../lib
	ar cr $@ $(OFILES)

test: hash_test list_test queue_test lqueue_test pageio_test indexio_test lhash_test
	./hash_test
	./list_test
	./queue_test
	./lqueue_test
	./pageio_test
	./indexio_test
	./lhash_test

%.o: src/%.c include/%.h
	gcc $(CFLAGS) -c $<

%_test.o: src/%_test.c
	gcc $(CFLAGS) -c $<


list_test: src/list.c include/list.h src/list_test.c
	gcc $(CFLAGS) -o list_test src/list_test.c


queue_test: src/queue.c include/queue.h src/queue_test.c
	gcc $(CFLAGS) -o queue_test src/queue_test.c


lqueue_test: src/lqueue.c src/queue.c include/lqueue.h include/queue.h src/lqueue_test.c
	gcc $(CFLAGS) -pthread -o lqueue_test src/lqueue_test.c src/lqueue.c src/queue.c


hash_test: src/hash.c src/queue.c include/hash.h include/queue.h src/hash_test.c
	gcc $(CFLAGS) -o hash_test src/hash_test.c

pageio_test: src/pageio.c src/webpage.c include/pageio.h include/webpage.h src/pageio_test.c
	gcc $(CFLAGS) -o pageio_test src/pageio_test.c src/pageio.c src/webpage.c -lcurl

indexio_test: src/indexio.c src/hash.c src/queue.c src/pageio.c src/webpage.c include/indexio.h src/indexio_test.c
	gcc $(CFLAGS) -o indexio_test src/indexio_test.c src/indexio.c src/hash.c src/queue.c src/pageio.c src/webpage.c -lcurl

lhash_test: src/lhash.c src/hash.c src/queue.c include/lhash.h include/hash.h include/queue.h src/lhash_test.c
	gcc $(CFLAGS) -o lhash_test src/lhash_test.c src/lhash.c src/hash.c src/queue.c -pthread

valgrind: test
	./valgrind.sh


clean:
	rm -f *.o hash_test list_test queue_test lqueue_test pageio_test indexio_test lhash_test
