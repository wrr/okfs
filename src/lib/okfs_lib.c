/*$Id: okfs_lib.c,v 1.2 2006/05/06 22:14:46 jan_wrobel Exp $*/
/*
 * Part of this file is from linux/lib/string.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

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
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include "okfs_lib.h"

/**
 * is_dir - Check if file is a directory.
 * @path: File location
 * returns: 0  on success, -1 on error
 */
int is_dir(const char *path)
{
	struct stat buf;
	if (stat(path, &buf) < 0)
		return -1;
	if (S_ISDIR(buf.st_mode)){
		errno = ENOTDIR;
		return -1;
	}
	return 0;	    
}

/**
 * id_from_pwd - Get uid and gid of a user from /etc/passwd.
 * @s: String with user's login or id
 * @uid: Pointer to user's uid obtained by function.
 * @gid: Pointer to user's gid obtained by function.
 * returns: 0  on success, -1 on error
 *
 * Uid or gid may be null.
 */
int id_from_pwd(char *s, int *uid, int *gid)
{
	struct passwd *passwd;
	char *e;
	long id = strtol(s, &e, 10);

	if (*s && *e == '\0')
		passwd = getpwuid(id);
	else
		passwd = getpwnam(s);
	
	if (!passwd)
		return -1;
	
	if (gid)
		*gid = passwd->pw_gid;
	if (uid)
		*uid = passwd->pw_uid;
	return 0;
}

/**
 * get_gid - Get gid of a group from /etc/group.
 * @s: String with group name or gid.
 * returns: gid on success, -1 on error.
 */
int get_gid(char *s)
{
	struct group *group;
	char *e;
	long id = strtol(s, &e, 10);

	if (*s && *e == '\0')
		group = getgrgid(id);
	else
		group = getgrnam(s);
	if (!group) 
		return -1;
	
	return group->gr_gid;
}


/**
 * memcpy - Copy one area of memory to another
 * @dest: Where to copy to
 * @src: Where to copy from
 * @count: The size of the area.
 */
void *okfs_memcpy(void *dest, const void *src, int count)
{
	char *tmp = (char *) dest, *s = (char *) src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}

void *okfs_mempcpy(void *dest, const void *src, int count)
{
	char *tmp = (char *) dest, *s = (char *) src;
	int n = count;
	
	while (n--)
		*tmp++ = *s++;

	return dest + count;
}

/**
 * memmove - Copy one area of memory to another
 * @dest: Where to copy to
 * @src: Where to copy from
 * @count: The size of the area.
 *
 * Unlike memcpy(), memmove() copes with overlapping areas.
 */
void *okfs_memmove(void * dest, const void *src, int count)
{
	char *tmp, *s;

	if (dest <= src) {
		tmp = (char *) dest;
		s = (char *) src;
		while (count--)
			*tmp++ = *s++;
		}
	else {
		tmp = (char *) dest + count;
		s = (char *) src + count;
		while (count--)
			*--tmp = *--s;
		}

	return dest;
}

