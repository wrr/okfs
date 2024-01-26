/*$Id: libc.h,v 1.2 2006/05/06 22:14:46 jan_wrobel Exp $*/
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

#ifndef LIBC_H
#define LIBC_H

#include <linux/types.h>

/*Glibc headers can't be include in files that include Kernel headers
 *because there is a conflict between them. 
 *This file provides declaration of some useful glibc functions.
 *There are only functions that uses simple types as arguments and return
 *values, to prevent OKFS of being dependent from glibc version.
 */

char *getcwd(char *, size_t size);
void exit(int);
int atexit(void (*f)(void));
pid_t wait(int *);
pid_t waitpid(pid_t, int *, int);
void *alloca(size_t);
void free(void *);
char *strsignal(int);
char *strerror(int);
int mkstemp(char *template);
time_t time(void *);

long int
strtol(const char *, char **, int);
unsigned long int
strtoul(const char *, char **, int);
unsigned long long int
strtoull(const char *, char **, int);

int isdigit(int);

int execvp(const char*, char *const argv[]);

int socketpair(int, int, int, int[2]);

/*OK, this is the only exception from the rule of not including GLIBC 
 *structures here ;)
 *I hope that struct option won't change in future version of glibc.
 */
struct option{
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

int 
getopt_long(int, char * const *, const char *, const struct option *, int *);
#define no_argument		0
#define required_argument	1
#define optional_argument	2

extern int errno;

#ifndef EOF
# define EOF (-1)
#endif

#endif
