#!/bin/sh

# Basic test for static pattern rules

cat > Remakefile <<"EOF"
all: test tist
     cat $^ > $@

tist tast test: t%st: u%su u%st
     cat $^ > $@

uesu uisu uest uist: u%:
     echo $* > $@
EOF

$REMAKE

cmp all <<EOF
esu
est
isu
ist
EOF
