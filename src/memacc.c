/*$Id: memacc.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
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

/*
 * Get and set content of traced process memory.
 * TODO: I think these routines can be optimized by reading and writing
 * to /proc/mem instead of using ptrace() call.
 */

#include <linux/ptrace.h>
#include "libc.h"
#include "memacc.h"
#include "task.h"
#include "debug.h"
#include "wrap.h"

/*TODO: are these values in some Linux header file?*/
#define WORDNBYTES 4
#define CHAR_BIT 8
//#define WORDNBYTES (__WORDSIZE / CHAR_BIT)

#define STRING_START_SIZE WORDNBYTES * 8

//extern int errno;


/*write n bytes to traced process memory*/
void* slave_memset(void *d, const void *s, int n)
{
	unsigned long word, mask = ULONG_MAX;
	long *dest = (long *)d, *src = (long *)s;
	
	for(;n >= WORDNBYTES; n -= WORDNBYTES){
		Ptrace(PTRACE_POKEDATA, slave->pid, dest, (void *)*src);
		dest++;
		src++;
	}
	
	if (n == 0)
		return(d);
	
	errno = 0;
	word = Ptrace(PTRACE_PEEKDATA, slave->pid, dest, 0);
	
	mask <<= CHAR_BIT * n;
	word &= mask;
	word |= *src & ~mask; 
	
	errno = 0;
	Ptrace(PTRACE_POKEDATA, slave->pid, dest, (void*)word);
	
	return(d);
}

/*read n bytes from traced process memory*/
void* slave_memget(void *d, const void *s, int n)
{
	unsigned long word;/* mask = ULONG_MAX;*/
	long *dest = (long *)d, *src = (long *)s;
	int i;
	
	for(;n >= WORDNBYTES; n -= WORDNBYTES){
		*dest = Ptrace(PTRACE_PEEKDATA, slave->pid, src, 0);
		dest++;
		src++;
	}
	
	if (n == 0)
		return(d);

	word = Ptrace(PTRACE_PEEKDATA, slave->pid, src, 0);
	
	for(i = 0; i < n; i++)
		*((char *)dest + i)= *((char *)&word + i);
	/*loop can by replaced by commented code. I think it is faster,
	  but valgrind complaints that it reads and writes besides allocated
	  memory. In fact it does but without changing it. To
	  shut up valgrind let the loop remain.
	mask <<= CHAR_BIT * n;
	dest &= mask;
	dest |= word & ~mask; 
	*/
	return(d);
}

/*Replace string in traced process memory with a new one.
 *Remember the original string, and the rest of memory behind it if
 *NEWSTRING is longer that ORIG_STRING.
 *Use this function when original string is already known in order to avoid
 *reading it once again.
 */
void 
slave_rpl_known_str(void *dest, char *newstring, char *orig_string)
{
	int newstring_len = strlen(newstring);
	int orig_string_len = strlen(orig_string);
	ASSERT(newstring != NULL && orig_string != NULL);
	if (slave->rpl_code)
		err_quit("code in traced process area was replaced and not restored");
	slave->rpl_code = Malloc(newstring_len + 1);
	slave->rpl_code_len = newstring_len + 1;
	strncpy(slave->rpl_code, orig_string, slave->rpl_code_len);
	if (slave->rpl_code_len > orig_string_len + 1)
		slave_memget(slave->rpl_code + orig_string_len + 1, 
			  dest + orig_string_len + 1, 
			  slave->rpl_code_len - orig_string_len - 1);
	
	slave_memset(dest, newstring, newstring_len + 1);	
	slave->rpl_dest = dest;
}

/*Replace N bytes of traced process memory with a given contents.
 *Remember the original contents in  order to restore it with a slave_restore_mem
 *call.*/
void
slave_rpl_mem(void *dest, void *src, int n)
{
	if (slave->rpl_code)
		err_quit("code in traced process area was replaced and not restored");
	slave->rpl_code = Malloc(n);
	slave->rpl_code_len = n;
	slave->rpl_dest = dest;
	slave_memget(slave->rpl_code, dest, n);
	slave_memset(dest, src, n);	
}

void slave_restore_mem(void)
{
	if (!slave->rpl_code)
		err_quit("can't restore traced process memory, it wasn't changed");
	slave_memset(slave->rpl_dest, slave->rpl_code, slave->rpl_code_len);
	free(slave->rpl_code);
	slave->rpl_code = NULL;
}


/*Read a string from traced process memory and alloc memory for it.*/
char* slave_getstr(void *s)
{
	char *string;
	int n = STRING_START_SIZE, nreaded = 0, i;
	long *dest, *src = (long *)s;
	string = Calloc(n, sizeof(char));
	dest = (long *)string;
	
	for(;;){
		errno = 0;
		*dest = Ptrace(PTRACE_PEEKDATA, slave->pid, src, 0);
		for(i = 0; i < WORDNBYTES; i++)
			if(((char *)dest)[i] == 0)
				return(string);
		dest++;
		src++;
		nreaded += WORDNBYTES;
		if (nreaded == n){
			n *= 2;
			string = Realloc(string, n);
			dest = (long *)(string + nreaded);
		}
	}
}

