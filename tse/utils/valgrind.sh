#!/bin/bash

status=0

for t in *_test; do
	if [ "$t" = pageio_test ]; then
		valgrind --leak-check=full --show-leak-kinds=definite,possible,indirect \
			./$t > valgrind.log 2>&1
		ok=$(grep -q "ERROR SUMMARY: 0 errors" valgrind.log &&
			grep -q "definitely lost: 0 bytes in 0 blocks" valgrind.log &&
			grep -q "indirectly lost: 0 bytes in 0 blocks" valgrind.log &&
			grep -q "possibly lost: 0 bytes in 0 blocks" valgrind.log && echo yes)
	else
		valgrind --leak-check=full ./$t > valgrind.log 2>&1
		ok=$(grep -q "ERROR SUMMARY: 0 errors" valgrind.log &&
			grep -q "All heap blocks were freed" valgrind.log && echo yes)
	fi
	if [ "$ok" = yes ]; then
		echo "VALGRIND PASS: $t"
	else
		echo "VALGRIND FAIL: $t"
		status=1
	fi
done

rm -f valgrind.log
exit $status
