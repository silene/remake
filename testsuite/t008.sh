#!/bin/sh

# Test rules with no script

cat > Remakefile <<EOF
test2: test
test:
EOF

$REMAKE
