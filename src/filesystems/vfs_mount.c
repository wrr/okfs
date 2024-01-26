/*$Id: vfs_mount.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/

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

#include "vfs_mount.h"
#include "debug.h"
#include "wrap.h"
#include "libc.h"
#include "okfs_lib.h"
#include "virtualfs.h"
#include "vfs_namei.h"
#include "localfs/localfs.h"
#include "shfs/shfs.h"

struct okfs_superblock *mounted_fs;

/*pointers to filesystem functions responsible for handling calls*/
struct fs_entry_point fs_table[]={
	{"localfs",
	 mount: localfs_mount,
	 lstat: localfs_lstat,
	 readlink: localfs_readlink,
	 readdir: localfs_readdir,
	 open: localfs_open,
	 close: localfs_close,
	 lseek: localfs_lseek,
	 read: localfs_read,
	 write: localfs_write,
	 link: localfs_link,
	 unlink: localfs_unlink,
	 access: localfs_access,
	 rename: localfs_rename,
	 mkdir: localfs_mkdir,
	 rmdir: localfs_rmdir,
	 symlink: localfs_symlink,
	 utime: localfs_utime,
	 chmod: localfs_chmod,
	 truncate: localfs_truncate,
	 lchown: localfs_lchown,
	 statfs: localfs_statfs,
	},
	{"shfs", 
	 mount: shfs_mount, 
	 lstat: shfs_lstat, 
	 mkdir: shfs_mkdir,
	 rmdir: shfs_rmdir,
	 rename: shfs_rename,	
	 readdir: shfs_readdir,
	 open: shfs_open,
	 read:shfs_read,
	 write: shfs_write,
	 readlink: shfs_readlink,
	 truncate: shfs_truncate,
	 link: shfs_link,
	 symlink: shfs_symlink,
	 unlink: shfs_unlink,
	 chmod: shfs_chmod,
	 lchown: shfs_lchown,
	 utime: shfs_utime,
	 statfs: shfs_statfs,
	 }, 
	{0, 0},
};

/*
 *Add new mount point. See man getmntent for details about arguments
 *of this function
 */
void add_entry_point(char *mnt_fsname, char *mnt_dir, char *mnt_type, 
		     char *mnt_opts)
{
	struct okfs_superblock *new_pointp;
	int fsidx;

	for(fsidx = 0; fs_table[fsidx].fs_type; fsidx++)
		if (!strcmp(fs_table[fsidx].fs_type, mnt_type))		    
			break;
		
	if (!fs_table[fsidx].fs_type){
		err_msg("unknown filesystem type %s", mnt_type);	       
		return;
	}

	if (mnt_dir[0] != '/'){
		err_msg("can't mount %s file system to %s: directory path is not absolute", mnt_type, mnt_dir);
		return;
	}	
	
	if (!strlen(mnt_fsname)){
		err_msg("can't mount %s file system to %s: fs_spec is an empty \
string", mnt_type, mnt_dir);
		return;
	}
	
	if (is_dir(mnt_dir) < 0){
		err_ret("can't mount %s to %s", fs_table[fsidx].fs_type, 
			mnt_dir);
		return;
	}
	
	new_pointp = (struct okfs_superblock *)Malloc( sizeof(struct okfs_superblock));
	new_pointp->mnt_point = canonicalize(NULL, mnt_dir);
	if (!new_pointp->mnt_point){
		err_ret("canonicalize of path %s error", mnt_dir);
		free(new_pointp);
		return;
	}
	new_pointp->mnt_point_len = strlen(new_pointp->mnt_point);
	new_pointp->fs_spec = Strdup(mnt_fsname);
	new_pointp->fs_spec_len = strlen(new_pointp->fs_spec);
	new_pointp->entry = &fs_table[fsidx];
	new_pointp->mnt_opts = Strdup(mnt_opts);
	memset(new_pointp->ipath_cache, 0, 
	       MAX_HASH * sizeof(struct ipath_lhead*));
	memset(new_pointp->dcache, 0,
	       MAX_HASH * sizeof(struct okfs_dentry_lhead*));
	memset(new_pointp->inr_cache, 0, 
	       MAX_HASH * sizeof(struct inr_lhead*));

	if (new_pointp->entry->mount(new_pointp) < 0){
		free(new_pointp->mnt_opts);
		free(new_pointp->fs_spec);
		free(new_pointp->mnt_point);
		free(new_pointp);
		return;
	}		
	
	new_pointp->next_fs = mounted_fs;
	mounted_fs = new_pointp; 
}

