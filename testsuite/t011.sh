#!/bin/sh

# Test for detection of plus and equal

cat > Remakefile <<EOF
VAR+X = a b c
VAR+X += d e
VAR+X += f=g
VAR+X += h+=i
VAR+X = \$(addprefix z, \$(VAR+X))

test:
	echo \$(VAR+X) > a
	echo "za zb zc zd ze zf=g zh+=i" > b
EOF

$REMAKE
cmp a b
