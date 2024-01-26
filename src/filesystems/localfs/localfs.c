/*$Id: localfs.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/

/*
 *  This file is part of Out of Kernel File System.
 *
 *  Copyright (C) 2004-2005 Jan Wrobel <wrobel@blues.ath.cx>
 *
 *  The idea of localfs comes from LUFS Copyright (C) 2002 Florin Malita
 *  See http:://lufs.sourceforge.net/
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
#include <linux/utime.h>
#include <linux/stat.h>
#include <linux/dirent.h>
#include <asm/statfs.h>
#include "okfs_unistd.h"
#include "okfs_lib.h"
#include "virtualfs.h"
#include "vfs_readdir.h"
#include "vfs_namei.h"
#include "localfs.h"
#include "debug.h"
#include "libc.h"
#include "wrap.h"


/*static char *newpath, *newpath2;
  static int newpath_len, newpath2_len;*/
static char *creat_path(struct okfs_superblock *fsi, const char *path)
{
	char *retval;
	int len;
	len = strlen(path) + fsi->fs_spec_len; 
	retval = Malloc(len + 1);
	strcpy(retval, fsi->fs_spec);
	strcat(retval, path);
	return retval;
}

int 
localfs_open(struct okfs_superblock *fsi, const char *path, int flags, mode_t mode)
{
	char *newpath;
	int ret;
	newpath = creat_path(fsi, path);
	ret = open(newpath, flags, mode);
	free(newpath);
	return (ret < 0) ? -errno : ret;
}

int localfs_rmdir(struct okfs_superblock *fsi, const char *path)
{
	char *newpath;
	int ret;
	newpath = creat_path(fsi, path);
	ret = rmdir(newpath);
	free(newpath);
	return (ret < 0) ? -errno : ret;
}

int localfs_mkdir(struct okfs_superblock *fsi, const char *path, mode_t mode)
{
	char *newpath;
	int ret;
	newpath = creat_path(fsi, path);
	ret = mkdir(newpath, mode);
	free(newpath);
	return (ret < 0) ? -errno : ret;
}

int localfs_lstat(struct okfs_superblock *fsi, const char *path, struct stat *buf)
{
	
	char *newpath;
	int ret;
	newpath = creat_path(fsi, path);
	ret = lstat(newpath, buf);
	free(newpath);
	return (ret < 0) ? -errno : ret;
}

int localfs_access(struct okfs_superblock *fsi, const char *path, int mode)
{
	char *newpath;
	int ret;
	newpath = creat_path(fsi, path);
	ret = access(newpath, mode);
	free(newpath);
	return (ret < 0) ? -errno : ret;
}

int localfs_close(struct okfs_file *fp, int fsfd)
{
	int ret;
	ret = close(fsfd);
	return (ret < 0) ? -errno : ret;
}

off_t localfs_lseek(struct okfs_file *fp, int fd, off_t offset, int whence)
{
	off_t ret;
	ret = lseek(fd, offset, whence);
	return (ret == (off_t)-1) ? -errno : ret;
}

int localfs_read(struct okfs_file *fp, int fd, void *buf, size_t count)
{
	int ret;
	ret = read(fd, buf, count);
	return (ret < 0) ? -errno : ret;
}

int localfs_readdir(struct okfs_file *fp, int fd, struct okfs_dirent **vfsent)
{
	struct dirent64 dtab[100], *dp;
	int res;
	char *p;
	
	for (; (res = getdents64(fd, dtab, sizeof(*dtab))) > 0; )
		for(p = (char *)dtab; p < (char *) dtab + res; ){
			dp = (struct dirent64 *)p;
			okfs_add_dirent(vfsent, dp->d_name, 
					dp->d_ino, dp->d_type);
			p += dp->d_reclen;
		}
	
	if (res < 0)
		return -errno;
	
	return res;
}

/*int localfs_getdents64(struct okfs_file *fp, int fd, struct dirent64 *dirp, 
		     unsigned int count)
{
	TRACE("path = %s count = %d fd = %d", fp->path, count, fd);
	return getdents64(fd, dirp, count);
}*/

int localfs_write(struct okfs_file *fp, int fd, void *buf, size_t count)
{
	int ret;
	ret = write(fd, buf, count);
	return (ret < 0) ? -errno : ret;
}

int localfs_readlink(struct okfs_superblock *fsi, const char *path, char *buf, 
		     size_t bufsize)
{
	char *newpath;
	int ret;
	newpath = creat_path(fsi, path);
	ret = readlink(newpath, buf, bufsize);
	free(newpath);
	return (ret < 0) ? -errno : ret;
}


int localfs_rename(struct okfs_superblock *fsi, const char *path1, const char *path2)
{
	char *newpath1, *newpath2;
	int ret;
	newpath1 = creat_path(fsi, path1);
	newpath2 = creat_path(fsi, path2);
	
	ret = rename(newpath1, newpath2);
	free(newpath1);
	free(newpath2);
	return (ret < 0) ? -errno : ret;
}

int localfs_link(struct okfs_superblock *fsi, const char *path1, const char *path2)
{
	char *newpath1, *newpath2;
	int ret;
	newpath1 = creat_path(fsi, path1);
	newpath2 = creat_path(fsi, path2);
	
	ret = link(newpath1, newpath2);
	free(newpath1);
	free(newpath2);
	return (ret < 0) ? -errno : ret;
}

int 
localfs_symlink(struct okfs_superblock *fsi, const char *path1, const char *path2)
{
	char *newpath2;
	int ret;
	newpath2 = creat_path(fsi, path2);
	ret = symlink(path1, newpath2);
	free(newpath2);
	return (ret < 0) ? -errno : ret;
}

int localfs_unlink(struct okfs_superblock *fsi, const char *path)
{
	char *newpath;
	int ret;
	newpath = creat_path(fsi, path);
	ret = unlink(newpath);
	free(newpath);
	return (ret < 0) ? -errno : ret;
}

int localfs_chmod(struct okfs_superblock *fsi, const char *path, mode_t mode)
{
	char *newpath;
	int ret;
	TRACE("");
	newpath = creat_path(fsi, path);
	ret = chmod(newpath, mode);
	free(newpath);
	return (ret < 0) ? -errno : ret;
}

int localfs_lchown(struct okfs_superblock *fsi, char *path, uid_t owner, gid_t group)
{
	char *newpath;
	int ret;
	newpath = creat_path(fsi, path);
	ret = chown(newpath, owner, group);
	free(newpath);
	return (ret < 0) ? -errno : ret;
}

int localfs_truncate(struct okfs_superblock *fsi, const char *path, off_t length)
{
	char *newpath;
	int ret;
	newpath = creat_path(fsi, path);
	ret = truncate(newpath, length);
	free(newpath);
	return (ret < 0) ? -errno : ret;
}

int localfs_statfs(struct okfs_superblock *fsi, const char *path, struct statfs *buf)
{
	char *newpath;
	int ret;
	newpath = creat_path(fsi, path);
	ret = statfs(newpath, buf);
	free(newpath);
	return (ret < 0) ? -errno : ret;
}

int 
localfs_utime(struct okfs_superblock *fsi, const char *path, const struct utimbuf *buf)
{
	char *newpath;
	int ret;
	newpath = creat_path(fsi, path);
	ret = utime(newpath, buf);
	free(newpath);
	return (ret < 0) ? -errno : ret;
}

/*check if file system specified in FS_SPEC can be mounted if so
  change FS_SPEC to canonical name.*/
int localfs_mount(struct okfs_superblock *mi)
{
	char *srcdir;

	if (mi->fs_spec[0] != '/'){
		err_msg("mount localfs from dir %s. Path should be absolute", 
			mi->fs_spec);
		return -1;	
	}
	if (strlen(mi->mnt_opts)){
		err_msg("Invalid option %s. Localfs doesn't support any options", mi->mnt_opts);
		return -1;
	}

	if (is_dir(mi->fs_spec) < 0){
		err_ret("can't mount localfs from dir %s", mi->fs_spec);
		return -1;
	}

	if (!(srcdir = canonicalize(NULL, mi->fs_spec))){
		err_ret("incorrect path name %s", mi->fs_spec);
		return -1;
	}
	free(mi->fs_spec);
	mi->fs_spec = srcdir;
	mi->fs_spec_len = strlen(srcdir);
	return 1;
}
