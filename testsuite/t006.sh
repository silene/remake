#!/bin/sh

# Basic test for variable assignments and function calls

cat > Remakefile <<EOF
VAR = b c
VAR += d e
VAR = \$(addprefix z, \$(VAR))

test:
	echo \$(VAR) > a
	echo "zb zc zd ze" > b
EOF

$REMAKE
cmp a b
