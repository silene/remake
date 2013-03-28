/**
@mainpage Remake, a build system that bridges the gap between make and redo.

As with <b>make</b>, <b>remake</b> uses a centralized rule file, which is
named <b>Remakefile</b>. It contains rules with a <em>make</em>-like
syntax:

@verbatim
target1 target2 ... : dependency1 dependency2 ...
	shell script
	that builds
	the targets
@endverbatim

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

@verbatim
target1 target2 ... : dependency1 dependency2 ...
	shell script

target1 target2 ... :
	remake dependency1 dependency2 ...
	shell script
@endverbatim

(There is a difference if the targets already exist, have never been
built before, and the dependencies are either younger or obsolete, since
the targets will not be rebuilt in the second case.)

The above usage of dynamic dependencies is hardly useful. Their strength
lies in the fact that they can be computed on the fly:

@verbatim
%.o : %.c
	gcc -MMD -MF $1.d -o $1 -c ${1%.o}.c
	cat $1.d | remake -r
	rm $1.d

%.cmo : %.ml
	ocamldep ${1%.cmo}.ml | remake -r $1
	ocamlc -c ${1%.cmo}.ml

after.xml: before.xml rules.xsl
	xsltproc --load-trace -o after.xml rules.xsl before.xml 2> deps
	remake $(sed -n -e "\\,//,! s,^.*URL=\"\\([^\"]*\\).*\$,\\1,p" deps)
	rm deps
@endverbatim

Note that the first rule fails if any of the header files included by
a C source file has to be automatically generated. In that case, one
should perform a first call to <b>remake</b> them before calling the
compiler. (Dependencies from several calls to <b>remake</b> are
cumulative, so they will all be remembered the next time.)

\section sec-usage Usage

Usage: <tt>remake <i>options</i> <i>targets</i></tt>

Options:

- <tt>-d</tt>: Echo script commands.
- <tt>-j[N]</tt>, <tt>--jobs=[N]</tt>: Allow N jobs at once; infinite jobs
  with no argument.
- <tt>-k</tt>, <tt>--keep-going</tt>: Keep going when some targets cannot be made.
- <tt>-r</tt>: Look up targets from the dependencies on standard input.
- <tt>-s</tt>, <tt>--silent</tt>, <tt>--quiet</tt>: Do not echo targets.

\section sec-syntax Syntax

Lines starting with a space character or a tabulation are assumed to be rule
scripts. They are only allowed after a rule header.

Lines starting with <tt>#</tt> are considered to be comments and are ignored.
They do interrupt rule scripts though.

Any other line is either a rule header or a variable definition. If such a
line ends with a backslash, the following line break is ignored and the line
extends to the next one.

Rule headers are a nonempty list of names, followed by a colon, followed by
another list of names, possibly empty. Variable definitions are a single
name followed by equal followed by a list of names, possibly empty. Basically,
the syntax of a rule is as follows:

@verbatim
targets : prerequisites
	shell script
@endverbatim

List of names are space-separated sequences of names. If a name contains a
space character, it should be put into double quotes. Names can not be any
of the following special characters <tt>:$(),="</tt>. Again, quotation
should be used. Quotation marks can be escaped by a backslash inside
quoted names.

\subsection sec-variables Variables

Variables can be used to factor lists of targets or dependencies. They are
expanded as they are encountered during <b>Remakefile</b> parsing.

@verbatim
VAR1 = c d
VAR2 = a $(VAR1) b
$(VAR2) e :
@endverbatim

Variables can be used inside rule scripts; they are available as non-exported
shell variables there.

\subsection sec-functions Built-in functions

<b>remake</b> also supports a few built-in functions inspired from <b>make</b>.

- <tt>$(addprefix <i>prefix</i>, <i>list</i>)</tt> returns the list obtained
  by prepending its first argument to each element of its second argument.
- <tt>$(addsuffix <i>suffix</i>, <i>list</i>)</tt> returns the list obtained
  by appending its first argument to each element of its second argument.

Note that functions are ignored inside scripts.

\section sec-semantics Semantics

\subsection src-obsolete When are targets obsolete?

A target is obsolete:

- if there is no file corresponding to the target, or to one of its siblings
  in a multi-target rule,
- if any of its dynamic dependencies from a previous run or any of its static
  prerequisites is obsolete,
- if the latest file corresponding to its siblings or itself is older than any
  of its dynamic dependencies or static prerequisites.

In all the other cases, it is assumed to be up-to-date (and so are all its
siblings). Note that the last rule above says "latest" and not "earliest". While
it might cause some obsolete targets to go unnoticed in corner cases, it allows
for the following kind of rules:

@verbatim
config.h stamp-config_h: config.h.in config.status
	./config.status config.h
	touch stamp-config_h
@endverbatim

A <tt>config.status</tt> file generally does not update header files (here
<tt>config.h</tt>) if they would not change. As a consequence, if not for the
<tt>stamp-config_h</tt> file above, a header would always be considered obsolete
once one of its prerequisites is modified. Note that touching <tt>config.h</tt>
rather than <tt>stamp-config_h</tt> would defeat the point of not updating it
in the first place, since the program files would need to be rebuilt.

Once all the static prerequisites of a target have been rebuilt, <b>remake</b>
checks if the target still needs to be built. If it was obsolete only because
its dependencies needed to be rebuilt and none of them changed, the target is
assumed to be up-to-date.

\subsection sec-rules How are targets (re)built?

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

Otherwise, it looks for a generic rule that match the target. If there are
several matching rules, it chooses the one with the shortest pattern (and if
there are several ones, the earliest one). <b>remake</b> then looks for
specific rules that match each target of the generic rule. All the
prerequisites of these specific rules are added to those of the generic rule.
The script of the generic rule is used to build the target.

Example:

@verbatim
t%1 t2%: p1 p%2
	commands building t%1 and t2%

t2z: p4
	commands building t2z

ty1: p3

# t2x is built by the first rule (which also builds tx1) and its prerequisites are p1, px2
# t2y is built by the first rule (which also builds ty1) and its prerequisites are p1, py2, p3
# t2z is built by the second rule and its prerequisite is p4
@endverbatim

The set of rules from <b>Remakefile</b> is ill-formed:

- if any specific rule matching a target of the generic rule has a nonempty script,
- if any target of the generic rule is matched by a generic rule with a shorter pattern.

\section sec-compilation Compilation

- On Linux: <tt>g++ -o remake remake.cpp</tt>
- On Windows: <tt>g++ -o remake.exe remake.cpp -lws2_32</tt>

Installing <b>remake</b> is needed only if <b>Remakefile</b> does not
specify the path to the executable for its recursive calls. Thanks to its
single source file, <b>remake</b> can be shipped inside other packages and
built at configuration time.

\section sec-differences Differences with other build systems

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
- The dependencies of generic rules (known as implicit rules in make lingo)
  are not used to decide between several of them. <b>remake</b> does not
  select one for which it could satisfy the dependencies.
- Variables and built-in functions are expanded as they are encountered
  during <b>Remakefile</b> parsing.

Differences with <b>redo</b>:

- As with <b>make</b>, it is possible to write the following kind of rules
  in <b>remake</b>.
@verbatim
Remakefile: Remakefile.in ./config.status
	./config.status Remakefile
@endverbatim
- If a target is already built the first time <b>remake</b> runs, it still
  uses the static prerequisites of rules mentioning it to check whether it
  needs to be rebuilt. It does not assume it to be up-to-date. As with
  <b>redo</b> though, if its obsolete status would be due to a dynamic
  dependency, it will go unnoticed; it should be removed beforehand.
- <b>remake</b> has almost no features: no checksum-based dependencies, no
  compatibility with token servers, etc.

Differences with both <b>make</b> and <b>redo</b>:

- Multiple targets are supported.
- When executing shell scripts, positional variables <tt>$1</tt>,
  <tt>$2</tt>, etc, point to the target names of the rule obtained after
  substituting <tt>%</tt>. No other variables are defined.

\section sec-limitations Limitations

- When the user or a script calls <b>remake</b>, the current working
  directory should be the one containing <b>Remakefile</b> (and thus
  <b>.remake</b> too).
- Some cases of ill-formed rules are not caught by <b>remake</b> and can
  thus lead to unpredictable behaviors.

\section sec-links Links

@see http://cr.yp.to/redo.html for the philosophy of <b>redo</b> and
https://github.com/apenwarr/redo for an implementation and some comprehensive documentation.

\section sec-licensing Licensing

@author Guillaume Melquiond
@version 0.2
@date 2012-2013
@copyright
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
\n
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define WINDOWS
#endif

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __APPLE__
#define MACOSX
#endif

#ifdef WINDOWS
#include <windows.h>
#include <winbase.h>
#include <winsock2.h>
#define pid_t HANDLE
typedef SOCKET socket_t;
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
typedef int socket_t;
enum { INVALID_SOCKET = -1 };
#endif

#if defined(WINDOWS) || defined(MACOSX)
enum { MSG_NOSIGNAL = 0 };
#endif

typedef std::list<std::string> string_list;

typedef std::set<std::string> string_set;

/**
 * Reference-counted shared object.
 * @note The default constructor delays the creation of the object until it
 *       is first deferenced.
 */
template<class T>
struct ref_ptr
{
	struct content
	{
		size_t cnt;
		T val;
		content(): cnt(1) {}
		content(T const &t): cnt(1), val(t) {}
	};
	mutable content *ptr;
	ref_ptr(): ptr(NULL) {}
	ref_ptr(T const &t): ptr(new content(t)) {}
	ref_ptr(ref_ptr const &p): ptr(p.ptr) { if (ptr) ++ptr->cnt; }
	~ref_ptr() { if (ptr && --ptr->cnt == 0) delete ptr; }
	ref_ptr &operator=(ref_ptr const &p)
	{
		if (ptr == p.ptr) return *this;
		if (ptr && --ptr->cnt == 0) delete ptr;
		ptr = p.ptr;
		if (ptr) ++ptr->cnt;
		return *this;
	}
	T &operator*() const
	{
		if (!ptr) ptr = new content;
		return ptr->val;
	}
	T *operator->() const { return &**this; }
};

struct dependency_t
{
	string_list targets;
	string_set deps;
};

typedef std::map<std::string, ref_ptr<dependency_t> > dependency_map;

typedef std::map<std::string, string_list> variable_map;

/**
 * Build status of a target.
 */
enum status_e
{
	Uptodate, ///< Target is up-to-date.
	Todo,     ///< Target is missing or obsolete.
	Recheck,  ///< Target has an obsolete dependency.
	Running,  ///< Target is being rebuilt.
	Remade,   ///< Target was successfully rebuilt.
	Failed    ///< Build failed for target.
};

/**
 * Build status of a target.
 */
struct status_t
{
	status_e status; ///< Actual status.
	time_t last;     ///< Last-modified date.
};

typedef std::map<std::string, status_t> status_map;

/**
 * A rule loaded from Remakefile.
 */
struct rule_t
{
	string_list targets; ///< Files produced by this rule.
	string_list deps;    ///< Files used for an implicit call to remake at the start of the script.
	std::string script;  ///< Shell script for building the targets.
};

typedef std::list<rule_t> rule_list;

typedef std::map<std::string, ref_ptr<rule_t> > rule_map;

typedef std::map<int, string_list> job_targets_map;

typedef std::map<pid_t, int> pid_job_map;

/**
 * Client waiting for a request complete.
 *
 * There are two kinds of clients:
 * - real clients, which are instances of remake created by built scripts,
 * - pseudo clients, which are created by the server to build specific targets.
 *
 * Among pseudo clients, there are two categories:
 * - original clients, which are created for the targets passed on the
 *   command line by the user or for the initial regeneration of the rule file,
 * - dependency clients, which are created to handle rules that have
 *   explicit dependencies and thus to emulate a call to remake.
 */
struct client_t
{
	socket_t socket;     ///< Socket used to reply to the client (invalid for pseudo clients).
	int job_id;          ///< Job for which the built script called remake and spawned the client (negative for original clients).
	bool failed;         ///< Whether some targets failed in mode -k.
	string_list pending; ///< Targets not yet started.
	string_set running;  ///< Targets being built.
	rule_t *delayed;     ///< Rule that implicitly created a dependency client, and which script has to be started on request completion.
	client_t(): socket(INVALID_SOCKET), job_id(-1), failed(false), delayed(NULL) {}
};

typedef std::list<client_t> client_list;

/**
 * Map from variable names to their content.
 */
static variable_map variables;

/**
 * Precomputed variable assignments for shell usage.
 */
static std::string variable_block;

/**
 * Map from targets to their known dependencies.
 */
static dependency_map dependencies;

/**
 * Map from targets to their build status.
 */
static status_map status;

/**
 * Set of generic rules loaded from Remakefile.
 */
static rule_list generic_rules;

/**
 * Map from targets to specific rules loaded from Remakefile.
 */
static rule_map specific_rules;

/**
 * Map from jobs to targets being built.
 */
static job_targets_map job_targets;

/**
 * Map from jobs to shell pids.
 */
static pid_job_map job_pids;

/**
 * List of clients waiting for a request to complete.
 * New clients are put to front, so that the build process is depth-first.
 */
static client_list clients;

/**
 * Maximum number of parallel jobs (non-positive if unbounded).
 * Can be modified by the -j option.
 */
static int max_active_jobs = 1;

/**
 * Whether to keep building targets in case of failure.
 * Can be modified by the -k option.
 */
static bool keep_going = false;

/**
 * Number of jobs currently running:
 * - it increases when a process is created in #run_script,
 * - it decreases when a completion message is received in #finalize_job.
 *
 * @note There might be some jobs running while #clients is empty.
 *       Indeed, if a client requested two targets to be rebuilt, if they
 *       are running concurrently, if one of them fails, the client will
 *       get a failure notice and might terminate before the other target
 *       finishes.
 */
static int running_jobs = 0;

/**
 * Number of jobs currently waiting for a build request to finish:
 * - it increases when a build request is received in #accept_client
 *   (since the client is presumably waiting for the reply),
 * - it decreases when a reply is sent in #complete_request.
 */
static int waiting_jobs = 0;

/**
 * Global counter used to produce increasing job numbers.
 * @see job_targets
 */
static int job_counter = 0;

/**
 * Socket on which the server listens for client request.
 */
static socket_t socket_fd;

/**
 * Whether the request of an original client failed.
 */
static bool build_failure;

/**
 * Name of the server socket in the file system.
 */
static char *socket_name;

/**
 * Name of the first target of the first specific rule, used for default run.
 */
static std::string first_target;

/**
 * Whether a short message should be displayed for each target.
 */
static bool show_targets = true;

/**
 * Whether script commands are echoed.
 */
static bool echo_scripts = false;

static time_t now = time(NULL);

static std::string working_dir;

#ifndef WINDOWS
static volatile sig_atomic_t got_SIGCHLD = 0;

static void child_sig_handler(int)
{
	got_SIGCHLD = 1;
}
#endif

struct log
{
	bool active, open;
	int depth;
	log(): active(false), open(false), depth(0)
	{
	}
	std::ostream &operator()()
	{
		if (open) std::cerr << std::endl;
		assert(depth >= 0);
		std::cerr << std::string(depth * 2, ' ');
		open = false;
		return std::cerr;
	}
	std::ostream &operator()(bool o)
	{
		if (o && open) std::cerr << std::endl;
		if (!o) --depth;
		assert(depth >= 0);
		if (o || !open) std::cerr << std::string(depth * 2, ' ');
		if (o) ++depth;
		open = o;
		return std::cerr;
	}
};

log debug;

struct log_auto_close
{
	bool still_open;
	log_auto_close(): still_open(true)
	{
	}
	~log_auto_close()
	{
		if (debug.active && still_open) debug(false) << "done\n";
	}
};

#define DEBUG if (debug.active) debug()
#define DEBUG_open log_auto_close auto_close; if (debug.active) debug(true)
#define DEBUG_close if ((auto_close.still_open = false), debug.active) debug(false)

/**
 * Return the original string if it does not contain any special characters,
 * a quoted and escaped string otherwise.
 */
static std::string escape_string(std::string const &s)
{
	char const *quoted_char = ",: '";
	char const *escaped_char = "\"\\$!";
	bool need_quotes = false;
	size_t len = s.length(), nb = len;
	for (size_t i = 0; i < len; ++i)
	{
		if (strchr(quoted_char, s[i])) need_quotes = true;
		if (strchr(escaped_char, s[i])) ++nb;
	}
	if (nb != len) need_quotes = true;
	if (!need_quotes) return s;
	std::string t(nb + 2, '\\');
	t[0] = '"';
	for (size_t i = 0, j = 1; i < len; ++i, ++j)
	{
		if (strchr(escaped_char, s[i])) ++j;
		t[j] = s[i];
	}
	t[nb + 1] = '"';
	return t;
}

/**
 * Initialize #working_dir.
 */
void init_working_dir()
{
	char buf[1024];
	char *res = getcwd(buf, sizeof(buf));
	if (!res)
	{
		perror("Failed to get working directory");
		exit(EXIT_FAILURE);
	}
	working_dir = buf;
}

/**
 * Normalize an absolute path with respect to the working directory.
 * Paths outside the working subtree are left unchanged.
 */
static std::string normalize_abs(std::string const &s)
{
	size_t l = working_dir.length();
	if (s.compare(0, l, working_dir)) return s;
	size_t ll = s.length();
	if (ll == l) return ".";
	if (s[l] != '/')
	{
		size_t pos = s.rfind('/', l);
		assert(pos != std::string::npos);
		return s.substr(pos + 1);
	}
	if (ll == l + 1) return ".";
	return s.substr(l + 1);
}

/**
 * Normalize a target name.
 */
static std::string normalize(std::string const &s)
{
#ifdef WINDOWS
	char const *delim = "/\\";
#else
	char delim = '/';
#endif
	size_t prev = 0, len = s.length();
	size_t pos = s.find_first_of(delim);
	if (pos == std::string::npos) return s;
	bool absolute = pos == 0;
	string_list l;
	for (;;)
	{
		if (pos != prev)
		{
			std::string n = s.substr(prev, pos - prev);
			if (n == "..")
			{
				if (!l.empty()) l.pop_back();
				else if (!absolute)
					return normalize(working_dir + '/' + s);
			}
			else if (n != ".")
				l.push_back(n);
		}
		++pos;
		if (pos >= len) break;
		prev = pos;
		pos = s.find_first_of(delim, prev);
		if (pos == std::string::npos) pos = len;
	}
	string_list::const_iterator i = l.begin(), i_end = l.end();
	if (i == i_end) return absolute ? "/" : ".";
	std::string n;
	if (absolute) n.push_back('/');
	n.append(*i);
	for (++i; i != i_end; ++i)
	{
		n.push_back('/');
		n.append(*i);
	}
	if (absolute) return normalize_abs(n);
	return n;
}

/**
 * Normalize the content of a list of targets.
 */
static void normalize_list(string_list &l)
{
	for (string_list::iterator i = l.begin(),
	     i_end = l.end(); i != i_end; ++i)
	{
		*i = normalize(*i);
	}
}

/**
 * Skip spaces.
 */
static void skip_spaces(std::istream &in)
{
	char c;
	while (strchr(" \t", (c = in.get()))) {}
	if (in.good()) in.putback(c);
}

/**
 * Skip end of line.
 */
static void skip_eol(std::istream &in)
{
	char c;
	while (strchr("\r\n", (c = in.get()))) {}
	if (in.good()) in.putback(c);
}

enum token_e { Word, Eol, Eof, Colon, Equal, Dollar, Rightpar, Comma };

/**
 * Skip spaces and return the kind of the next token.
 */
static token_e next_token(std::istream &in)
{
	while (true)
	{
		skip_spaces(in);
		char c = in.peek();
		if (!in.good()) return Eof;
		switch (c)
		{
		case ':': return Colon;
		case ',': return Comma;
		case '=': return Equal;
		case '$': return Dollar;
		case ')': return Rightpar;
		case '\r':
		case '\n':
			return Eol;
		case '\\':
			in.ignore(1);
			c = in.peek();
			if (c != '\r' && c != '\n')
			{
				in.putback('\\');
				return Word;
			}
			skip_eol(in);
			break;
		default:
			return Word;
		}
	}
}

/**
 * Read a (possibly quoted) word.
 */
static std::string read_word(std::istream &in)
{
	int c = in.get();
	std::string res;
	if (!in.good()) return res;
	char const *separators = " \t\r\n:$(),=\"";
	bool quoted = c == '"';
	if (!quoted)
	{
		if (strchr(separators, c))
		{
			in.putback(c);
			return res;
		}
		res += c;
	}
	while (true)
	{
		c = in.get();
		if (!in.good()) return res;
		if (quoted)
		{
			if (c == '\\')
				res += in.get();
			else if (c == '"')
				return res;
			else
				res += c;
		}
		else
		{
			if (strchr(separators, c))
			{
				in.putback(c);
				return res;
			}
			res += c;
		}
	}
}

static string_list read_words(std::istream &in);

/**
 * Execute a built-in function @a name and append its result to @a dest.
 */
static void execute_function(std::istream &in, std::string const &name, string_list &dest)
{
	if (false)
	{
		error:
		std::cerr << "Failed to load rules: syntax error" << std::endl;
		exit(EXIT_FAILURE);
	}
	skip_spaces(in);
	std::string s = read_word(in);
	if (next_token(in) != Comma) goto error;
	in.ignore(1);
	string_list names = read_words(in);
	if (next_token(in) != Rightpar) goto error;
	in.ignore(1);
	if (name == "addprefix")
	{
		for (string_list::const_iterator i = names.begin(),
		     i_end = names.end(); i != i_end; ++i)
		{
			dest.push_back(s + *i);
		}
	}
	else if (name == "addsuffix")
	{
		for (string_list::const_iterator i = names.begin(),
		     i_end = names.end(); i != i_end; ++i)
		{
			dest.push_back(*i + s);
		}
	}
	else goto error;
}

/**
 * Read a list of words, possibly executing functions.
 */
static string_list read_words(std::istream &in)
{
	if (false)
	{
		error:
		std::cerr << "Failed to load rules: syntax error" << std::endl;
		exit(EXIT_FAILURE);
	}
	string_list res;
	while (true)
	{
		switch (next_token(in))
		{
		case Word:
			res.push_back(read_word(in));
			break;
		case Dollar:
		{
			in.ignore(1);
			if (in.get() != '(') goto error;
			std::string name = read_word(in);
			if (name.empty()) goto error;
			token_e tok = next_token(in);
			if (tok == Rightpar)
			{
				in.ignore(1);
				variable_map::const_iterator i = variables.find(name);
				if (i != variables.end())
					res.insert(res.end(), i->second.begin(), i->second.end());
			}
			else execute_function(in, name, res);
		}
		default:
			return res;
		}
	}
}

/**
 * Load dependencies from @a in.
 */
static void load_dependencies(std::istream &in)
{
	while (!in.eof())
	{
		string_list targets = read_words(in);
		if (targets.empty()) return;
		DEBUG << "reading dependencies of target " << targets.front() << std::endl;
		if (in.get() != ':')
		{
			std::cerr << "Failed to load database" << std::endl;
			exit(EXIT_FAILURE);
		}
		ref_ptr<dependency_t> dep;
		dep->targets = targets;
		string_list d = read_words(in);
		dep->deps.insert(d.begin(), d.end());
		for (string_list::const_iterator i = targets.begin(),
		     i_end = targets.end(); i != i_end; ++i)
		{
			dependencies[*i] = dep;
		}
		skip_eol(in);
	}
}

/**
 * Load known dependencies from file <tt>.remake</tt>.
 */
static void load_dependencies()
{
	DEBUG_open << "Loading database... ";
	std::ifstream in(".remake");
	if (!in.good())
	{
		DEBUG_close << "not found\n";
		return;
	}
	load_dependencies(in);
}

/**
 * Read a rule starting with target @a first, if nonempty.
 * Store into #generic_rules or #specific_rules depending on its genericity.
 */
static void load_rule(std::istream &in, std::string const &first)
{
	DEBUG_open << "Reading rule for target " << first << "... ";
	if (false)
	{
		error:
		DEBUG_close << "failed\n";
		std::cerr << "Failed to load rules: syntax error" << std::endl;
		exit(EXIT_FAILURE);
	}
	rule_t rule;

	// Read targets and check genericity.
	string_list targets = read_words(in);
	if (!first.empty()) targets.push_front(first);
	else if (targets.empty()) goto error;
	else DEBUG << "actual target: " << targets.front() << std::endl;
	bool generic = false;
	normalize_list(targets);
	for (string_list::const_iterator i = targets.begin(),
	     i_end = targets.end(); i != i_end; ++i)
	{
		if (i->empty()) goto error;
		if ((i->find('%') != std::string::npos) != generic)
		{
			if (i == targets.begin()) generic = true;
			else goto error;
		}
	}
	std::swap(rule.targets, targets);
	skip_spaces(in);
	if (in.get() != ':') goto error;

	// Read dependencies.
	rule.deps = read_words(in);
	normalize_list(rule.deps);
	skip_spaces(in);
	char c = in.get();
	if (c != '\r' && c != '\n') goto error;
	skip_eol(in);

	// Read script.
	std::ostringstream buf;
	while (true)
	{
		char c = in.get();
		if (!in.good()) break;
		if (c == '\t' || c == ' ')
		{
			in.get(*buf.rdbuf());
			if (in.fail() && !in.eof()) in.clear();
		}
		else if (c == '\r' || c == '\n')
			buf << c;
		else
		{
			in.putback(c);
			break;
		}
	}
	rule.script = buf.str();

	// Add generic rules to the correct set.
	if (generic)
	{
		generic_rules.push_back(rule);
		return;
	}

	// Rules with a nonempty script lump all their targets in the same
	// dependency set, while other rules behave as if they had been
	// replicated for each of their targets.
	if (!rule.script.empty())
	{
		ref_ptr<dependency_t> dep;
		dep->targets = rule.targets;
		dep->deps.insert(rule.deps.begin(), rule.deps.end());
		for (string_list::const_iterator i = rule.targets.begin(),
		     i_end = rule.targets.end(); i != i_end; ++i)
		{
			ref_ptr<dependency_t> &d = dependencies[*i];
			dep->deps.insert(d->deps.begin(), d->deps.end());
			d = dep;
		}
	}
	else
	{
		for (string_list::const_iterator i = rule.targets.begin(),
		     i_end = rule.targets.end(); i != i_end; ++i)
		{
			ref_ptr<dependency_t> &dep = dependencies[*i];
			if (dep->targets.empty()) dep->targets.push_back(*i);
			dep->deps.insert(rule.deps.begin(), rule.deps.end());
		}
	}

	if (first_target.empty())
		first_target = rule.targets.front();

	ref_ptr<rule_t> r(rule);
	for (string_list::const_iterator i = rule.targets.begin(),
	     i_end = rule.targets.end(); i != i_end; ++i)
	{
		std::pair<rule_map::iterator,bool> j =
			specific_rules.insert(std::make_pair(*i, r));
		if (j.second) continue;
		std::cerr << "Failed to load rules: " << *i
			<< " cannot be the target of several rules" << std::endl;
		exit(EXIT_FAILURE);
	}
}

/**
 * Save all the dependencies in file <tt>.remake</tt>.
 */
static void save_dependencies()
{
	DEBUG_open << "Saving database... ";
	std::ofstream db(".remake");
	while (!dependencies.empty())
	{
		ref_ptr<dependency_t> dep = dependencies.begin()->second;
		for (string_list::const_iterator i = dep->targets.begin(),
		     i_end = dep->targets.end(); i != i_end; ++i)
		{
			db << escape_string(*i) << ' ';
			dependencies.erase(*i);
		}
		db << ':';
		for (string_set::const_iterator i = dep->deps.begin(),
		     i_end = dep->deps.end(); i != i_end; ++i)
		{
			db << ' ' << escape_string(*i);
		}
		db << std::endl;
	}
}

/**
 * Load rules.
 * If some rules have dependencies and non-generic targets, add these
 * dependencies to the targets.
 */
static void load_rules()
{
	DEBUG_open << "Loading rules... ";
	if (false)
	{
		error:
		std::cerr << "Failed to load rules: syntax error" << std::endl;
		exit(EXIT_FAILURE);
	}
	std::ifstream in("Remakefile");
	if (!in.good())
	{
		std::cerr << "Failed to load rules: no Remakefile found" << std::endl;
		exit(EXIT_FAILURE);
	}
	skip_eol(in);

	// Read rules
	while (in.good())
	{
		char c = in.peek();
		if (c == '#')
		{
			while (in.get() != '\n') {}
			skip_eol(in);
			continue;
		}
		if (c == ' ' || c == '\t') goto error;
		token_e tok = next_token(in);
		if (tok == Word)
		{
			std::string name = read_word(in);
			if (name.empty()) goto error;
			if (next_token(in) == Equal)
			{
				in.ignore(1);
				DEBUG << "Assignment to variable " << name << std::endl;
				variables[name] = read_words(in);
				skip_eol(in);
			}
			else load_rule(in, name);
		}
		else if (tok == Dollar)
			load_rule(in, std::string());
		else goto error;
	}

	// Generate script for variable assignment
	std::ostringstream buf;
	for (variable_map::const_iterator i = variables.begin(),
	     i_end = variables.end(); i != i_end; ++i)
	{
		std::ostringstream var;
		bool first = true;
		for (string_list::const_iterator j = i->second.begin(),
		     j_end = i->second.end(); j != j_end; ++j)
		{
			if (first) first = false;
			else var << ' ';
			var << *j;
		}
		buf << i->first << '=' << escape_string(var.str()) << std::endl;
	}
	variable_block = buf.str();
}

/**
 * Substitute a pattern into a list of strings.
 */
static void substitute_pattern(std::string const &pat, string_list const &src, string_list &dst)
{
	for (string_list::const_iterator i = src.begin(),
	     i_end = src.end(); i != i_end; ++i)
	{
		size_t pos = i->find('%');
		if (pos == std::string::npos)dst.push_back(*i);
		else dst.push_back(i->substr(0, pos) + pat + i->substr(pos + 1));
	}
}

/**
 * Find a generic rule matching @a target:
 * - the one leading to shorter matches has priority,
 * - among equivalent rules, the earliest one has priority.
 */
static rule_t find_generic_rule(std::string const &target)
{
	size_t tlen = target.length(), plen = tlen + 1;
	rule_t rule;
	for (rule_list::const_iterator i = generic_rules.begin(),
	     i_end = generic_rules.end(); i != i_end; ++i)
	{
		for (string_list::const_iterator j = i->targets.begin(),
		     j_end = i->targets.end(); j != j_end; ++j)
		{
			size_t len = j->length();
			if (tlen < len) continue;
			if (plen <= tlen - (len - 1)) continue;
			size_t pos = j->find('%');
			if (pos == std::string::npos) continue;
			size_t len2 = len - (pos + 1);
			if (j->compare(0, pos, target, 0, pos) ||
			    j->compare(pos + 1, len2, target, tlen - len2, len2))
				continue;
			plen = tlen - (len - 1);
			std::string pat = target.substr(pos, plen);
			rule = rule_t();
			rule.script = i->script;
			substitute_pattern(pat, i->targets, rule.targets);
			substitute_pattern(pat, i->deps, rule.deps);
			break;
		}
	}
	return rule;
}

/**
 * Find a specific rule matching @a target. Return a generic one otherwise.
 * If there is both a specific rule with an empty script and a generic rule, the
 * generic one is returned after adding the dependencies of the specific one.
 */
static rule_t find_rule(std::string const &target)
{
	rule_map::const_iterator i = specific_rules.find(target),
		i_end = specific_rules.end();
	// If there is a specific rule with a script, return it.
	if (i != i_end && !i->second->script.empty()) return *i->second;
	rule_t grule = find_generic_rule(target);
	// If there is no generic rule, return the specific rule (no script), if any.
	if (grule.targets.empty())
	{
		if (i != i_end) return *i->second;
		return grule;
	}
	// Optimize the lookup when there is only one target (alread looked up).
	if (grule.targets.size() == 1)
	{
		if (i != i_end)
			grule.deps.insert(grule.deps.end(),
				i->second->deps.begin(), i->second->deps.end());
		return grule;
	}
	// Add the dependencies of the specific rules of every target to the
	// generic rule. If any of those rules has a nonempty script, error out.
	for (string_list::const_iterator j = grule.targets.begin(),
	     j_end = grule.targets.end(); j != j_end; ++j)
	{
		i = specific_rules.find(*j);
		if (i == i_end) continue;
		if (!i->second->script.empty()) return rule_t();
		grule.deps.insert(grule.deps.end(),
			i->second->deps.begin(), i->second->deps.end());
	}
	return grule;
}

/**
 * Compute and memoize the status of @a target:
 * - if the file does not exist, the target is obsolete,
 * - if any dependency is obsolete or younger than the file, it is obsolete,
 * - otherwise it is up-to-date.
 *
 * @note With multiple targets, they all share the same status. (If one is
 *       obsolete, they all are.) For the second rule above, the latest target
 *       is chosen, not the oldest!
 */
static status_t const &get_status(std::string const &target)
{
	std::pair<status_map::iterator,bool> i =
		status.insert(std::make_pair(target, status_t()));
	status_t &ts = i.first->second;
	if (!i.second) return ts;
	DEBUG_open << "Checking status of " << target << "... ";
	dependency_map::const_iterator j = dependencies.find(target);
	if (j == dependencies.end())
	{
		struct stat s;
		if (stat(target.c_str(), &s) != 0)
		{
			DEBUG_close << "missing\n";
			ts.status = Todo;
			ts.last = 0;
			return ts;
		}
		DEBUG_close << "up-to-date\n";
		ts.status = Uptodate;
		ts.last = s.st_mtime;
		return ts;
	}
	dependency_t const &dep = *j->second;
	status_e st = Uptodate;
	time_t latest = 0;
	for (string_list::const_iterator k = dep.targets.begin(),
	     k_end = dep.targets.end(); k != k_end; ++k)
	{
		struct stat s;
		if (stat(k->c_str(), &s) != 0)
		{
			if (st == Uptodate) DEBUG_close << *k << " missing\n";
			s.st_mtime = 0;
			st = Todo;
		}
		status[*k].last = s.st_mtime;
		if (s.st_mtime > latest) latest = s.st_mtime;
	}
	if (st == Todo) goto update;
	for (string_set::const_iterator k = dep.deps.begin(),
	     k_end = dep.deps.end(); k != k_end; ++k)
	{
		status_t const &ts_ = get_status(*k);
		if (latest < ts_.last)
		{
			DEBUG_close << "older than " << *k << std::endl;
			st = Todo;
			goto update;
		}
		if (ts_.status == Uptodate) continue;
		if (st == Uptodate)
			DEBUG << "obsolete dependency " << *k << std::endl;
		st = Recheck;
	}
	if (st == Uptodate) DEBUG_close << "all siblings up-to-date\n";
	update:
	for (string_list::const_iterator k = dep.targets.begin(),
	     k_end = dep.targets.end(); k != k_end; ++k)
	{
		status[*k].status = st;
	}
	return ts;
}

/**
 * Change the status of @a target to #Remade or #Uptodate depending on whether
 * its modification time changed.
 */
static void update_status(std::string const &target)
{
	DEBUG_open << "Rechecking status of " << target << "... ";
	status_map::iterator i = status.find(target);
	assert (i != status.end());
	status_t &ts = i->second;
	ts.status = Remade;
	if (ts.last >= now)
	{
		DEBUG_close << "possibly remade\n";
		return;
	}
	struct stat s;
	if (stat(target.c_str(), &s) != 0)
	{
		DEBUG_close << "missing\n";
		ts.last = 0;
	}
	else if (s.st_mtime != ts.last)
	{
		DEBUG_close << "remade\n";
		ts.last = s.st_mtime;
	}
	else
	{
		DEBUG_close << "unchanged\n";
		ts.status = Uptodate;
	}
}

/**
 * Check if all the prerequisites of @a target ended being up-to-date.
 */
static bool still_need_rebuild(std::string const &target)
{
	DEBUG_open << "Rechecking obsoleteness of " << target << "... ";
	status_map::const_iterator i = status.find(target);
	assert (i != status.end());
	if (i->second.status != Recheck) return true;
	dependency_map::const_iterator j = dependencies.find(target);
	assert(j != dependencies.end());
	dependency_t const &dep = *j->second;
	for (string_set::const_iterator k = dep.deps.begin(),
	     k_end = dep.deps.end(); k != k_end; ++k)
	{
		if (status[*k].status != Uptodate) return true;
	}
	for (string_list::const_iterator k = dep.targets.begin(),
	     k_end = dep.targets.end(); k != k_end; ++k)
	{
		status[*k].status = Uptodate;
	}
	DEBUG_close << "no longer obsolete\n";
	return false;
}

/**
 * Handle job completion.
 */
static void complete_job(int job_id, bool success)
{
	DEBUG_open << "Completing job " << job_id << "... ";
	job_targets_map::iterator i = job_targets.find(job_id);
	assert(i != job_targets.end());
	string_list const &targets = i->second;
	if (success)
	{
		for (string_list::const_iterator j = targets.begin(),
		     j_end = targets.end(); j != j_end; ++j)
		{
			update_status(*j);
		}
	}
	else
	{
		DEBUG_close << "failed\n";
		std::cerr << "Failed to build";
		for (string_list::const_iterator j = targets.begin(),
		     j_end = targets.end(); j != j_end; ++j)
		{
			status[*j].status = Failed;
			std::cerr << ' ' << *j;
			remove(j->c_str());
		}
		std::cerr << std::endl;
	}
	job_targets.erase(i);
}

/**
 * Execute the script from @a rule.
 */
static bool run_script(int job_id, rule_t const &rule)
{
	if (show_targets)
	{
		std::cout << "Building";
		for (string_list::const_iterator i = rule.targets.begin(),
		     i_end = rule.targets.end(); i != i_end; ++i)
		{
			std::cout << ' ' << *i;
		}
		std::cout << std::endl;
	}

	ref_ptr<dependency_t> dep;
	dep->targets = rule.targets;
	dep->deps.insert(rule.deps.begin(), rule.deps.end());
	for (string_list::const_iterator i = rule.targets.begin(),
	     i_end = rule.targets.end(); i != i_end; ++i)
	{
		dependencies[*i] = dep;
	}

	DEBUG_open << "Starting script for job " << job_id << "... ";
#ifdef WINDOWS
	HANDLE pfd[2];
	if (false)
	{
		error2:
		CloseHandle(pfd[0]);
		CloseHandle(pfd[1]);
		error:
		DEBUG_close << "failed\n";
		complete_job(job_id, false);
		return false;
	}
	if (!CreatePipe(&pfd[0], &pfd[1], NULL, 0))
		goto error;
	if (!SetHandleInformation(pfd[0], HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
		goto error2;
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdInput = pfd[0];
	si.dwFlags |= STARTF_USESTDHANDLES;
	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	std::ostringstream buf;
	buf << job_id;
	if (!SetEnvironmentVariable("REMAKE_JOB_ID", buf.str().c_str()))
		goto error2;
	std::ostringstream argv;
	argv << "SH.EXE -e -s";
	if (echo_scripts) argv << " -v";
	for (string_list::const_iterator i = rule.targets.begin(),
	     i_end = rule.targets.end(); i != i_end; ++i)
	{
		argv << " \"" << escape_string(*i) << '"';
	}
	if (!CreateProcess(NULL, (char *)argv.str().c_str(), NULL, NULL,
	    true, 0, NULL, NULL, &si, &pi))
	{
		goto error2;
	}
	CloseHandle(pi.hThread);
	std::string script = variable_block + rule.script;
	DWORD len = script.length(), wlen;
	if (!WriteFile(pfd[1], script.c_str(), len, &wlen, NULL) || wlen < len)
		std::cerr << "Unexpected failure while sending script to shell" << std::endl;
	CloseHandle(pfd[0]);
	CloseHandle(pfd[1]);
	++running_jobs;
	job_pids[pi.hProcess] = job_id;
	return true;
#else
	int pfd[2];
	if (false)
	{
		error2:
		close(pfd[0]);
		close(pfd[1]);
		error:
		DEBUG_close << "failed\n";
		complete_job(job_id, false);
		return false;
	}
	if (pipe(pfd) == -1)
		goto error;
	if (pid_t pid = fork())
	{
		if (pid == -1) goto error2;
		std::string script = variable_block + rule.script;
		ssize_t len = script.length();
		if (write(pfd[1], script.c_str(), len) < len)
			std::cerr << "Unexpected failure while sending script to shell" << std::endl;
		close(pfd[0]);
		close(pfd[1]);
		++running_jobs;
		job_pids[pid] = job_id;
		return true;
	}
	// Child process starts here.
	std::ostringstream buf;
	buf << job_id;
	if (setenv("REMAKE_JOB_ID", buf.str().c_str(), 1))
		_exit(EXIT_FAILURE);
	int num = echo_scripts ? 4 : 3;
	char const **argv = new char const *[num + rule.targets.size() + 1];
	argv[0] = "sh";
	argv[1] = "-e";
	argv[2] = "-s";
	if (echo_scripts) argv[3] = "-v";
	for (string_list::const_iterator i = rule.targets.begin(),
	     i_end = rule.targets.end(); i != i_end; ++i, ++num)
	{
		argv[num] = i->c_str();
	}
	argv[num] = NULL;
	if (pfd[0] != 0)
	{
		dup2(pfd[0], 0);
		close(pfd[0]);
	}
	close(pfd[1]);
	execv("/bin/sh", (char **)argv);
	_exit(EXIT_FAILURE);
#endif
}

/**
 * Create a job for @a target according to the loaded rules.
 * Mark all the targets from the rule as running and reset their dependencies.
 * If the rule has dependencies, create a new client to build them just
 * before @a current, and change @a current so that it points to it.
 */
static bool start(std::string const &target, client_list::iterator &current)
{
	DEBUG_open << "Starting job " << job_counter << " for " << target << "... ";
	rule_t rule = find_rule(target);
	if (rule.targets.empty())
	{
		status[target].status = Failed;
		DEBUG_close << "failed\n";
		std::cerr << "No rule for building " << target << std::endl;
		return false;
	}
	for (string_list::const_iterator i = rule.targets.begin(),
	     i_end = rule.targets.end(); i != i_end; ++i)
	{
		status[*i].status = Running;
	}
	int job_id = job_counter++;
	job_targets[job_id] = rule.targets;
	if (!rule.deps.empty())
	{
		current = clients.insert(current, client_t());
		current->job_id = job_id;
		std::swap(current->pending, rule.deps);
		current->delayed = new rule_t(rule);
		return true;
	}
	return run_script(job_id, rule);
}

/**
 * Send a reply to a client then remove it.
 * If the client was a dependency client, start the actual script.
 */
static void complete_request(client_t &client, bool success)
{
	DEBUG_open << "Completing request from client of job " << client.job_id << "... ";
	if (client.delayed)
	{
		assert(client.socket == INVALID_SOCKET);
		if (success)
		{
			if (still_need_rebuild(client.delayed->targets.front()))
				run_script(client.job_id, *client.delayed);
			else complete_job(client.job_id, true);
		}
		else complete_job(client.job_id, false);
		delete client.delayed;
	}
	else if (client.socket != INVALID_SOCKET)
	{
		char res = success ? 1 : 0;
		send(client.socket, &res, 1, 0);
	#ifdef WINDOWS
		closesocket(client.socket);
	#else
		close(client.socket);
	#endif
		--waiting_jobs;
	}

	if (client.job_id < 0 && !success) build_failure = true;
}

/**
 * Return whether there are slots for starting new jobs.
 */
static bool has_free_slots()
{
	if (max_active_jobs <= 0) return true;
	return running_jobs - waiting_jobs < max_active_jobs;
}

/**
 * Update clients as long as there are free slots:
 * - check for running targets that have finished,
 * - start as many pending targets as allowed,
 * - complete the request if there are neither running nor pending targets
 *   left or if any of them failed.
 */
static void update_clients()
{
	DEBUG_open << "Updating clients... ";
	for (client_list::iterator i = clients.begin(), i_next = i,
	     i_end = clients.end(); i != i_end && has_free_slots(); i = i_next)
	{
		++i_next;
		DEBUG_open << "Handling client from job " << i->job_id << "... ";
		if (false)
		{
			failed:
			complete_request(*i, false);
			clients.erase(i);
			DEBUG_close << "failed\n";
			continue;
		}

		// Remove running targets that have finished.
		for (string_set::iterator j = i->running.begin(), j_next = j,
		     j_end = i->running.end(); j != j_end; j = j_next)
		{
			++j_next;
			status_map::const_iterator k = status.find(*j);
			assert(k != status.end());
			switch (k->second.status)
			{
			case Running:
				break;
			case Failed:
				if (!keep_going) goto failed;
				i->failed = true;
				// no break
			case Uptodate:
			case Remade:
				i->running.erase(j);
				break;
			case Recheck:
			case Todo:
				assert(false);
			}
		}

		// Start pending targets.
		while (!i->pending.empty())
		{
			std::string target = i->pending.front();
			i->pending.pop_front();
			switch (get_status(target).status)
			{
			case Running:
				i->running.insert(target);
				break;
			case Failed:
				pending_failed:
				if (!keep_going) goto failed;
				i->failed = true;
				// no break
			case Uptodate:
			case Remade:
				break;
			case Recheck:
			case Todo:
				client_list::iterator j = i;
				if (!start(target, i)) goto pending_failed;
				j->running.insert(target);
				if (!has_free_slots()) return;
				// Job start might insert a dependency client.
				i_next = i;
				++i_next;
				break;
			}
		}

		// Try to complete request.
		// (This might start a new job if it was a dependency client.)
		if (i->running.empty())
		{
			if (i->failed) goto failed;
			complete_request(*i, true);
			clients.erase(i);
			DEBUG_close << "finished\n";
		}
	}
}

/**
 * Create a named unix socket that listens for build requests. Also set
 * the REMAKE_SOCKET environment variable that will be inherited by all
 * the job scripts.
 */
static void create_server()
{
	if (false)
	{
		error:
		perror("Failed to create server");
#ifndef WINDOWS
		error2:
#endif
		exit(EXIT_FAILURE);
	}
	DEBUG_open << "Creating server... ";

#ifdef WINDOWS
	// Prepare a windows socket.
	struct sockaddr_in socket_addr;
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	socket_addr.sin_port = 0;

	// Create and listen to the socket.
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) goto error;
	if (!SetHandleInformation((HANDLE)socket_fd, HANDLE_FLAG_INHERIT, 0))
		goto error;
	if (bind(socket_fd, (struct sockaddr *)&socket_addr, sizeof(sockaddr_in)))
		goto error;
	int len = sizeof(sockaddr_in);
	if (getsockname(socket_fd, (struct sockaddr *)&socket_addr, &len))
		goto error;
	std::ostringstream buf;
	buf << socket_addr.sin_port;
	if (!SetEnvironmentVariable("REMAKE_SOCKET", buf.str().c_str()))
		goto error;
	if (listen(socket_fd, 1000)) goto error;
#else
	// Set a handler for SIGCHLD then block the signal (unblocked during select).
	sigset_t sigmask;
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &sigmask, NULL) == -1) goto error;
	struct sigaction sa;
	sa.sa_flags = 0;
	sa.sa_handler = &child_sig_handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGCHLD, &sa, NULL) == -1) goto error;

	// Prepare a named unix socket in temporary directory.
	socket_name = tempnam(NULL, "rmk-");
	if (!socket_name) goto error2;
	struct sockaddr_un socket_addr;
	size_t len = strlen(socket_name);
	if (len >= sizeof(socket_addr.sun_path) - 1) goto error2;
	socket_addr.sun_family = AF_UNIX;
	strcpy(socket_addr.sun_path, socket_name);
	len += sizeof(socket_addr.sun_family);
	if (setenv("REMAKE_SOCKET", socket_name, 1)) goto error;

	// Create and listen to the socket.
#ifdef MACOSX
	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_fd < 0) goto error;
	if (fcntl(socket_fd, F_SETFD, FD_CLOEXEC) < 0) goto error;
#else
	socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (socket_fd < 0) goto error;
#endif
	if (bind(socket_fd, (struct sockaddr *)&socket_addr, len))
		goto error;
	if (listen(socket_fd, 1000)) goto error;
#endif
}

/**
 * Accept a connection from a client, get the job it spawned from,
 * get the targets, and mark them as dependencies of the job targets.
 */
void accept_client()
{
	DEBUG_open << "Handling client request... ";

	// Accept connection.
#ifdef WINDOWS
	socket_t fd = accept(socket_fd, NULL, NULL);
	if (fd == INVALID_SOCKET) return;
	if (!SetHandleInformation((HANDLE)fd, HANDLE_FLAG_INHERIT, 0))
	{
		error2:
		std::cerr << "Unexpected failure while setting connection with client" << std::endl;
		closesocket(fd);
		return;
	}
	// WSAEventSelect puts sockets into nonblocking mode, so disable it here.
	u_long nbio = 0;
	if (ioctlsocket(fd, FIONBIO, &nbio)) goto error2;
#elif defined(MACOSX)
	int fd = accept(socket_fd, NULL, NULL);
	if (fd < 0) return;
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) return;
#else
	int fd = accept4(socket_fd, NULL, NULL, SOCK_CLOEXEC);
	if (fd < 0) return;
#endif
	clients.push_front(client_t());
	client_list::iterator proc = clients.begin();

	if (false)
	{
		error:
		DEBUG_close << "failed\n";
		std::cerr << "Received an ill-formed client message" << std::endl;
	#ifdef WINDOWS
		closesocket(fd);
	#else
		close(fd);
	#endif
		clients.erase(proc);
		return;
	}

	// Receive message. Stop when encountering two nuls in a row.
	std::vector<char> buf;
	size_t len = 0;
	while (len < sizeof(int) + 2 || buf[len - 1] || buf[len - 2])
	{
		buf.resize(len + 1024);
		ssize_t l = recv(fd, &buf[0] + len, 1024, 0);
		if (l <= 0) goto error;
		len += l;
	}

	// Parse job that spawned the client.
	int job_id;
	memcpy(&job_id, &buf[0], sizeof(int));
	proc->socket = fd;
	proc->job_id = job_id;
	job_targets_map::const_iterator i = job_targets.find(job_id);
	if (i == job_targets.end()) goto error;
	DEBUG << "receiving request from job " << job_id << std::endl;

	// Parse the targets and mark them as dependencies from the job targets.
	dependency_t &dep = *dependencies[job_targets[job_id].front()];
	char const *p = &buf[0] + sizeof(int);
	while (true)
	{
		len = strlen(p);
		if (len == 0)
		{
			++waiting_jobs;
			return;
		}
		std::string target(p, p + len);
		DEBUG << "adding dependency " << target << " to job\n";
		proc->pending.push_back(target);
		dep.deps.insert(target);
		p += len + 1;
	}
}

/**
 * Loop until all the jobs have finished.
 */
void server_loop()
{
	while (true)
	{
		update_clients();
		if (running_jobs == 0)
		{
			assert(clients.empty());
			break;
		}
		DEBUG_open << "Handling events... ";
	#ifdef WINDOWS
		size_t len = job_pids.size() + 1;
		HANDLE h[len];
		int num = 0;
		for (pid_job_map::const_iterator i = job_pids.begin(),
		     i_end = job_pids.end(); i != i_end; ++i, ++num)
		{
			h[num] = i->first;
		}
		WSAEVENT aev = WSACreateEvent();
		h[num] = aev;
		WSAEventSelect(socket_fd, aev, FD_ACCEPT);
		DWORD w = WaitForMultipleObjects(len, h, false, INFINITE);
		WSAEventSelect(socket_fd, aev, 0);
		WSACloseEvent(aev);
		if (w < WAIT_OBJECT_0 || WAIT_OBJECT_0 + len <= w)
			continue;
		if (w == WAIT_OBJECT_0 + len - 1)
		{
			accept_client();
			continue;
		}
		pid_t pid = h[w - WAIT_OBJECT_0];
		DWORD s = 0;
		bool res = GetExitCodeProcess(pid, &s) && s == 0;
		CloseHandle(pid);
		pid_job_map::iterator i = job_pids.find(pid);
		assert(i != job_pids.end());
		int job_id = i->second;
		job_pids.erase(i);
		--running_jobs;
		complete_job(job_id, res);
	#else
		sigset_t emptymask;
		sigemptyset(&emptymask);
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(socket_fd, &fdset);
		int ret = pselect(socket_fd + 1, &fdset, NULL, NULL, NULL, &emptymask);
		if (ret > 0 /* && FD_ISSET(socket_fd, &fdset)*/) accept_client();
		if (!got_SIGCHLD) continue;
		got_SIGCHLD = 0;
		pid_t pid;
		int status;
		while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
		{
			bool res = WIFEXITED(status) && WEXITSTATUS(status) == 0;
			pid_job_map::iterator i = job_pids.find(pid);
			assert(i != job_pids.end());
			int job_id = i->second;
			job_pids.erase(i);
			--running_jobs;
			complete_job(job_id, res);
		}
	#endif
	}
}

/**
 * Load dependencies and rules, listen to client requests, and loop until
 * all the requests have completed.
 * If Remakefile is obsolete, perform a first run with it only, then reload
 * the rules, and perform a second with the original clients.
 */
void server_mode(string_list const &targets)
{
	load_dependencies();
	load_rules();
	create_server();
	if (get_status("Remakefile").status != Uptodate)
	{
		clients.push_back(client_t());
		clients.back().pending.push_back("Remakefile");
		server_loop();
		if (build_failure) goto early_exit;
		variables.clear();
		specific_rules.clear();
		generic_rules.clear();
		first_target.clear();
		load_rules();
	}
	clients.push_back(client_t());
	if (!targets.empty()) clients.back().pending = targets;
	else if (!first_target.empty())
		clients.back().pending.push_back(first_target);
	server_loop();
	early_exit:
	close(socket_fd);
	remove(socket_name);
	save_dependencies();
	exit(build_failure ? EXIT_FAILURE : EXIT_SUCCESS);
}

/**
 * Connect to the server @a socket_name, send a build request for @a targets,
 * and exit with the status returned by the server.
 */
void client_mode(char *socket_name, string_list const &targets)
{
	if (false)
	{
		error:
		perror("Failed to send targets to server");
		exit(EXIT_FAILURE);
	}
	if (targets.empty()) exit(EXIT_SUCCESS);
	DEBUG_open << "Connecting to server... ";

	// Connect to server.
#ifdef WINDOWS
	struct sockaddr_in socket_addr;
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) goto error;
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	socket_addr.sin_port = atoi(socket_name);
	if (connect(socket_fd, (struct sockaddr *)&socket_addr, sizeof(sockaddr_in)))
		goto error;
#else
	struct sockaddr_un socket_addr;
	size_t len = strlen(socket_name);
	if (len >= sizeof(socket_addr.sun_path) - 1) exit(EXIT_FAILURE);
	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_fd < 0) goto error;
	socket_addr.sun_family = AF_UNIX;
	strcpy(socket_addr.sun_path, socket_name);
	if (connect(socket_fd, (struct sockaddr *)&socket_addr, sizeof(socket_addr.sun_family) + len))
		goto error;
#ifdef MACOSX
	int set_option = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_NOSIGPIPE, &set_option, sizeof(set_option)))
		goto error;
#endif
#endif

	// Send current job id.
	char *id = getenv("REMAKE_JOB_ID");
	int job_id = id ? atoi(id) : -1;
	if (send(socket_fd, (char *)&job_id, sizeof(job_id), MSG_NOSIGNAL) != sizeof(job_id))
		goto error;

	// Send tagets.
	for (string_list::const_iterator i = targets.begin(),
	     i_end = targets.end(); i != i_end; ++i)
	{
		DEBUG_open << "Sending " << *i << "... ";
		ssize_t len = i->length() + 1;
		if (send(socket_fd, i->c_str(), len, MSG_NOSIGNAL) != len)
			goto error;
	}

	// Send terminating nul and wait for reply.
	char result = 0;
	if (send(socket_fd, &result, 1, MSG_NOSIGNAL) != 1) goto error;
	if (recv(socket_fd, &result, 1, 0) != 1) exit(EXIT_FAILURE);
	exit(result ? EXIT_SUCCESS : EXIT_FAILURE);
}

/**
 * Display usage and exit with @a exit_status.
 */
void usage(int exit_status)
{
	std::cerr << "Usage: remake [options] [target] ...\n"
		"Options\n"
		"  -d                     Echo script commands.\n"
		"  -d -d                  Print lots of debugging information.\n"
		"  -h, --help             Print this message and exit.\n"
		"  -j[N], --jobs=[N]      Allow N jobs at once; infinite jobs with no arg.\n"
		"  -k                     Keep going when some targets cannot be made.\n"
		"  -r                     Look up targets from the dependencies on standard input.\n"
		"  -s, --silent, --quiet  Do not echo targets.\n";
	exit(exit_status);
}

/**
 * This program behaves in two different ways.
 *
 * - If the environment contains the REMAKE_SOCKET variable, the client
 *   connects to this socket and sends to the server its build targets.
 *   It exits once it receives the server reply.
 *
 * - Otherwise, it creates a server that waits for build requests. It
 *   also creates a pseudo-client that requests the targets passed on the
 *   command line.
 */
int main(int argc, char *argv[])
{
	init_working_dir();

	string_list targets;
	bool indirect_targets = false;

	// Parse command-line arguments.
	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg.empty()) usage(EXIT_FAILURE);
		if (arg == "-h" || arg == "--help") usage(EXIT_SUCCESS);
		if (arg == "-d")
			if (echo_scripts) debug.active = true;
			else echo_scripts = true;
		else if (arg == "-k" || arg =="--keep-going")
			keep_going = true;
		else if (arg == "-s" || arg == "--silent" || arg == "--quiet")
			show_targets = false;
		else if (arg == "-r")
			indirect_targets = true;
		else if (arg.compare(0, 2, "-j") == 0)
			max_active_jobs = atoi(arg.c_str() + 2);
		else if (arg.compare(0, 7, "--jobs=") == 0)
			max_active_jobs = atoi(arg.c_str() + 7);
		else
		{
			if (arg[0] == '-') usage(1);
			targets.push_back(normalize(arg));
			DEBUG << "New target: " << arg << '\n';
		}
	}

	if (indirect_targets)
	{
		load_dependencies(std::cin);
		string_list l;
		targets.swap(l);
		if (l.empty() && !dependencies.empty())
		{
			l.push_back(dependencies.begin()->second->targets.front());
		}
		for (string_list::const_iterator i = l.begin(),
		     i_end = l.end(); i != i_end; ++i)
		{
			dependency_map::const_iterator j = dependencies.find(*i);
			if (j == dependencies.end()) continue;
			dependency_t const &dep = *j->second;
			for (string_set::const_iterator k = dep.deps.begin(),
			     k_end = dep.deps.end(); k != k_end; ++k)
			{
				targets.push_back(normalize(*k));
			}
		}
		dependencies.clear();
	}

#ifdef WINDOWS
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,2), &wsaData))
	{
		std::cerr << "Unexpected failure while initializing Windows Socket" << std::endl;
		return 1;
	}
#endif

	// Run as client if REMAKE_SOCKET is present in the environment.
	if (char *sn = getenv("REMAKE_SOCKET")) client_mode(sn, targets);

	// Otherwise run as server.
	server_mode(targets);
}
