#!/bin/sh

# Check that command-line variables survive a reload

cat > Remakefile.in <<EOF
check:
	test -n "\$(VAR)"

Remakefile: Remakefile.in
	cp Remakefile.in Remakefile
EOF

cp Remakefile.in Remakefile
$REMAKE check VAR=1
$REMAKE -B check VAR=1
