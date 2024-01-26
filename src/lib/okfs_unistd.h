/*$Id: okfs_unistd.h,v 1.2 2006/05/06 22:14:46 jan_wrobel Exp $*/
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

#ifndef OKFS_UNISTD_H
#define OKFS_UNISTD_H

#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/utime.h>
#include <linux/stat.h>
#include <linux/dirent.h>
#include <asm/statfs.h>

/*
 * One of the reason why these routines aren't taken from glibc
 * is because glibc version of them uses different structures that one 
 * defined in Kernel. 
 */
int statfs(const char *path, struct statfs *buf); 
int stat(const char *file_name, struct stat *buf); 
int stat64(const char *file_name, struct stat64 *buf); 
int lstat(const char *file_name, struct stat *buf); 
int lstat64(const char *file_name, struct stat64 *buf); 


/*All above routines can be taken from glibc as well*/
gid_t getgid(void);
gid_t getegid(void);
gid_t getuid(void);
gid_t geteuid(void);

int unlink(const char *path); 
int rmdir(const char *path); 
int acct(const char *path); 
int close(int fd); 

int symlink(const char *oldpath, const char *newpath); 
int kill(pid_t pid, int sig); 
int utime(const char *path, const struct utimbuf *buf); 
int link(const char *oldpath, const char *newpath); 
int rename(const char *oldpath, const char *newpath); 
int access(const char *path, mode_t mode); 
int mkdir(const char *path, mode_t mode); 
int chmod(const char *path, mode_t mode); 
int truncate(const char *path, off_t len); 

int readlink(const char *path, char *buf, size_t bufsiz); 
int lchown(const char *path, uid_t owner, gid_t group); 
int chown(const char *path, uid_t owner, gid_t group); 
int mknod(const char *path, mode_t mode, dev_t dev); 
int open(const char *path, int flags, mode_t mode); 
int write(int fd, const void *buf, size_t count); 
int read(int fd, void *buf, size_t count); 
int lseek(int fd, off_t offset, int whence); 
int getdents64(int fd, struct dirent64 *dirp, int count); 


#endif
