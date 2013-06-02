#!/bin/sh

# Basic test for generic and specific rules

cat > Remakefile <<EOF
VAR = bad

t%st: af
	test \$(VAR) = ok
	cat af bf cf > /dev/null

%f:
	touch \$\$1

test: bf

test: cf

test: VAR = ok
EOF

$REMAKE
