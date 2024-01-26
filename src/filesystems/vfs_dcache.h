/*$Id: vfs_dcache.h,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/

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

#ifndef DCACHE_H
#define DCACHE_H

/*size of hash tables*/
#define MAX_HASH 13277

#include <linux/stat.h>

/*
  For now OKFS caches content of directories and state structures of files. 
  It does not yet cache content of files.
  There is a lot of work to do here, like for example aging and removing 
  information.
 */


struct okfs_inode{
	/*stat structure of this file*/
	struct stat statbuf;
	/*where this file points to if it is a symlink*/
	char *symlink;
};

/*list of inodes in cache, accessed by a path to file*/
struct ipath_lhead{
	struct okfs_inode *ino;
	char *path;
	struct ipath_lhead *next;
};

/*list of inodes in cache, accessed by inodenr*/
struct inr_lhead{
	struct okfs_inode *ino;
	struct inr_lhead *next;
};

struct okfs_dentry{
	/*dirent structure with content of this directory*/
	struct okfs_dirent *d_dirent;
	/*directory name (not full path)*/
	char *d_name;
};

/*list of dentry structures in cache*/
struct okfs_dentry_lhead{
	struct okfs_dentry *entry;
	struct okfs_dentry_lhead *next;
};


struct okfs_superblock;


ino_t 
get_ino(struct okfs_superblock *fsp, const char *path);

ino_t 
get_parent_ino(struct okfs_superblock *fsp, char *path);

struct okfs_inode *
cache_find_inode(struct okfs_superblock *fsp, const char *path, int len);

struct okfs_inode *
cache_add_inode(struct okfs_superblock *fsp,  const char *path, int pathlen,
		const struct stat *statbuf, const char *symlink);

struct okfs_dirent*
cache_find_dirent(struct okfs_superblock *fsp, char *path);


struct okfs_dirent*
cache_add_dirent(struct okfs_superblock *fsp, const char *path, 
		 struct okfs_dirent *dp);

void
cache_add_entry(struct okfs_superblock *fsp, char *path);

void
cache_remove_entry(struct okfs_superblock *fsp, char *path);

void
change_file_size(struct okfs_superblock *fsp, const char *path, off_t size);
#endif
