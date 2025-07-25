#! /bin/sh

rm -rf sandbox

export REMAKE="$PWD/../remake -s"
unset REMAKE_SOCKET

if [ "x$1" = x ]; then TESTS=t*.sh; else TESTS=$1; fi

EXITCODE=0

for f in $TESTS; do
	mkdir sandbox
	if ! (cd sandbox ; $SHELL -e ../$f); then
		echo "** Failure: $f"
		EXITCODE=1
	fi
	rm -rf sandbox
done

exit $EXITCODE
