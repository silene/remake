#!/bin/sh

# Check that an empty rule does not lump all its targets together.

cat > Remakefile <<EOF
%:
	echo \$1\$TICK > \$1
a b: c
EOF

export TICK=1
$REMAKE a b
rm a
TICK=2
$REMAKE b
# if b contains "b2", then b was updated while already up-to-date
grep -q b1 b
