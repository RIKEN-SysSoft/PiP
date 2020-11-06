/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019, 2020$
 * $PIP_VERSION: Version 2.0.0$
 * $PIP_license: <Simplified BSD License>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the PiP project.$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>
 */

/** \page pips pips

\brief List or kill running PiP tasks

\synopsis
pips [a][u][x] [PIP-OPTIONS] [-] [PATTERN ..]

\param a|u|x similar to the aux options of the Linux ps command
\param \-\-root List PiP root(s)
\param \-\-task List PiP task(s)
\param \-\-family List PiP root(s) and PiP task(s) in family order
\param \-\-kill Send SIGTERM to PiP root(s) and task(s)
\param \-\-signal Send a signal to PiP root(s) and task(s). This
option must be followed by a signal number of name.
\param \-\-ps Run the ps Linux command. This option may have \c ps
command option(s) separated by comma (,)
\param \-\-top Run the top Linux command. This option may have
\c top command option(s) separated by comma (,)
\param \- Simply ignored. This option can be used to avoid the
ambiguity of the options.

\description
\c pips is a filter to target only PiP tasks (including PiP root)
to show status like the way what the \c ps commands does and send
signals to the selected PiP tasks.

Just like the \c ps command, \c pips can take the most familiar
\c ps options \c a, \c u, \c x. Here is an example;

\verbatim
$ pips
PID   TID   TT       TIME     PIP COMMAND
18741 18741 pts/0    00:00:00 RT  pip-exec
18742 18742 pts/0    00:00:00 RG  pip-exec
18743 18743 pts/0    00:00:00 RL  pip-exec
18741 18744 pts/0    00:00:00 0T  a
18745 18745 pts/0    00:00:00 0G  b
18746 18746 pts/0    00:00:00 0L  c
18747 18747 pts/0    00:00:00 1L  c
18741 18748 pts/0    00:00:00 1T  a
18749 18749 pts/0    00:00:00 1G  b
18741 18750 pts/0    00:00:00 2T  a
18751 18751 pts/0    00:00:00 2G  b
18741 18752 pts/0    00:00:00 3T  a
\endverbatim

here, there are 3 \c pip\-exec root processes running. Four pip
tasks running program 'a' with the ptherad mode, three PiP tasks
running program 'b' with the process:got mode, and two PiP tasks
running program 'c' with the process:preload mode.

Unlike the \c ps command, two columns 'TID' and 'PIP' are added. The
'TID' field is to identify PiP tasks in pthread execution mode.
three PiP tasks running in the pthread mode. As for the 'PiP'
field, if the first letter is 'R' then that pip task is running as a
PiP root. If this letter is
a number from '0' to '9' then this is a PiP task (not root).
The number is the least\-significant
digit of the PiP ID of that PiP task. The second letter represents
the PiP execution mode which is common with PiP root and task. 'L' is
'process:preload,' 'C' is 'process:pipclone,', 'G' is 'process:got,'
and 'T' is 'thread.'

The last 'COMMAND' column of the
\c pips output may be different from what the \c ps command shows,
although it looks the same. It represents the command, not the
command line consisting of a command and its argument(s). More
precisely speaking, it is the first 14 letters of the command. This comes
from the PiP's specificity. PiP tasks are not created by using the
normal \c exec systemcall and the Linux assumes the same command
line with the pip root process which creates the pip tasks.

If users want to have the other \c ps command options other than
'aux', then refer to the \c \-\-ps option described below. But in this
case, the command lines of PiP tasks (excepting PiP roots) are not
correct.

\li \c \-\-root (\c \-r) Only the PiP root tasks will be shown.
\verbatim
$ pips --root
PID   TID   TT       TIME     PIP COMMAND
18741 18741 pts/0    00:00:00 RT  pip-exec
18742 18742 pts/0    00:00:00 RG  pip-exec
18743 18743 pts/0    00:00:00 RL  pip-exec
\endverbatim

\li \c \-\-task (\c \-t) Only the PiP tasks (excluding root) will
be shown. If both of \c \-\-root and \c \-\-task are specified,
then firstly PiP roots are shown and then PiP tasks will be
shown.
\verbatim
$ pips --tasks
PID   TID   TT       TIME     PIP COMMAND
18741 18744 pts/0    00:00:00 0T  a
18745 18745 pts/0    00:00:00 0G  b
18746 18746 pts/0    00:00:00 0L  c
18747 18747 pts/0    00:00:00 1L  c
18741 18748 pts/0    00:00:00 1T  a
18749 18749 pts/0    00:00:00 1G  b
18741 18750 pts/0    00:00:00 2T  a
18751 18751 pts/0    00:00:00 2G  b
18741 18752 pts/0    00:00:00 3T  a
\endverbatim

\li \c \-\-family (\c \-f) All PiP roots and tasks of the selected
PiP tasks by the \c PATTERN optional argument of \c pips.
\verbatim
$ pips - a
PID   TID   TT       TIME     PIP COMMAND
18741 18744 pts/0    00:00:00 0T  a
18741 18748 pts/0    00:00:00 1T  a
18741 18750 pts/0    00:00:00 2T  a
$ pips --family a
PID   TID   TT       TIME     PIP COMMAND
18741 18741 pts/0    00:00:00 RT  pip-exec
18741 18744 pts/0    00:00:00 0T  a
18741 18748 pts/0    00:00:00 1T  a
18741 18750 pts/0    00:00:00 2T  a
\endverbatim
In this example, "pips - a" (the \- is needed not to confused the
command name \c a as the \c pips option) shows the PiP tasks which
is derived
from the program \c a. The second run, "pips --family a," shows the
PiP tasks of \c a and their PiP root (\c pip-exec, in this example).

\li \c \-\-kill (\c \-k) Send SIGTERM signal to the selected PiP
tasks.
\li \c \-\-signal (\c \-s) \c SIGNAL Send the specified signal to
the selected PiP tasks.
\li \c \-\-ps (\c \-P) This option may be followed by the \c ps
command options. When this option is specified, the PIDs of
selected PiP tasks are passed to the \c ps command with the
specified \c ps command options, if given.
\li \c \-\-top (\c \-T) This option may be followed by the \c top
command options. When this option is specified, the PIDs of
selected PiP tasks are passed to the \c top command with the
specified \c top command options, if given.
\li \c PATTERN The last argument is the pattern(s) to select which PiP
tasks to be selected and shown. This pattern can be a command name (only
the first 14 characters are effective), PID, TID, or a Unix (Linux)
filename matching pattern (if the fnmatch Python module is available).
\verbatim
$ pips - *-*
PID   TID   TT       TIME     PIP COMMAND
18741 18741 pts/0    00:00:00 RT  pip-exec
18742 18742 pts/0    00:00:00 RG  pip-exec
18743 18743 pts/0    00:00:00 RL  pip-exec
\endverbatim

\note
\c pips collects PiP tasks' status by using the Linux's \c ps
command. When the \c \-\-ps or \c \-\-top option is specified, the \c
ps or \c top command is invoked after invoking the \c ps command for
information gathering. This, however, may result some PiP tasks may not
appear in the invoked \c ps or \c top command when one or more PiP tasks
finished after the first \c ps command invocation.
The same situation may also happen with the \c \-\-kill or \-\-signal
option.
*/
