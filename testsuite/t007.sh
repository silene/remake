#!/bin/sh

# Basic test for generic and specific rules

cat > Remakefile <<EOF
t%st: af
	cat af bf cf > /dev/null

%f:
	touch \$1

test: bf

test: cf
EOF

$REMAKE
