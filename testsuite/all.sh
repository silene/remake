#! /bin/sh

rm -rf sandbox

export REMAKE="$PWD/../remake -s"
unset REMAKE_SOCKET

if [ "x$1" = x ]; then TESTS=t*.sh; else TESTS=$1; fi

for f in $TESTS; do
	mkdir sandbox
	(cd sandbox ; $SHELL -e ../$f) || echo "** Failure: $f"
	rm -rf sandbox
done
