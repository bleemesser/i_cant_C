CFLAGS= -Wall -pedantic -std=c11 -Iinclude -I../utils/include -L../lib -g
LIBS=-lutils -lcurl
LIB=../lib/libutils.a

all: querier

.PHONY: all test valgrind clean

querier: src/querier.c $(LIB)
	gcc $(CFLAGS) src/querier.c $(LIBS) -o $@

$(LIB):
	$(MAKE) -C ../utils

test: querier
	./test.sh

valgrind: querier
	./valgrind.sh

clean:
	rm -f querier
