/*$Id: error.c,v 1.2 2006/05/06 22:14:46 jan_wrobel Exp $*/
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include "error.h"

#define MAXLINE 8192

static FILE *logfile;

/**
 * loginit - Open a file for debugging logs.
 * @name: The name of the log file.
 *
 * If file doesn't exist it is created, otherwise it is truncated.
 */
void loginit(const char *name)
{
	if (name == NULL){
		logfile = NULL;
		return;
	}
	if (!(logfile = fopen(LOGFILE, "w+")))
		err_quit("can't open log file %s\n", LOGFILE);
	setlinebuf(logfile);
}

/**
 * err_sys_func - Report fatal error to stderr and to log file if it was
 * opened and terminate process. Write also error message specified
 * by errno value.
 * @funcname: The name of the function in which error has occured.
 * @fmt: printf like format
 *
 * Don't use this function, use macro err_sys() instead, it does the same
 * thing but sets funcname argument automatically.
 */
void err_sys_func(const char *funcname, const char *fmt, ...)
{
	int errno_save = errno;
	char buf[MAXLINE];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	fprintf(stderr, "fatal_error: %s: %s\n", buf, strerror(errno_save));
	if (logfile){
		fprintf(logfile,"[%d](%s) fatal error: ", getpid(), funcname); 
		fprintf(logfile, "%s: %s\n", buf, strerror(errno_save));
	}
	va_end(ap);
	exit(1);
}


/**
 * err_ret_func - Report error to stderr and to log file if it was
 * opened. Write also error message specified by errno value.
 * @funcname: The name of the function in which error has occured.
 * @fmt: printf like format
 *
 * Don't use this function, use macro err_ret() instead, it does the same
 * thing but sets funcname argument automatically.
 */
void err_ret_func(const char *funcname, const char *fmt, ...)
{
	int errno_save = errno;
	char buf[MAXLINE];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	fprintf(stderr, "error: %s: %s\n", buf, strerror(errno_save));
	if (logfile){
		fprintf(logfile,"[%d](%s) error: ", getpid(), funcname); 
		fprintf(logfile, "%s: %s\n", buf, strerror(errno_save));
	}
	va_end(ap);
}

/**
 * err_quit_func - The same as err_sys_func but doesn't print error message
 * specified by errno value. 
 *
 * Don't use this function, use macro err_quit() instead, it does the same
 * thing but sets funcname argument automatically.
 */
void err_quit_func(const char *funcname, const char *fmt, ...)
{
	char buf[MAXLINE];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	fprintf(stderr, "fatal error: %s\n", buf);
	if (logfile){
		fprintf(logfile,"[%d](%s) error: ", getpid(), funcname); 
		fprintf(logfile, "%s\n", buf);
	}
	va_end(ap);
	exit(1);
}

/**
 * err_msg_func - The same as err_ret_func but doesn't print error message
 * specified by errno value. 
 *
 * Don't use this function, use macro err_msg() instead, it does the same
 * thing but sets funcname argument automatically.
 */
void err_msg_func(const char *funcname, const char *fmt, ...)
{
	char buf[MAXLINE];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	fprintf(stderr, "error: %s\n", buf);
	if (logfile){
		fprintf(logfile,"[%d](%s) error: ", getpid(), funcname); 
		fprintf(logfile, "%s\n", buf);
	}
	va_end(ap);
}

/*Do not use this function, use TRACE() macro from debug.h instead*/
inline void trace_func(const char *funcname, int pid, const char *fmt, ...)
{
	char buf[MAXLINE];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	
	if(pid)
		fprintf(logfile, "[%d](%s) ", pid, funcname); 
	else
		fprintf(logfile, "[no-slave](%s)", funcname);
	fprintf(logfile, "%s\n", buf); 
}

/**
 * stderr_print - Print message to stderr.
 * @fmt: printf like format
 */
void stderr_print(const char *fmt, ...)
{
	char buf[MAXLINE];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	fprintf(stderr, "%s", buf);
	va_end(ap);
}


/*
  struct user_regs_struct {
  long ebx, ecx, edx, esi, edi, ebp, eax;
  unsigned short ds, __ds, es, __es;
  unsigned short fs, __fs, gs, __gs;
  long orig_eax, eip;
  unsigned short cs, __cs;
  long eflags, esp;
  unsigned short ss, __ss;
void 
dump_registers(struct user_regs_struct r)
{
	fprintf(stderr, "dumping registers:\n"
		"eax = %ld orig_eax = %ld ebx = %ld\n"
		"ecx = %ld edx = %ld esi = %ld edi = %ld ebp = %ld\n" 
		"eip = %ld esp = %ld\n", r.eax, r.orig_eax, r.ebx, r.ecx,
		r.edx, r.esi, r.edi, r.ebp, r.eip, r.esp);
}*/
