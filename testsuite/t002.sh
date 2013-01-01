#!/bin/sh

# Basic test for dynamic prerequisites

cat > Remakefile <<EOF
a:
	$REMAKE b c
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
