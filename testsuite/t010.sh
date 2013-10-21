#!/bin/sh

# Test variable passing

cat > Remakefile <<EOF
.OPTIONS = variable-propagation
VAR = 0

a:
	$REMAKE VAR=1 b
	$REMAKE e
	echo \$(VAR) > a

b:
	$REMAKE c
	echo \$(VAR) > b

c: d g

d:
	$REMAKE VAR=2 f
	echo \$(VAR) > \$@

g: VAR=3

g: h

%:
	echo \$(VAR) > \$@

EOF

$REMAKE a

# a should contain 0 (initial value)
# b should contain 1 (explicit variable passing)
# c should contain 1 (implicit variable propagation to dynamic dependencies)
# d should contain 1 (implicit variable propagation to static dependencies)
# e should contain 0 (no leak)
# f should contain 2 (explicit variable passing)
# g should contain 3 (variable setting)
# h should contain 3 (implicit variable propagation to static dependencies)

cat > z <<EOF
0
1
1
1
0
2
3
3
EOF

cat a b c d e f g h | cmp - z
