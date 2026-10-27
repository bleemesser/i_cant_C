#!/bin/bash

# NOTE: libcurl and OpenSSL keep still-reachable memory alive at exit,
# so "All heap blocks were freed" is not achievable here.
# Instead, require zero errors and zero definite/possible loss.

status=0

cd "$(dirname "$0")/.." && mkdir -p pages # must run from tse/ with pages/ dir existing

valgrind --leak-check=full --show-leak-kinds=definite,possible \
	./crawler/crawler https://thayer.github.io/engs50/ pages 1 > valgrind.log 2>&1

if grep -q "ERROR SUMMARY: 0 errors" valgrind.log &&
	grep -qE "All heap blocks were freed|definitely lost: 0 bytes in 0 blocks" valgrind.log &&
	! grep -qE "(definitely|indirectly|possibly) lost: [1-9]" valgrind.log; then
	echo "VALGRIND PASS: crawler"
else
	echo "VALGRIND FAIL: crawler"
	status=1
fi

rm -f valgrind.log
exit $status
