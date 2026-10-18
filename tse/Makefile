.PHONY: all utils crawler test valgrind clean

all:
	$(MAKE) -C utils all
	$(MAKE) -C crawler

utils:
	$(MAKE) -C utils all

crawler:
	$(MAKE) -C crawler

test:
	$(MAKE) -C utils test

valgrind:
	$(MAKE) -C utils valgrind
	$(MAKE) -C crawler valgrind

clean:
	$(MAKE) -C utils clean
	$(MAKE) -C crawler clean
	rm -rf lib
