#!/bin/sh

# Check that dangling tabulations do not cause parsing to exit early

EMPTY=

cat > Remakefile <<EOF
a: b
	cat b > a
	$EMPTY
b:
	echo b > b
EOF

$REMAKE
