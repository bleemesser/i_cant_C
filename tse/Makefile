.PHONY: all utils crawler test valgrind clean

all:
	$(MAKE) -C utils all
	$(MAKE) -C crawler
	$(MAKE) -C indexer
	$(MAKE) -C querier

utils:
	$(MAKE) -C utils all

crawler:
	$(MAKE) -C crawler

indexer:
	$(MAKE) -C indexer

querier:
	$(MAKE) -C querier

test:
	$(MAKE) -C utils test

valgrind:
	$(MAKE) -C utils valgrind
	$(MAKE) -C crawler valgrind
	$(MAKE) -C indexer valgrind
	# todo: querier valgrind

clean:
	$(MAKE) -C utils clean
	$(MAKE) -C crawler clean
	$(MAKE) -C indexer clean
	$(MAKE) -C querier clean
	rm -rf lib
