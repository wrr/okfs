/*$Id: vfs_stat.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
 
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

#include <linux/errno.h>
#include "task.h"
#include "libc.h"
#include "debug.h"
#include "memacc.h"
#include "okfs_unistd.h"
#include "vfs_stat.h"

/*States of files*/


/*
stat to stat64 converter.
It is quite brutal and I don't think that 100% correct
*/
static void creat_stat64(const struct stat *buf, struct stat64 *buf64)
{
	memset(buf64, 0, sizeof(struct stat64));
	buf64->st_dev = buf->st_dev;/*huge_encode_dev(buf->dev);*/
	buf64->st_rdev = buf->st_rdev; /*huge_encode_dev(buf->rdev);*/
	buf64->st_ino = buf->st_ino;
	buf64->st_mode = buf->st_mode;
	buf64->st_nlink = buf->st_nlink;
	buf64->st_uid = buf->st_uid;
	buf64->st_gid = buf->st_gid;
	buf64->st_atime = buf->st_atime;
	//buf64->st_atime_nsec = buf->st_atime.tv_nsec;
	buf64->st_mtime = buf->st_mtime;
	//buf64->st_mtime_nsec = buf->st_mtime.tv_nsec;
	buf64->st_ctime = buf->st_ctime;
	//buf64->st_ctime_nsec = buf->st_ctime.tv_nsec;
	buf64->st_size = buf->st_size;
	buf64->st_blocks = buf->st_blocks;
	buf64->st_blksize = buf->st_blksize;
	/*TRACE("values = dev = %llu rdev = %llu ino = %llu mode = %u\n nlink = %u uid = %u\n gid = %u atime = %u mtime = %u\n ctime = %u size = %lld blosks = %u block_size = %u\n", 
	      buf64->st_dev,
	      buf64->st_rdev,
	      buf64->st_ino,
	      buf64->st_mode,
	      buf64->st_nlink,
	      buf64->st_uid,
	      buf64->st_gid,
	      buf64->st_atime,
	      buf64->st_mtime,
	      buf64->st_ctime,
	      buf64->st_size,
	      buf64->st_blocks,
	      buf64->st_blksize);*/
}

static int 
okfs_dolstat(struct okfs_superblock *fsp, const char *fullpath, const char *path, 
	    struct stat *buf)
{
	struct okfs_inode *ip;
	int ret;
	
	if (!fsp->entry->lstat)
		return -EOPNOTSUPP;
	
	TRACE("searching for %s", fullpath);
	if ((ip = cache_find_inode(fsp, fullpath, strlen(fullpath))))
		*buf = ip->statbuf;
	else{
		TRACE("not found");
		ret = fsp->entry->lstat(fsp, path, buf);
		if (ret < 0)
			return ret;
		if (!cache_find_inode(fsp, fullpath, strlen(fullpath)))
			cache_add_inode(fsp, fullpath, strlen(fullpath), 
					buf, NULL);
	}
	return 0;
}

void okfs_fstat(void)
{
	struct stat buf;
	struct okfs_file *fdp = slave->fd_tab[ARG1];
	int ret;
	ret = okfs_dolstat(fdp->fsp, fdp->path, fdp->rel_path, &buf);
	if (ret == 0)
		slave_memset((void *)ARG2, &buf, sizeof(buf));
	
	set_slave_return(ret);
}

void okfs_fstat64(void)
{
	struct stat buf;
	struct stat64 buf64;
	struct okfs_file *fdp = slave->fd_tab[ARG1];
	int ret;

	if ((ret = okfs_dolstat(fdp->fsp, fdp->path, fdp->rel_path, &buf)) 
	    == 0){
		creat_stat64(&buf, &buf64);
		TRACE("creat_stat %ld ->%ld %d", 
		      (long)buf.st_ino, (long)buf64.st_ino, sizeof(buf64));
		ASSERT(slave->rpl_code == NULL);
		slave_memset((void *)ARG2, &buf64, sizeof(buf64));
		slave_memget(&buf64, (void *)ARG2, sizeof(buf64));
		TRACE("after get %ld ->%ld %d", 
		      (long)buf.st_ino, (long)buf64.st_ino, sizeof(buf64));
	}
	set_slave_return(ret);
}

void okfs_stat(void)
{
	struct okfs_superblock *fsp;
	struct stat buf;
	int ret;
	
	if (proc_path1(READ_LAST_LINK) < 0){
		ret = -errno;
		goto end;
	}
	
	if ((fsp = slave->callfs.arg1_fsp))
		ret = okfs_dolstat(fsp, slave->callfs.canon_path1, 
				  slave->callfs.rel_path1, &buf);
	else{
		ret = stat(slave->callfs.canon_path1, &buf);
		if (ret < 0)
			ret = -errno;
	}
	
	if (ret == 0)
		slave_memset((void *)ARG2, &buf, sizeof(buf));
	
 end:
	set_slave_return(ret);
	clean_path1();
}

void okfs_lstat(void)
{
	struct okfs_superblock *fsp;
	struct stat buf;
	int ret;
	
	if (proc_path1(!READ_LAST_LINK) < 0){
		TRACE("error");
		ret = -errno;
		goto end;
	}

	if ((fsp = slave->callfs.arg1_fsp))
		ret = okfs_dolstat(fsp, slave->callfs.canon_path1, 
				  slave->callfs.rel_path1, &buf);
	else{
		ret = lstat(slave->callfs.canon_path1, &buf);
		if (ret < 0)
			ret = -errno;
	}
	
	if (ret == 0)
		slave_memset((void *)ARG2, &buf, sizeof(buf));
	
 end:
	set_slave_return(ret);
	clean_path1();
}

void okfs_stat64(void)
{
	struct okfs_superblock *fsp;
	struct stat64 buf64;
	struct stat buf;
	int ret;
	
	if (proc_path1(READ_LAST_LINK) < 0){
		TRACE("forcing error %s", strerror(errno));
		ret = -errno;
		goto end;
	}

	if ((fsp = slave->callfs.arg1_fsp)){
		ret = okfs_dolstat(fsp, slave->callfs.canon_path1, 
				  slave->callfs.rel_path1, &buf);
		if (ret == 0)
			creat_stat64(&buf, &buf64);
	}
	else{
		ret = stat64(slave->callfs.canon_path1, &buf64);
		if (ret < 0)
			ret = -errno;
	}
	
	if (ret == 0)
		slave_memset((void *)ARG2, &buf64, sizeof(buf64));
	
 end:
	set_slave_return(ret);
	clean_path1();
}

void okfs_lstat64(void)
{
	struct okfs_superblock *fsp;
	struct stat64 buf64;
	struct stat buf;
	int ret;

	if (proc_path1(!READ_LAST_LINK) < 0){
		TRACE("error");
		ret = -errno;
		goto end;
	}

	if ((fsp = slave->callfs.arg1_fsp)){
		ret = okfs_dolstat(fsp, slave->callfs.canon_path1, 
				  slave->callfs.rel_path1, &buf);
		if (ret == 0)
			creat_stat64(&buf, &buf64);
	}
	else{
		ret = lstat64(slave->callfs.canon_path1, &buf64);
		if (ret < 0)
			ret = -errno;
	}
	
	if (ret == 0)
		slave_memset((void *)ARG2, &buf64, sizeof(buf64));
	
 end:
	
	set_slave_return(ret);
	clean_path1();
	TRACE("returning %d", ret);
}

/*Like kernel lstat, but operates also on files visible only for OKFS.*/
int okfs_arglstat(const char *path, struct stat *buf)
{
	struct okfs_superblock *fsp;
	int ret;
	ASSERT(path);
	for(fsp = mounted_fs; fsp; fsp = fsp->next_fs)
		if (strncmp(fsp->mnt_point, path, fsp->mnt_point_len) == 0)
			break;
	
	if (!fsp){
		TRACE("lstating %s", path);
		return lstat(path, buf);
	}
	TRACE("path = %s, new_path = %s, path len = %d, mnt point len = %d",
	      path, path + fsp->mnt_point_len, strlen(path), 
	      fsp->mnt_point_len);
	
	ret = okfs_dolstat(fsp, path, path + fsp->mnt_point_len, buf);
	if (ret < 0){
		errno = -ret;
		ret= -1;
	}
	return ret;
}
