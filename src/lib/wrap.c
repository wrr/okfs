/*$Id: wrap.c,v 1.2 2006/05/06 22:14:46 jan_wrobel Exp $*/
/*
 *  This file is part of Out of Kernel File System.
 *
 *  Copyright (C) 2004-2005 Jan Wrobel <wrobel@blues.ath.cx>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License Version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*This file doesn't include Kernel headers so it can use GLIBC one*/
#include "wrap.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <linux/ptrace.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "error.h"

extern int errno;

/****************************************************************/
/*Wrapping routines similar to these introduced by Richard Stevens
 *in his "UNIX Network Programming". They reduce lenght of fatal error
 *detection code.
 *Coding convention: Function which name starts with capital
 *letter calls suitable libc function and when it fails it 
 *reports fatal error and interrupts application.
 */

inline void *Malloc(int size)
{
	void *r = malloc(size);
	if (!r)
		err_sys("malloc %u", size);
	return r;
}

inline char *Strdup(const char *s)
{
	char *r = strdup(s);
	if (!r)
		err_sys("strdup");
	return r;
}


inline int Setuid(int uid)
{
	int r = setuid(uid);
        if (r < 0)
            err_sys("setuid");
	return r;
}

inline int Setgid(int gid)
{
	int r = setgid(gid);
        if (r < 0)
		err_sys("setgid");
	return r;
}



inline void *Calloc(int nmemb, int size)
{
	void *r = calloc(nmemb, size);
	if (!r)
		err_sys("calloc");
	return r;
}

inline void *Realloc(void *ptr, int size)
{
	void *r = realloc(ptr, size);
	if (!r)
		err_sys("realloc");
	return r;
}

inline sighandler_t Signal(int signum, sighandler_t handler)
{
	sighandler_t r;
	if ((r = signal(signum, handler)) == SIG_ERR)
		err_sys("signal error");
	return r;
}

inline int Sigdelset(sigset_t *set, int signum)
{
	if (sigdelset(set, signum) < 0)
		err_sys("sigdelset error");
	return 0;
}

inline int Sigemptyset(sigset_t *set)
{
	if (sigemptyset(set) < 0)
		err_sys("sigemptyset error");
	return 0;
}

inline int Sigaddset(sigset_t *set, int signum)
{
	if (sigaddset(set, signum) < 0)
		err_sys("sigaddset error");
	return 0;
}
inline int Sigismember(sigset_t *set, int signum)
{
	int r;
	if ((r = sigismember(set, signum)) < 0)
		err_sys("sigismember error signum = %d", signum);
	return r;
}

inline int Sigaction(int signum, const struct sigaction *act, struct sigaction *olda)
{
	int r;
	if ((r = sigaction(signum, act, olda)) < 0)
		err_sys("sigaction error for signal %s", strsignal(signum));
	return r;
}

inline int Close(int fd)
{
	if (close(fd) < 0)
		err_sys("close error");
	return 0;
}

inline int Dup2(int oldfd, int newfd)
{
	int fd;
	if ((fd = dup2(oldfd, newfd)) < 0)
		err_sys("dup2 error");
	return 0;
}

inline int Read(int fd, void *buf, int count)
{
	int n;
	if ((n = read(fd, buf, count)) < 0)
		err_sys("read error");
	return n;
}

inline int Write(int fd, const void *buf, int count)
{
	int n;
	if ((n = write(fd, buf, count)) < 0)
		err_sys("write error");
	return n;
}

inline int Fork()
{
        int pid;
        if ((pid = fork()) < 0)
                err_sys("fork error");
        return pid;
}

inline int Wait(int *status){
        int pid;
        if ((pid = wait(status)) < 0)
                err_sys("wait error");
        return pid;
}

inline int Waitpid(int pid, int * status, int options){
	int r;
	if ((r = waitpid(pid, status, options)) < 0)
		err_sys("waitpid error");
	return r;
}


inline int Kill(int pid, int sig)
{
        if (kill(pid, sig) < 0)
                err_sys("kill error");
        return 0;
}

inline int Pipe(int filedes[2])
{
	if (pipe(filedes) < 0)
		err_sys("pipe error");
	return 0;
}

inline int Select(int n, fd_set *rfd, fd_set *wfd, fd_set *efd, struct timeval *to)
{
	int r;
	if ((r = select(n, rfd, wfd, efd, to)) < 0)
		err_sys("select error");
	return r;
}


long int okfs_ptrace(int request, int pid, void * addr, void * data)
{
	return ptrace(request, pid, addr, data);
}


long int Ptrace(int request, int pid, void * addr, void * data)
{
        long int r;
        errno = 0;
        if ((r = ptrace(request, pid, addr, data)) < 0 && errno){
		/*let's be more verbose here*/
		switch(request){
		case PTRACE_TRACEME: 
			err_sys("ptrace traceme error");
		case PTRACE_ATTACH: 
			err_sys("ptrace attach to process %d error", pid);
		case PTRACE_DETACH: 
			err_sys("ptrace dettach process %d error", pid);
		case PTRACE_SYSCALL:
			err_sys("ptrace(PTRACE_SYSCALL, %d, ...) error", pid);
		case PTRACE_CONT:
			err_sys("ptrace cont process %d error", pid);
		case PTRACE_SINGLESTEP:
			err_sys("ptrace single step process %d error", pid);
		case PTRACE_KILL:
			err_sys("ptrace kill process %d error", pid);
		case PTRACE_GETREGS:
			err_sys("ptrace get registers from process %d error", 
				pid);
		case PTRACE_SETREGS:
			err_sys("ptrace set registers for process %d error",
				pid);

		case PTRACE_PEEKDATA:
			err_sys("ptrace peek data from process %d error", pid);
		case PTRACE_PEEKTEXT:
			err_sys("ptrace peek text from process %d error", pid);
		case PTRACE_PEEKUSER:
			err_sys("ptrace peek a word from the user area of" 
				" process %d error", pid);

		case PTRACE_POKEDATA:
			err_sys("ptrace poke data for process %d error", pid);
		case PTRACE_POKETEXT:
			err_sys("ptrace poke text for process %d error", pid);
		case PTRACE_POKEUSER:
			err_sys("ptrace poke a word to the user area of process"
				" %d error", pid);
		case PTRACE_SETOPTIONS:
			err_sys("ptrace setoptions request failed. Maybe this"\
				"kernel version doesn't support this option"\
				"or its value in /usr/include/linux/ptrace.h"\
				"doesn't match one in task.h"
				);
		default:
			err_sys("ptrace unrecognized error");
		}
	}
	return r;
}
/****************************************************************/

/* 
 *These functions are wraping glibc macros, which are used in files
 *that include Kernel headers, so they can't include glibc one.
 */
inline int wifexited(int status)
{
	return WIFEXITED(status);
}

inline int wexitstatus(int status)
{
	return WEXITSTATUS(status);
}

inline int wifstopped(int status)
{
	return WIFSTOPPED(status);
}

inline int wstopsig(int status)
{
	return WSTOPSIG(status);
}

inline int wifsignaled(int status)
{
	return WIFSIGNALED(status);
}

inline int wtermsig(int status)
{
	return WTERMSIG(status);
}
/****************************************************************/

/*
 * Wrapers for sigaction function from glibc. Unlike glibc version
 * these functions don't tak structures as arguments so can be included
 * without glibc headers.
 */
int simple_sigaction(int signum, sighandler_t handler, int flags)
{
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = flags;
	act.sa_handler = handler;
	return sigaction(signum, &act, NULL);
}

inline int
Simple_sigaction(int signum, sighandler_t handler, int flags)
{
	if (simple_sigaction(signum, handler, flags) < 0)
		err_sys("sigaction");
	return 0;
}
