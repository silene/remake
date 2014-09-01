#!/bin/sh

# Check that the obsolete status is rechecked.

cat > Remakefile <<EOF
a: b
	echo a\$\$TICK > a
b: c
	if test -e b; then true; else echo b > b; fi
c:
	echo c > c
EOF

export TICK=1
$REMAKE
rm c
touch -d "2 day ago" b
touch -d "1 day ago" a
TICK=2
$REMAKE
# a should not be remade since b is unchanged
grep -q a1 a
