.PHONY: all utils crawler test valgrind clean

all:
	$(MAKE) -C utils all
	$(MAKE) -C crawler
	$(MAKE) -C indexer

utils:
	$(MAKE) -C utils all

crawler:
	$(MAKE) -C crawler

indexer:
	$(MAKE) -C indexer

test:
	$(MAKE) -C utils test

valgrind:
	$(MAKE) -C utils valgrind
	$(MAKE) -C crawler valgrind
	$(MAKE) -C indexer valgrind

clean:
	$(MAKE) -C utils clean
	$(MAKE) -C crawler clean
	$(MAKE) -C indexer clean
	rm -rf lib
