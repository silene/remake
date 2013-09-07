Remake, a build system that bridges the gap between make and redo
=================================================================

As with <b>make</b>, <b>remake</b> uses a centralized rule file, which is
named <b>Remakefile</b>. It contains rules with a <em>make</em>-like
syntax:

	target1 target2 ... : prerequisite1 prerequisite2 ...
		shell script
		that builds
		the targets

A target is known to be up-to-date if all its prerequisites are. If it
has no known prerequisites yet the file already exits, it is assumed to
be up-to-date. Obsolete targets are rebuilt thanks to the shell script
provided by the rule.

As with <b>redo</b>, <b>remake</b> supports dynamic dependencies in
addition to these static dependencies. Whenever a script executes
<tt>remake prerequisite4 prerequisite5 ...</tt>, these prerequisites are
rebuilt if they are obsolete. (So <b>remake</b> acts like
<b>redo-ifchange</b>.) Moreover, all the dependencies are stored in file
<b>.remake</b> so that they are remembered in subsequent runs. Note that
dynamic dependencies from previous runs are only used to decide whether a
target is obsolete; they are not automatically rebuilt when they are
obsolete yet a target depends on them. They will only be rebuilt once the
dynamic call to <b>remake</b> is executed.

In other words, the following two rules have almost the same behavior.

	target1 target2 ... : prerequisite1 prerequisite2 ...
		shell script

	target1 target2 ... :
		remake prerequisite1 prerequisite2 ...
		shell script

(There is a difference if the targets already exist, have never been
built before, and the prerequisites are either younger or obsolete, since
the targets will not be rebuilt in the second case.)

The above usage of dynamic dependencies is hardly useful. Their strength
lies in the fact that they can be computed on the fly:

	%.o : %.c
		gcc -MMD -MF $@.d -o $@ -c $<
		remake -r < $@.d
		rm $@.d

	%.cmo : %.ml
		ocamldep $< | remake -r $@
		ocamlc -c $<

	after.xml: before.xml rules.xsl
		xsltproc --load-trace -o after.xml rules.xsl before.xml 2> deps
		remake `sed -n -e "\\,//,! s,^.*URL=\"\\([^\"]*\\).*\$,\\1,p" deps`
		rm deps

Note that the first rule fails if any of the header files included by
a C source file has to be automatically generated. In that case, one
should perform a first call to <b>remake</b> them before calling the
compiler. (Dependencies from several calls to <b>remake</b> are
cumulative, so they will all be remembered the next time.)

Usage
-----

Usage: <tt>remake <i>options</i> <i>targets</i></tt>

Options:

- <tt>-d</tt>: Echo script commands.
- <tt>-f FILE</tt>: Read <tt>FILE</tt> as <b>Remakefile</b>.
- <tt>-j\[N\]</tt>, <tt>--jobs=\[N\]</tt>: Allow <tt>N</tt> jobs at once;
  infinite jobs with no argument.
- <tt>-k</tt>, <tt>--keep-going</tt>: Keep going when some targets cannot be made.
- <tt>-r</tt>: Look up targets from the dependencies on standard input.
- <tt>-s</tt>, <tt>--silent</tt>, <tt>--quiet</tt>: Do not echo targets.

Syntax
------

Lines starting with a space character or a tabulation are assumed to be rule
scripts. They are only allowed after a rule header.

Lines starting with <tt>#</tt> are considered to be comments and are ignored.
They do interrupt rule scripts though.

Any other line is either a variable definition or a rule header. If such a
line ends with a backslash, the following line break is ignored and the line
extends to the next one.

Variable definitions are a single name followed by equal followed by a list
of names, possibly empty.

Rule headers are a nonempty list of names, followed by a colon, followed by
another list of names, possibly empty. Basically, the syntax of a rule is as
follows:

	targets : prerequisites
		shell script

List of names are space-separated sequences of names. If a name contains a
space character, it should be put into double quotes. Names can not be any
of the following special characters <tt>:$(),="</tt>. Again, quotation
should be used. Quotation marks can be escaped by a backslash inside
quoted names.

### Variables

Variables can be used to factor lists of targets or prerequisites. They are
expanded as they are encountered during <b>Remakefile</b> parsing.

	VAR2 = a
	VAR1 = c d
	VAR2 += $(VAR1) b
	$(VAR2) e :

Variable assignments can appear instead of prerequisites inside non-generic
rules with no script. They are then expanded inside the corresponding
generic rule.

	foo.o: CFLAGS += -DBAR

	%.o : %.c
		gcc $(CFLAGS) -MMD -MF $@.d -o $@ -c $<
		remake -r < $@.d
		rm $@.d

Note: contrarily to <b>make</b>, variable names have to be enclosed in
parentheses. For instance, <tt>$y</tt> is not a shorthand for <tt>$(y)</tt> and
is left unexpanded.

### Automatic variables

The following special symbols can appear inside scripts:

- <tt>$&lt;</tt> expands to the first static prerequisite of the rule.
- <tt>$^</tt> expands to all the static prerequisites of the rule, including
  duplicates if any.
- <tt>$@</tt> expands to the first target of the rule.
- <tt>$*</tt> expands to the string that matched <tt>%</tt> in a generic rule.
- <tt>$$</tt> expands to a single dollar symbol.

Note: contrarily to <b>make</b>, there are no corresponding variables. For
instance, <tt>$^</tt> is not a shorthand for <tt>$(^)</tt>. Another
difference is that <tt>$@</tt> is always the first target, not the one that
triggered the rule.

### Built-in functions

<b>remake</b> also supports a few built-in functions inspired from <b>make</b>.

- <tt>$(addprefix <i>prefix</i>, <i>list</i>)</tt> returns the list obtained
  by prepending its first argument to each element of its second argument.
- <tt>$(addsuffix <i>suffix</i>, <i>list</i>)</tt> returns the list obtained
  by appending its first argument to each element of its second argument.

### Order-only prerequisites

If the static prerequisites of a rule contain a pipe symbol, prerequisites
on its right do not cause the targets to become obsolete if they are newer
(unless they are also dynamically registered as dependencies). They are
meant to be used when the targets do not directly depend on them, but the
computation of their dynamic dependencies does.

	%.o : %.c | parser.h
		gcc -MMD -MF $@.d -o $@ -c $<
		remake -r < $@.d
		rm $@.d

	parser.c parser.h: parser.y
		yacc -d -o parser.c parser.y

Semantics
---------

### When are targets obsolete?

A target is obsolete:

- if there is no file corresponding to the target, or to one of its siblings
  in a multi-target rule,
- if any of its dynamic prerequisites from a previous run or any of its static
  prerequisites is obsolete,
- if the latest file corresponding to its siblings or itself is older than any
  of its dynamic prerequisites or static prerequisites.

In all the other cases, it is assumed to be up-to-date (and so are all its
siblings). Note that the last rule above says "latest" and not "earliest". While
it might cause some obsolete targets to go unnoticed in corner cases, it allows
for the following kind of rules:

	config.h stamp-config_h: config.h.in config.status
		./config.status config.h
		touch stamp-config_h

A <tt>config.status</tt> file generally does not update header files (here
<tt>config.h</tt>) if they would not change. As a consequence, if not for the
<tt>stamp-config\_h</tt> file above, a header would always be considered obsolete
once one of its prerequisites is modified. Note that touching <tt>config.h</tt>
rather than <tt>stamp-config\_h</tt> would defeat the point of not updating it
in the first place, since the program files would need to be rebuilt.

Once all the static prerequisites of a target have been rebuilt, <b>remake</b>
checks whether the target still needs to be built. If it was obsolete only
because its prerequisites needed to be rebuilt and none of them changed, the
target is assumed to be up-to-date.

### How are targets (re)built?

There are two kinds of rules. If any of the targets or prerequisites contains
a <tt>%</tt> character, the rule is said to be <em>generic</em>. All the
targets of the rule shall then contain a single <tt>%</tt> character. All the
other rules are said to be <em>specific</em>.

A rule is said to <em>match</em> a given target:

- if it is specific and the target appears inside its target list,
- if it is generic and there is a way to replace the <tt>%</tt> character
  from one of its targets so that it matches the given target.

When <b>remake</b> tries to build a given target, it looks for a specific rule
that matches it. If there is one and its script is nonempty, it uses it to
rebuild the target.

Otherwise, it looks for a generic rule that matches the target. If there are
several matching rules, it chooses the one with the shortest pattern (and if
there are several ones, the earliest one). <b>remake</b> then looks for
specific rules that match each target of the generic rule. All the
prerequisites of these specific rules are added to those of the generic rule.
The script of the generic rule is used to build the target.

Example:

	t%1 t2%: p1 p%2
		commands building t%1 and t2%
	
	t2z: p4
		commands building t2z
	
	ty1: p3
	
	# t2x is built by the first rule (which also builds tx1) and its prerequisites are p1, px2
	# t2y is built by the first rule (which also builds ty1) and its prerequisites are p1, py2, p3
	# t2z is built by the second rule and its prerequisite is p4

The set of rules from <b>Remakefile</b> is ill-formed:

- if any specific rule matching a target of the generic rule has a nonempty script,
- if any target of the generic rule is matched by a generic rule with a shorter pattern.

Compilation
-----------

- On Linux, MacOSX, and BSD: <tt>g++ -o remake remake.cpp</tt>
- On Windows: <tt>g++ -o remake.exe remake.cpp -lws2_32</tt>

Installing <b>remake</b> is needed only if <b>Remakefile</b> does not
specify the path to the executable for its recursive calls. Thanks to its
single source file, <b>remake</b> can be shipped inside other packages and
built at configuration time.

Differences with other build systems
------------------------------------

Differences with <b>make</b>:

- Dynamic dependencies are supported.
- For rules with multiple targets, the shell script is executed only once
  and is assumed to build all the targets. There is no need for
  convoluted rules that are robust enough for parallel builds. For generic
  rules, this is similar to the behavior of pattern rules from <b>gmake</b>.
- As with <b>redo</b>, only one shell is run when executing a script,
  rather than one per script line. Note that the shells are run with
  option <tt>-e</tt>, thus causing them to exit as soon as an error is
  encountered.
- The prerequisites of generic rules (known as implicit rules in make lingo)
  are not used to decide between several of them. <b>remake</b> does not
  select one for which it could satisfy the dependencies.
- Variables and built-in functions are expanded as they are encountered
  during <b>Remakefile</b> parsing.

Differences with <b>redo</b>:

- As with <b>make</b>, it is possible to write the following kind of rules
  in <b>remake</b>.

		Remakefile: Remakefile.in ./config.status
			./config.status Remakefile

- If a target is already built the first time <b>remake</b> runs, it still
  uses the static prerequisites of rules mentioning it to check whether it
  needs to be rebuilt. It does not assume it to be up-to-date. As with
  <b>redo</b> though, if its obsolete status would be due to a dynamic
  prerequisite, it will go unnoticed; it should be removed beforehand.
- Multiple targets are supported.
- <b>remake</b> has almost no features: no checksum-based dependencies, no
  compatibility with job servers, etc.

Limitations
-----------

- When the user calls <b>remake</b>, the current working directory should be
  the one containing <b>.remake</b>. Rules are understood relatively to this
  directory. If a rule script calls <b>remake</b>, the current working
  directory should be the same as the one from the original <b>remake</b>.
- Some cases of ill-formed rules are not caught by <b>remake</b> and can
  thus lead to unpredictable behaviors.

Links
-----

See <http://cr.yp.to/redo.html> for the philosophy of <b>redo</b> and
<https://github.com/apenwarr/redo> for an implementation and some comprehensive documentation.

Licensing
---------

Copyright (C) 2012-2013 by Guillaume Melquiond <guillaume.melquiond@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

