CFLAGS= -Wall -pedantic -std=c11 -Iinclude -I../utils/include -L../lib -g
LIBS=-lutils -lcurl
LIB=../lib/libutils.a

all: indexer

.PHONY: all valgrind clean

indexer: src/indexer.c $(LIB)
	gcc $(CFLAGS) src/indexer.c $(LIBS) -o $@

$(LIB):
	$(MAKE) -C ../utils

valgrind: indexer
	./valgrind.sh

clean:
	rm -f indexer
