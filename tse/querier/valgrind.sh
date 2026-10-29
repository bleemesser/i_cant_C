#!/bin/bash

status=0

cd "$(dirname "$0")/.." # must run from tse/ with pages/ dir existing

valgrind --leak-check=full ./querier/querier > valgrind.log 2>&1

if grep -q "ERROR SUMMARY: 0 errors" valgrind.log &&
	grep -q "All heap blocks were freed" valgrind.log; then
	echo "VALGRIND PASS: querier"
else
	echo "VALGRIND FAIL: querier"
	status=1
fi

rm -f valgrind.log
exit $status
