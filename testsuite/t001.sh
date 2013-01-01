#!/bin/sh

# Basic test for static prerequisites

cat > Remakefile <<EOF
a: b c
	cat b c > a
b:
	echo b > b
c:
	echo c > c
EOF

$REMAKE
cp a d
cat /dev/null > a
rm b
$REMAKE
cmp a d
