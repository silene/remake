#!/bin/sh

# Check that failed files are not removed if they were left unchanged.

cat > Remakefile <<EOF
a: b c
	touch a

b c:
	touch b
	touch c
	exit 1
EOF

touch -d "1 day ago" a
touch -d "1 day ago" b
! $REMAKE 2> /dev/null
test -f a -a '!' -f b -a '!' -f c
