#!/bin/sh

# Check that if one of the targets of a multi-rule is obsolete, all of them are.

cat > Remakefile <<EOF
a: b c
	cat b c > a
b: d
	echo b > b
	cat d >> b
c d:
	echo c > c
	echo d\$TAG > d
EOF

export TAG=1
$REMAKE
rm c
TAG=2
$REMAKE
# TAG emulates time; if b contains "d1", then b is obsolete since d contains "d2"
grep -q d2 b
