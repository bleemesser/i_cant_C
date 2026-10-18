#!/bin/bash

# NOTE: libcurl and OpenSSL keep still-reachable memory alive at exit,
# so "All heap blocks were freed" is not
# achievable here. Instead, require zero errors and zero definite/possible loss.

status=0

t=crawler
	valgrind --leak-check=full --show-leak-kinds=definite,possible ./$t > valgrind.log 2>&1
	if grep -q "ERROR SUMMARY: 0 errors" valgrind.log &&
		grep -q "definitely lost: 0 bytes in 0 blocks" valgrind.log &&
		grep -q "indirectly lost: 0 bytes in 0 blocks" valgrind.log &&
		grep -q "possibly lost: 0 bytes in 0 blocks" valgrind.log; then
		echo "VALGRIND PASS: $t"
	else
		echo "VALGRIND FAIL: $t"
		status=1
	fi

rm -f valgrind.log
exit $status
