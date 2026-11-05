#!/bin/bash

cd "$(dirname "$0")"
status=0

# expect_usage <name> <args...>
expect_usage() {
	if ./querier "${@:2}" </dev/null | grep -q "usage:"; then
		echo "PASS: $1"
	else
		echo "FAIL: $1"
		status=1
	fi
}

# expect_output <name> <queries file> <expected output file>
expect_output() {
	if ./querier tests/docs tests/index -q <"$2" | diff - "$3"; then
		echo "PASS: $1"
	else
		echo "FAIL: $1"
		status=1
	fi
}

expect_usage "no arguments"
expect_usage "too many arguments" tests/docs tests/index -q extra
expect_usage "unknown flag" tests/docs tests/index -x
expect_usage "pagedir missing" nope tests/index
expect_usage "pagedir not crawled" src tests/index
expect_usage "pagedir not accessible" /root tests/index
expect_usage "index missing" tests/docs nope
expect_usage "index not readable" tests/docs /etc/shadow

expect_output "empty input" /dev/null /dev/null
expect_output "valid queries" tests/valid.txt tests/valid.out
expect_output "invalid queries" tests/invalid.txt tests/invalid.out

exit $status
