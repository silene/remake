#!/bin/sh

# Test rules with no script, complicated version
# 1. b and c then e are started;   running: b e, waiting: (c/e/)
# 2. b calls remake, d is started; running: d e, waiting: (b/c,d/) (c/e/)
# 3. e terminates;                 running:    , waiting: (b/c,?/)
# There is a deadlock if b is not restarted there, since b was handled before c.

cat > Remakefile <<EOF
a: b c

b:
	$REMAKE c d

c: e

d:
	touch d

e:
	inotifywait -e create -q . | (test -e d && touch dd || read v)
EOF

$REMAKE -j2
