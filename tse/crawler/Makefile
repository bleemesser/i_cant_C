CFLAGS= -Wall -pedantic -std=c11 -Iinclude -I../utils/include -L../lib -g
LIBS=-lutils -lcurl
LIB=../lib/libutils.a

all: crawler

.PHONY: all valgrind clean

crawler: src/crawler.c $(LIB)
	gcc $(CFLAGS) src/crawler.c $(LIBS) -o $@

$(LIB):
	$(MAKE) -C ../utils

valgrind: crawler
	./valgrind.sh

clean:
	rm -f crawler
