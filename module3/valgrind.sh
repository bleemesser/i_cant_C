#!/bin/bash

status=0

for t in *_test; do
	valgrind --leak-check=full ./$t > valgrind.log 2>&1
	if grep -q "ERROR SUMMARY: 0 errors" valgrind.log &&
		grep -q "All heap blocks were freed" valgrind.log; then
		echo "PASS: $t"
	else
		echo "FAIL: $t"
		status=1
	fi
done

rm -f valgrind.log
exit $status
