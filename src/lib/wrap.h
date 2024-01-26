/*$Id: wrap.h,v 1.2 2006/05/06 22:14:46 jan_wrobel Exp $*/
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
#ifndef WRAP_H
#define WRAP_H

/*TODO: convert them to macros*/

void *Malloc(int);
void *Calloc(int, int);
void *Realloc(void *, int);

char *Strdup(const char *);
long int Ptrace(int, int, void *, void *);

int Setuid(int);
int Setgid(int);

int Fork(void);
int Wait(int *);
int Waitpid(int, int *, int);
int Kill(int, int);

int Pipe(int [2]);
int Dup2(int, int);
int Close(int);

int Read(int, void*, int);
int Write(int, const void *, int);

long int okfs_ptrace(int, int, void *, void *);

typedef void (*sighandler_t)(int);
int simple_sigaction(int signum, sighandler_t handler, int flags);
int Simple_sigaction(int, sighandler_t, int);

int wifexited(int);
int wexitstatus(int);
int wifstopped(int);
int wstopsig(int);
int wifsignaled(int);
int wtermsig(int);



#endif
