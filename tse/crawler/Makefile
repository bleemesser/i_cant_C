CFLAGS= -Wall -pedantic -std=c11 -Iinclude -I../utils/include -L../lib -g
LIBS=-lutils -lcurl

all: crawler

crawler: src/crawler.c ../lib/libutils.a
	gcc $(CFLAGS) src/crawler.c $(LIBS) -o $@

clean:
	rm -f crawler
