CFLAGS= -Wall -pedantic -std=c11 -Iinclude -I../utils/include -L../lib -g
LIBS=-lutils -lcurl
LIB=../lib/libutils.a

all: querier

.PHONY: all valgrind clean

querier: src/querier.c $(LIB)
	gcc $(CFLAGS) src/querier.c $(LIBS) -o $@

$(LIB):
	$(MAKE) -C ../utils

valgrind: querier
	./valgrind.sh

clean:
	rm -f querier
