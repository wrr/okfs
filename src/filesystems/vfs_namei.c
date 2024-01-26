/*$Id: vfs_namei.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/

/* 
 * Return the canonical absolute name of a given file.
 * Copyright (C) 1996, 1997, 1998, 1999, 2000 Free Software Foundation, Inc.
 * This file was originally part of the GNU C Library.
 *
 * It is modified for Out of Kernel File System. 
 * Copyright (C) 2004-2005 Jan Wrobel <wrobel@blues.ath.cx>
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/errno.h>
#include "okfs_lib.h"
#include "vfs_namei.h"
#include "virtualfs.h"
#include "debug.h"
#include "libc.h"
#include "wrap.h"

#ifndef MAXSYMLINKS
#define MAXSYMLINKS 20
#endif 

/*check if S contains any characters other than '/'*/
static inline int empty_path(const char *s)
{
	const char *p;
	for(p = s; *p; p++)
		if (*p != '/')
			return(0);
	return(1);
}

/* Return the canonical absolute name of file NAME.  A canonical name
   does not contain any `.', `..' components nor any repeated path
   separators ('/') or symlinks.
   The result is malloced. If NAME is not absolute path result is
   composed from NAME and CWD which specified slave working directory 
   otherwise CWD can be NULL. It is ok, if last component in NAME doesn't 
   exist (for example in open() last component will be created).
   The difference between lcanonicalize and canonicalize is that it
   doesn't follow last link in path*/
char *
canonical(const char *cwd, const char *name, int read_last_link)
{
	char *rpath, *dest, *extra_buf = NULL;
	const char *start, *end, *rpath_limit;
	long int path_max;
	int num_links = 0;
	if (name == NULL){
		errno = EINVAL;
		return NULL;
	}

	if (name[0] == '\0'){
		errno = ENOENT;
		return NULL;
	}
      
	path_max = PATH_MAX;
	rpath = Malloc(path_max);
	rpath_limit = rpath + path_max;

	if (name[0] != '/'){
		if (!cwd){
			errno = EINVAL;
			rpath[0] = '\0';
			goto error;
		}
		strcpy(rpath, cwd);
		dest = strchr(rpath, '\0');
	}else{
		rpath[0] = '/';
		dest = rpath + 1;
	}

	for (start = end = name; *start; start = end){
		struct stat st;
		int n;

		/* Skip sequence of multiple path-separators.*/
		while (*start == '/')
			++start;
		
		/* Find end of path component.*/
		for (end = start; *end && *end != '/'; ++end)
			/* Nothing.*/;
		
		if (end - start == 0)
			break;
		else if (end - start == 1 && start[0] == '.')
			/* nothing */;
		else if (end - start == 2 && start[0] == '.' 
			 && start[1] == '.'){
			/* Back up to previous component, ignore if at root 
			   already.  */
			if (dest > rpath + 1)
				while ((--dest)[-1] != '/');
		}
		else{
			size_t new_size;
			if (dest[-1] != '/')
				*dest++ = '/';

			if (dest + (end - start) >= rpath_limit){
				ptrdiff_t dest_offset = dest - rpath;
				new_size = rpath_limit - rpath;
				if (end - start + 1 > path_max)
					new_size += end - start + 1;
				else
					new_size += path_max;
				rpath = Realloc(rpath, new_size);
				rpath_limit = rpath + new_size;
				if (rpath == NULL)
					return NULL;

				dest = rpath + dest_offset;
			}


			dest = okfs_mempcpy(dest, start, end - start);
			*dest = '\0';

			if (!read_last_link && empty_path(end))
				continue;
			
			if (okfs_arglstat(rpath, &st) < 0){
				if (errno == ENOENT && empty_path(end))
					continue;
				else
					goto error;
			}

			if (S_ISLNK(st.st_mode)){
				char *buf = alloca(path_max);
				size_t len;
				
				if (++num_links > MAXSYMLINKS){
					errno = ELOOP;
					goto error;
				}
				
				n = okfs_argreadlink(rpath, buf, path_max);
				if (n < 0)
					goto error;
				buf[n] = '\0';

				if (!extra_buf)
					extra_buf = alloca(path_max);

				len = strlen (end);
				if ((long int) (n + len) >= path_max){
					errno = ENAMETOOLONG;
					goto error;
				}

				/* Careful here, end may be a pointer into 
				   extra_buf... */
				okfs_memmove(&extra_buf[n], end, len + 1);
				name = end = okfs_memcpy(extra_buf, buf, n);
				
				if (buf[0] == '/')
					/* It's an absolute symlink */
					dest = rpath + 1;  
				else
					/* Back up to previous component, 
					   ignore if at root already: */
					if (dest > rpath + 1)
						while ((--dest)[-1] != '/');
			}
		}
	}
	if (dest > rpath + 1 && dest[-1] == '/')
		--dest;
	*dest = '\0';
	return rpath;

 error:
	free(rpath);
	return NULL;
}
