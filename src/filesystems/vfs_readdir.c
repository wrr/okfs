/*$Id: vfs_readdir.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/

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

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/dirent.h>
#include "okfs_unistd.h"
#include "okfs_lib.h"
#include "task.h"
#include "virtualfs.h"
#include "vfs_readdir.h"
#include "debug.h"
#include "libc.h"
#include "wrap.h"
#include "memacc.h"

/*Routines to read directories' contents*/

/*
  Add infromation about file in directory.
  @head - dirent structure to which information will be added
  @name - file name
  @ino - inode number of this file
  @d_type - type of file or DT_UNKNOWN
 */
void
okfs_add_dirent(struct okfs_dirent **head, const char *name, 
	       ino_t ino, unsigned int d_type)
{
	struct okfs_dirent *new_ent;
	new_ent= (struct okfs_dirent *) Malloc(sizeof(struct okfs_dirent));
	new_ent->name = Strdup(name);
	new_ent->ino = ino; 
	new_ent->type = d_type;
	new_ent->name_len = strlen(name);
	new_ent->next_dir = *head;
	*head = new_ent;
}

static int okfs_do_readdir(int fd, struct okfs_dirent **dpp)
{
	int ret = 0;
	struct okfs_superblock *fsp;
	struct okfs_file *filep = slave->fd_tab[fd];
	
	ASSERT(filep);
	ASSERT(filep->fsp);
	fsp = filep->fsp;
	
	if (!fsp->entry->readdir)
		return -EOPNOTSUPP;
	
	if (!(*dpp = cache_find_dirent(fsp, filep->path))){
		if ((ret = fsp->entry->readdir(filep, slave->fsfd[fd], dpp)) 
		    != 0)
			return ret;
		cache_add_dirent(fsp, filep->path, *dpp);
	}
	
	return 0;
}

/*
  Creates dirent64 structure (defined in kernel headers) from
  okfs_dirent structure
  @start - offset from which copying of directory is started
  @count - number of bytes to copy
*/
static int creat_dirent64(struct okfs_dirent *okfs_dir, void *dir64,
			  loff_t start, unsigned int count)
{
	loff_t i = 0;
	unsigned int size = 0;
	int n;
	struct dirent64 newdir;
	
	while(okfs_dir && i < start){
		i += okfs_dir->name_len;
		okfs_dir = okfs_dir->next_dir;
	}
	
	if (!okfs_dir)
		return 0;
	
	while(okfs_dir){
		newdir.d_ino = okfs_dir->ino;
		newdir.d_off = i;
		newdir.d_type = okfs_dir->type;
		n = MIN(256, okfs_dir->name_len + 1);
		strncpy(newdir.d_name, okfs_dir->name, n - 1);
		newdir.d_name[n - 1] = 0;
		
		newdir.d_reclen = n + sizeof(newdir.d_ino) + 
			sizeof(newdir.d_off) + sizeof(newdir.d_type)
			+ sizeof(newdir.d_reclen);
		if (newdir.d_reclen + size > count)
			break;
		okfs_memcpy(dir64, &newdir, newdir.d_reclen);
		size += newdir.d_reclen;
		dir64 += newdir.d_reclen;
		i += okfs_dir->name_len;
		okfs_dir = okfs_dir->next_dir;
	}
	
	if (!size)
		return -EINVAL;
	return size;
}

/*
  the same as create_dirent64 but creates dirent structure
*/
static int creat_dirent(struct okfs_dirent *okfs_dir, void *dir,
			loff_t start, unsigned int count)
{
	loff_t i = 0;
	unsigned int size = 0;
	int n;
	struct dirent newdir;
	
	while(okfs_dir && i < start){
		i += okfs_dir->name_len;
		okfs_dir = okfs_dir->next_dir;
	}
	
	if (!okfs_dir)
		return 0;
	
	while(okfs_dir){
		newdir.d_ino = okfs_dir->ino;
		newdir.d_off = i;
		n = MIN(256, okfs_dir->name_len + 1);
		strncpy(newdir.d_name, okfs_dir->name, n - 1);
		newdir.d_name[n - 1] = 0;
		
		newdir.d_reclen = n + sizeof(newdir.d_ino) + 
			sizeof(newdir.d_off) //+ sizeof(newdir.d_type)
			+ sizeof(newdir.d_reclen);
		if (newdir.d_reclen + size > count)
			break;
		okfs_memcpy(dir, &newdir, newdir.d_reclen);
		size += newdir.d_reclen;
		dir += newdir.d_reclen;
		i += okfs_dir->name_len;
		okfs_dir = okfs_dir->next_dir;
	}
	
	if (!size)
		return -EINVAL;
	return size;
}


void okfs_getdents64(void)
{
	int fd = ARG1;
	struct dirent64 *dp64;
	unsigned int count = ARG3;
	int ret = -EINVAL;
	struct okfs_superblock *fsp;
	struct okfs_dirent *dp = NULL;
	
	fsp = slave->fd_tab[fd]->fsp;
	
	if (!count)
		goto error;

	
	if ((ret = okfs_do_readdir(fd, &dp)) != 0)
		goto error;

	dp64 = (struct dirent64 *)Malloc(count);	
	
	if ((ret = creat_dirent64(dp, dp64, slave->fd_tab[fd]->f_pos, count))
	    > 0){
		slave_memset((void *)ARG2, dp64, ret); 	
		slave->fd_tab[fd]->f_pos += ret;
	}
	free(dp64);
 error:	
	set_slave_return(ret);	
}


void okfs_getdents(void)
{
	int fd = ARG1;
	struct dirent *dirent;
	unsigned int count = ARG3;
	int ret = -EINVAL;
	struct okfs_superblock *fsp;
	struct okfs_dirent *dp = NULL;
	
	fsp = slave->fd_tab[fd]->fsp;
	
	if (!count)
		goto error;

	
	if ((ret = okfs_do_readdir(fd, &dp)) != 0)
		goto error;

	dirent = (struct dirent *)Malloc(count);	
	
	if ((ret = creat_dirent(dp, dirent, slave->fd_tab[fd]->f_pos, count))
	    > 0){
		slave_memset((void *)ARG2, dirent, ret); 	
		slave->fd_tab[fd]->f_pos += ret;
	}
	free(dirent);
 error:	
	set_slave_return(ret);	

}

