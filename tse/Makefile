all: library crawler

utils:
	mkdir -p lib
	$(MAKE) -C utils all

crawler: utils
	$(MAKE) -C crawler

test:
	$(MAKE) -C utils test

valgrind:
	$(MAKE) -C utils valgrind

clean:
	$(MAKE) -C utils clean
	$(MAKE) -C crawler clean
	rm -rf lib
