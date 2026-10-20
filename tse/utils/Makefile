CFLAGS= -Wall -pedantic -std=c11 -Iinclude -g
OFILES=queue.o hash.o webpage.o pageio.o
LIB=../lib/libutils.a

all: $(LIB)

$(LIB): $(OFILES)
	mkdir -p ../lib
	ar cr $@ $(OFILES)

test: hash_test list_test queue_test
	./hash_test
	./list_test
	./queue_test

%.o: src/%.c include/%.h
	gcc $(CFLAGS) -c $<

%_test.o: src/%_test.c
	gcc $(CFLAGS) -c $<


list_test: src/list.c include/list.h src/list_test.c
	gcc $(CFLAGS) -o list_test src/list_test.c


queue_test: src/queue.c include/queue.h src/queue_test.c
	gcc $(CFLAGS) -o queue_test src/queue_test.c


hash_test: src/hash.c src/queue.c include/hash.h include/queue.h src/hash_test.c
	gcc $(CFLAGS) -o hash_test src/hash_test.c


valgrind: test
	./valgrind.sh


clean:
	rm -f *.o hash_test list_test queue_test
