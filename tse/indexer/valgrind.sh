#!/bin/bash

status=0

cd "$(dirname "$0")/.." # must run from tse/ with pages/ dir existing

valgrind --leak-check=full ./indexer/indexer pages /dev/null > valgrind.log 2>&1

if grep -q "ERROR SUMMARY: 0 errors" valgrind.log &&
	grep -q "All heap blocks were freed" valgrind.log; then
	echo "VALGRIND PASS: indexer"
else
	echo "VALGRIND FAIL: indexer"
	status=1
fi

rm -f valgrind.log
exit $status
