Remake, a build system that bridges the gap between make and redo.
==========================================================

As with <b>make</b>, <b>remake</b> uses a centralized rule file, which is
named <b>Remakefile</b>. It contains rules with a <em>make</em>-like
syntax:

<pre>
target1 target2 ... : dependency1 dependency2 ...
	shell script
	that builds
	the targets
</pre>

A target is known to be up-to-date if all its dependencies are. If it
has no known dependencies yet the file already exits, it is assumed to
be up-to-date. Obsolete targets are rebuilt thanks to the shell script
provided by the rule.

As with <b>redo</b>, <b>remake</b> supports dynamic dependencies in
addition to these static dependencies. Whenever a script executes
<tt>remake dependency4 dependency5 ...</tt>, these dependencies are
rebuilt if they are obsolete. (So <b>remake</b> acts like
<b>redo-ifchange</b>.) Moreover, these dependencies are stored in file
<b>.remake</b> so that they are remembered in subsequent runs. Note that
dynamic dependencies from previous runs are only used to decide whether a
target is obsolete; they are not automatically rebuilt when they are
obsolete yet a target depends on them. They will only be rebuilt once the
dynamic call to <b>remake</b> is executed.

In other words, the following two rules have almost the same behavior.

<pre>
target1 target2 ... : dependency1 dependency2 ...
	shell script

target1 target2 ... :
	remake dependency1 dependency2 ...
	shell script
</pre>

(There is a difference if the targets already exist, have never been
built before, and the dependencies are either younger or obsolete, since
the targets will not be rebuilt in the second case.)

The above usage of dynamic dependencies is hardly useful. Their strength
lies in the fact that they can be computed on the fly:

	%.o : %.c
		gcc -MMD -MF $1.d -o $1 -c ${1%.o}.c
		read DEPS < $1.d
		remake ${DEPS#*:}
		rm $1.d

	%.cmo : %.ml
		remake $(ocamldep ${1%.cmo}.ml | sed -n -e "\\,^.*: *\$, b; \\,$1:, { b feed2; :feed1 N; :feed2 s/[\\]\$//; t feed1; s/.*://; s/[ \\t\\r\\n]*\\([ \\t\\r\\n]\\+\\)/\\1\n/g; s/\\n\$//; p; q}")
		ocamlc -c ${1%.cmo}.ml

Note that the first rule fails if any of the header files included by
a C source file has to be automatically generated. In that case, one
should perform a first call to <b>remake</b> them before calling the
compiler. (Dependencies from several calls to <b>remake</b> are
cumulative, so they will all be remembered the next time.)

Options:

- <tt>-j\[N\]</tt>, <tt>--jobs=\[N\]</tt>: Allow N jobs at once; infinite jobs
  with no argument.

Other differences with <b>make</b>:

- For rules with multiple targets, the shell script is executed only once
  and is assumed to build all the targets. There is no need for
  convoluted rules that are robust enough for parallel builds.
- As with <b>redo</b>, only one shell is run when executing a script,
  rather than one per script line. Note that the shells are run with
  option <tt>-e</tt>, thus causing them to exit as soon as an error is
  encountered.
- The dependencies of generic rules (known as implicit rules in make lingo)
  are not used to decide between several of them. <b>remake</b> does not
  select one for which it could satisfy the dependencies.
- <b>remake</b> has almost no features: no variables, no predefined
  functions, etc.

Other differences with <b>redo</b>:

- As with <b>make</b>, it is possible to write the following kind of rules
  in <b>remake</b>.

		Remakefile: Remakefile.in ./config.status
			./config.status Remakefile

- <b>remake</b> has almost no features: no checksum-based dependencies, no
  compatibility with token servers, etc.

Other differences with <b>make</b> and <b>redo</b>:

- When executing shell scripts, positional variables <tt>$1</tt>,
  <tt>$2</tt>, etc, point to the target names of the rule obtained after
  substituting <tt>%</tt>. No other variables are defined.

Limitations:

- When the user or a script calls <b>remake</b>, the current working
  directory should be the one containing <b>Remakefile</b> (and thus
  <b>.remake</b> too). This is unavoidable for user calls, but could be
  improved for recursive calls.
- Target names are not yet normalized, so <tt>f</tt> and <tt>d/../f</tt>
  are two different targets.

See <http://cr.yp.to/redo.html> for the philosophy of <b>redo</b> and
<https://github.com/apenwarr/redo> for an implementation and some comprehensive documentation.

Copyright (C) 2012 by Guillaume Melquiond <guillaume.melquiond@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

