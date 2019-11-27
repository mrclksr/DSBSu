/*-
 * Copyright (c) 2018 Marcel Kaiser. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <libutil.h>
#include <time.h>
#include <fcntl.h>
#include <paths.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <pwd.h>
#include <signal.h>
#include <termios.h>

#include "libdsbsu.h"

#define PATH_SU		"/usr/bin/su"
#define AUTHMSG_FAIL	"su: Sorry"
#define SUPROMPT	"Password:"
#define AUTHMSG_SUCCESS	"4a6e03e670e39ebe649fbfdb64bb14e0"
#define SUMAXREBUFSZ	((sizeof(SUPROMPT) + sizeof(AUTHMSG_FAIL)) * 2 + 1024)
#define ERRBUFSZ	1024
#define FATAL_SYSERR	(DSBSU_ERR_SYS | DSBSU_ERR_FATAL)

#define ERROR(ret, error, prepend, fmt, ...) do { \
	set_error(error, prepend, fmt, ##__VA_ARGS__); \
	return (ret); \
} while (0)

struct dsbsu_proc_s {
	int	 fdstdout;   /* The process' stdout. */
	int	 fdstdin;    /* The process' stdin. */
	int	 master;     /* Master fd of pty */
	bool	 ttymod;     /* Whether tty on stdin was modified. */
	pid_t	 pid;	     /* PID of process. */
	sigset_t sset;	     /* Saved signal set. */
	struct termios term; /* Saved settings of current tty. */
};

static int     send_eof(int);
static int     init_tty(dsbsu_proc *);
static int     reset_tty(dsbsu_proc *);
static int     send_pass(dsbsu_proc *, const char *, const char *);
static int     wait_on_proc(dsbsu_proc *, bool);
static void    set_error(int, bool, const char *, ...);
static ssize_t _write(int, const void *, size_t);
static ssize_t _read(int, void *, size_t);

static int  _error;
static char errmsg[ERRBUFSZ];

const char *
dsbsu_strerror()
{
	return (errmsg);
}

int
dsbsu_error()
{
	return (_error);
}

bool
dsbsu_validate_user(const char *user)
{
	struct passwd *pw;

	if (user == NULL)
		user = "root";
	errno = 0;
	pw = getpwnam(user); endpwent();
	if (pw == NULL) {
		if (errno != 0)
			ERROR(false, FATAL_SYSERR, false, "getpwnam()");
		ERROR(false, DSBSU_ENOUSER, false, "No such user '%s'", user);
	}
	return (true);
}

bool
dsbsu_is_me(const char *user)
{
	struct passwd *pw;

	if (user == NULL)
		user = "root";
	errno = 0;
	pw = getpwuid(getuid()); endpwent();
	if (strcmp(user, pw->pw_name) == 0)
		return (true);
	return (false);
}

dsbsu_proc *
dsbsu_exec_su(const char *cmd, const char *user, const char *pass)
{
	int	      c, n, slave, stdin_sv[2], stdout_sv[2], ptest[2];
	char	      *cmdbuf;
	bool	      stdin_ispipe, stdout_ispipe, retignore;
	sigset_t      sset;
	dsbsu_proc    *proc;
	struct winsize wsz, *wszp;

	retignore = false;
	slave = stdin_sv[0] = stdin_sv[1] = ptest[0] = ptest[1] = -1;
	stdout_sv[0] = stdout_sv[1] = -1;

	if (user == NULL)
		user = "root";
	if (!dsbsu_validate_user(user))
		return (NULL);
	if ((proc = malloc(sizeof(dsbsu_proc))) == NULL)
		ERROR(NULL, FATAL_SYSERR, false, "malloc()");
	proc->pid = proc->master = proc->fdstdin = proc->fdstdout = -1;
	proc->ttymod = false;

	/*
	 * We can tell whether authentication failed, but not
	 * if it was successful. Prepend an "/bin/echo" that outputs
	 * a string to indicate success.
	 */
	if ((cmdbuf = malloc(strlen(cmd) +
	    sizeof(AUTHMSG_SUCCESS) + strlen("/bin/echo \"\";"))) == NULL)
		ERROR(NULL, FATAL_SYSERR, false, "malloc()");
	(void)sprintf(cmdbuf, "/bin/echo \"%s\";%s", AUTHMSG_SUCCESS, cmd);
	(void)sigemptyset(&sset);
        (void)sigaddset(&sset, SIGCHLD);
	(void)sigprocmask(SIG_BLOCK, &sset, &proc->sset);
	if (isatty(fileno(stdin))) {
		if (ioctl(fileno(stdin), TIOCGWINSZ, &wsz) == -1) {
			set_error(FATAL_SYSERR, false, "ioctl()");
			goto error;
		}
		wszp = &wsz;
	} else
		wszp = NULL;
	if ((openpty(&proc->master, &slave, NULL, NULL, wszp)) == -1) {
		set_error(FATAL_SYSERR, false, "openpty()");
		goto error;
	}
	if (init_tty(proc) == -1)
		goto error;
	if (socketpair(AF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0, ptest) == -1) {
		set_error(FATAL_SYSERR, false, "socketpair()");
		goto error;
	}

	stdin_ispipe = false;
	proc->fdstdin = proc->master;

	stdout_ispipe = false;
	proc->fdstdout = fileno(stdout);

	if (!isatty(fileno(stdout)) || !isatty(fileno(stdin))) {
		if (!isatty(fileno(stdout))) {
			if (socketpair(AF_LOCAL,
			    SOCK_STREAM | SOCK_CLOEXEC, 0, stdout_sv) == -1) {
				set_error(FATAL_SYSERR, false, "socketpair()");
				goto error;
			}
			stdout_ispipe = true;
			proc->fdstdout = stdout_sv[0];
		}
		if (socketpair(AF_LOCAL,
		    SOCK_STREAM | SOCK_CLOEXEC, 0, stdin_sv) == -1) {
			set_error(FATAL_SYSERR, false, "socketpair()");
			goto error;
		}
		stdin_ispipe = true;
		proc->fdstdin = stdin_sv[0];
	}
	if ((proc->pid = fork()) == -1) {
		set_error(FATAL_SYSERR, false, "fork()");
		goto error;
	} else if (proc->pid == 0) {
		(void)close(stdin_sv[0]);
		(void)close(stdout_sv[0]);
		(void)close(ptest[0]);
		(void)close(proc->master);
		if (login_tty(slave) == -1)
			_exit(DSBSU_ERR_SYS + errno);
		if (stdin_ispipe) {
			if (dup2(stdin_sv[1], fileno(stdin)) == -1)
				_exit(DSBSU_ERR_SYS + errno);
		}
		if (stdout_ispipe) {
			if (dup2(stdout_sv[1], fileno(stdout)) == -1)
				_exit(DSBSU_ERR_SYS + errno);
		}
		(void)sigprocmask(SIG_SETMASK, &proc->sset, NULL);
		(void)execl(PATH_SU, PATH_SU, "-m", user, "-c",
		    cmdbuf, NULL);
		_exit(DSBSU_EEXECSU + errno);
	} else {
		(void)close(stdin_sv[1]);
		(void)close(stdout_sv[1]);
		(void)close(slave);
		(void)close(ptest[1]);
		free(cmdbuf); cmdbuf = NULL;
	}
	/* Wait for the child to exec. su */
	if (_read(ptest[0], &c, 1) == -1)
		goto error;
	(void)close(ptest[0]);
	if ((n = send_pass(proc, SUPROMPT, pass)) == 0)
		return (proc);
	if (n != -1)
		retignore = true;
error:
	free(cmdbuf);
	(void)close(slave);
	(void)close(proc->master);
	(void)close(stdin_sv[0]);
	(void)close(stdin_sv[1]);
	(void)close(stdout_sv[0]);
	(void)close(stdout_sv[1]);
	(void)close(ptest[0]);
	(void)close(ptest[1]);
	(void)reset_tty(proc);
	if (proc->pid != -1) {
		(void)kill(proc->pid, SIGTERM);
		(void)wait_on_proc(proc, retignore);
	}
	(void)sigprocmask(SIG_SETMASK, &proc->sset, NULL);
	free(proc);

	return (NULL);
}

int
dsbsu_wait(dsbsu_proc *proc)
{
	int	maxfd, ret;
	char	buf[4096];
	size_t	bufsz;
	fd_set	allset, rset;
	ssize_t	rd;
	struct sigaction ign, sapipe;

	bufsz = sizeof(buf) - 1;
	maxfd = proc->master > fileno(stdin) ? proc->master : fileno(stdin);
	maxfd = maxfd > proc->fdstdout ? maxfd : proc->fdstdout;
	FD_ZERO(&allset);
	FD_SET(proc->master, &allset);
	FD_SET(fileno(stdin), &allset);
	if (proc->fdstdout != fileno(stdout))
		FD_SET(proc->fdstdout, &allset);
	ign.sa_flags   = 0;
	ign.sa_handler = SIG_IGN;
	(void)sigemptyset(&ign.sa_mask);
	(void)sigaction(SIGPIPE, &ign, &sapipe);

	for (ret = errno = 0;;) {
		rset = allset;
		if (select(maxfd + 1, &rset, 0, 0, 0) <= 0) {
			if (errno != EINTR)
				ERROR(-1, FATAL_SYSERR, false, "select()");
			continue;
		}
		if (FD_ISSET(proc->master, &rset)) {
			FD_CLR(proc->master, &rset);
			if ((rd = _read(proc->master, buf, bufsz)) == -1) {
				if (errno == EPIPE)
					goto cleanup;
				goto error;
			} else if (rd == 0) {
				goto cleanup;
			} else if (_write(proc->fdstdout, buf, rd) == -1) {
				if (errno == EPIPE)
					goto cleanup;
				goto error;
			}
		}
		if (FD_ISSET(fileno(stdin), &rset)) {
			FD_CLR(fileno(stdin), &rset);
			if ((rd = _read(fileno(stdin), buf, bufsz)) == -1) {
				if (errno == EPIPE) {
					FD_CLR(fileno(stdin), &allset);
					if (proc->fdstdin == proc->master) {
						if (send_eof(proc->master) < 0)
							goto error;
					} else
						(void)close(proc->fdstdin);
				} else
					goto error;
			} else if (rd == 0) {
				FD_CLR(fileno(stdin), &allset);
				if (proc->fdstdin == proc->master) {
					if (send_eof(proc->master) < 0)
						goto error;
				} else
					(void)close(proc->fdstdin);
			} else if (_write(proc->fdstdin, buf, rd) == -1) {
				if (errno == EPIPE) {
					(void)close(proc->fdstdin);
					goto cleanup;
				}
				goto error;
			}
		}
		if (FD_ISSET(proc->fdstdout, &rset)) {
			if ((rd = _read(proc->fdstdout, buf, bufsz)) == -1) {
				if (errno == EPIPE) {
					FD_CLR(proc->fdstdout, &allset);
					(void)close(proc->fdstdout);
				} else
					goto error;
			} else if (rd == 0) {
				FD_CLR(proc->fdstdout, &allset);
				(void)close(proc->fdstdout);
			} else if (_write(fileno(stdout), buf, rd) == -1) {
				if (errno == EPIPE) {
					(void)close(proc->fdstdout);
					goto cleanup;
				}
				goto error;
			}
		}
	}
error:
	ret = -1;
	(void)kill(proc->pid, SIGTERM);
cleanup:
	(void)close(proc->fdstdin);
	(void)close(proc->master);
	(void)reset_tty(proc);

	/* Wait for the child to terminiate. */
	if (wait_on_proc(proc, false) != 0)
		ret = -1;
	(void)sigaction(SIGPIPE, &sapipe, NULL);
	(void)sigprocmask(SIG_SETMASK, &proc->sset, NULL);
	
	return (ret);
}

static void
set_error(int error, bool prepend, const char *fmt, ...)
{
	int	_errno;
	char	errbuf[ERRBUFSZ];
	size_t  len;
	va_list ap;

	_errno = errno;
	_error = error;
	va_start(ap, fmt);
	if (prepend) {
		if (error & DSBSU_ERR_FATAL) {
			if (strncmp(errmsg, "Fatal: ", 7) == 0) {
				(void)memmove(errmsg, errmsg + 7,
				    strlen(errmsg) - 6);
			}
			(void)strlcpy(errbuf, "Fatal: ", sizeof(errbuf) - 1);
			len = strlen(errbuf);
		} else
			len = 0;
		(void)vsnprintf(errbuf + len, sizeof(errbuf) - len, fmt, ap);
		len = strlen(errbuf);
		(void)snprintf(errbuf + len, sizeof(errbuf) - len, ":%s",
		    errmsg);
		(void)strlcpy(errmsg, errbuf, sizeof(errmsg));
	} else {
		(void)vsnprintf(errmsg, sizeof(errmsg), fmt, ap);
		if (error & DSBSU_ERR_FATAL) {
			(void)snprintf(errbuf, sizeof(errbuf), "Fatal: %s",
			    errmsg);
			(void)strlcpy(errmsg, errbuf, sizeof(errmsg));
		}
	}
	if ((error & DSBSU_ERR_SYS) && _errno != 0) {
		len = strlen(errmsg);
		(void)snprintf(errmsg + len, sizeof(errmsg) - len,
		    ": %s", strerror(_errno));
		errno = 0;
	}
}

static ssize_t
_write(int fd, const void *buf, size_t len)
{
	ssize_t wr;

	while ((wr = write(fd, buf, len)) == -1) {
		if (errno != EINTR)
			ERROR(-1, FATAL_SYSERR, false, "write()");
	}
	return (wr);
}

static ssize_t
_read(int fd, void *buf, size_t len)
{
	ssize_t rd;

	while ((rd = read(fd, buf, len)) == -1) {
		if (errno != EINTR)
			ERROR(-1, FATAL_SYSERR, false, "read()");
	}
	return (rd);
}

static int
send_pass(dsbsu_proc *proc, const char *prompt, const char *pass)
{
	int	n, maxfd, rfd, wfd;
	char	nl, ln[SUMAXREBUFSZ];
	bool	got_prompt;
	fd_set	rset, allset;
	ssize_t rd, len;
	struct timeval tv;

	got_prompt = false; nl = '\n'; len = 0;
	/*
	 * If neither stdin nor stdout is a terminal "su" will write
	 * messages to its terminal (master). If stdin is a terminal,
	 * but stdout is not, "su" will write messages to stdout.
	 */
	wfd = rfd = proc->master;
	FD_ZERO(&allset);
	maxfd = proc->master > proc->fdstdout ? proc->master : proc->fdstdout;
	FD_SET(proc->fdstdout, &allset);
	FD_SET(proc->master, &allset);

	for (;;) {
		rset = allset;
		tv.tv_sec = 10; tv.tv_usec = 0;
		if ((n = select(maxfd + 1, &rset, NULL, NULL, &tv)) == -1) {
			if (errno == EINTR)
				continue;
			ERROR(FATAL_SYSERR, FATAL_SYSERR, false, "select()");
		}
		if (n == 0) {
			warnx("ln == %s", ln);
			ERROR(DSBSU_ETIMEOUT, DSBSU_ETIMEOUT, false,
			    "'su' timed out");
		}
		if (FD_ISSET(proc->master, &rset))
			rfd = proc->master;
		else
			rfd = proc->fdstdout;
		if ((rd = _read(rfd, ln + len, sizeof(ln) - len - 1)) <= 0) {
			if (rd == -1) {
				ERROR(FATAL_SYSERR, FATAL_SYSERR, false,
				    "read()");
			}
			ERROR(DSBSU_EUNEXPECTED, DSBSU_EUNEXPECTED, false,
				"Unexpected output received");
		}
		if (len + rd >= (ssize_t)sizeof(ln)) {
			ERROR(DSBSU_EUNEXPECTED, DSBSU_EUNEXPECTED, false,
			    "Unexpectedly long reply received from 'su'");
		}
		len += rd; ln[len] = '\0';
		if (!got_prompt) {
			if (strstr(ln, prompt) != NULL) {
				(void)usleep(50000);
				got_prompt = true;
				if (_write(wfd, pass, strlen(pass)) == -1 ||
				    _write(wfd, &nl, 1) == -1) {
					ERROR(FATAL_SYSERR, FATAL_SYSERR,
					    false, "write()");
				}
				(void)tcsendbreak(wfd, 0);
			}
		} else if (strstr(ln, AUTHMSG_FAIL) != NULL) {
			ERROR(DSBSU_EAUTH, DSBSU_EAUTH, false,
			    "Wrong password");
		} else if (strstr(ln, AUTHMSG_SUCCESS) != NULL)
			return (0);
	}
	/* NOTREACHED */
}

static int
send_eof(int tty)
{
	struct termios t;

	if (tcgetattr(tty, &t) == -1)
		ERROR(-1, FATAL_SYSERR, false, "tcgetattr()");
	return ((int)_write(tty, &t.c_cc[VEOF], 1));
}

static int
init_tty(dsbsu_proc *proc)
{
	struct termios t;

	proc->ttymod = false;
	if (!isatty(fileno(stdout)) || !isatty(fileno(stdin)))
		return (0);
	if (tcgetattr(fileno(stdin), &t) == -1)
		ERROR(-1, FATAL_SYSERR, false, "tcgetattr()");
	proc->term = t; cfmakeraw(&t);
	if (tcsetattr(fileno(stdin), TCSAFLUSH, &t) == -1 ||
	    tcsetattr(proc->master, TCSAFLUSH, &proc->term) == -1)
		ERROR(-1, FATAL_SYSERR, false, "tcsetattr()");
	proc->ttymod = true;

	return (0);
}

static int
reset_tty(dsbsu_proc *proc)
{
	if (isatty(fileno(stdin)) && proc->ttymod) {
		if (tcsetattr(fileno(stdin), TCSAFLUSH, &proc->term) == -1)
			ERROR(-1, FATAL_SYSERR, false, "tcsetattr()");
	}
	return (0);
}

static int
wait_on_proc(dsbsu_proc *proc, bool retignore)
{
	siginfo_t si;

	while (waitid(P_PID, proc->pid, &si, WEXITED | WSTOPPED) == -1) {
		if (errno != EINTR) {
			warnx("waitpid()");
			set_error(FATAL_SYSERR, false, "waitid()");
			return (-1);
		}
	}
	if (retignore)
		return (0);
	if (si.si_status & DSBSU_EEXECSU) {
		/* Failed to execute su */
		errno = si.si_status & 0x7f;
		set_error(DSBSU_EEXECSU | DSBSU_ERR_SYS, false, "exec()");
		return (1);
	} else if (si.si_status & DSBSU_ERR_SYS) {
		errno = si.si_status & 0x7f;
		set_error(FATAL_SYSERR, false, "Error");
		return (1);
	} else if (si.si_status == 127) {
		set_error(DSBSU_EEXECCMD, false, "Command not found");
		return (1);
	} else if (si.si_status != 0) {
		set_error(DSBSU_EEXECSU, false,
		    "su returned with exit code %d", si.si_status);
		return (1);
	}
	return (0);
}
