/*$Id: okfs_unistd.c,v 1.2 2006/05/06 22:14:46 jan_wrobel Exp $*/
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
#include <linux/unistd.h>
#include <linux/utime.h>
#include <linux/stat.h>
#include <linux/dirent.h>
#include <asm/statfs.h>
#include "okfs_unistd.h"
/*
 * Automatic function generators:
 */
_syscall0(uid_t, getuid);
_syscall0(uid_t, geteuid);
_syscall0(gid_t, getgid);
_syscall0(gid_t, getegid);


_syscall1(int, unlink, const char *, path); 
_syscall1(int, rmdir, const char *, path); 
_syscall1(int, acct, const char *, path); 
_syscall1(int, close, int, fd); 

_syscall2(int, symlink, const char *, oldpath, const char*, newpath); 
_syscall2(int, kill, pid_t, pid, int, sig); 
_syscall2(int, utime, const char *, path, const struct utimbuf*, buf); 
_syscall2(int, link, const char *, oldpath, const char*, newpath); 
_syscall2(int, rename, const char *, oldpath, const char*, newpath); 
_syscall2(int, access, const char *, path, mode_t, mode); 
_syscall2(int, mkdir, const char *, path, mode_t, mode); 
_syscall2(int, chmod, const char *, path, mode_t, mode); 
_syscall2(int, truncate, const char *, path, off_t, len); 
_syscall2(int, statfs, const char *, path, struct statfs *, buf); 
_syscall2(int, stat, const char *, file_name, struct stat *, buf); 
_syscall2(int, stat64, const char *, file_name, struct stat64 *, buf); 
_syscall2(int, lstat, const char *, file_name, struct stat *, buf); 
_syscall2(int, lstat64, const char *, file_name, struct stat64 *, buf); 

_syscall3(int, readlink, const char *, path, char*, buf, size_t, bufsiz); 
_syscall3(int, lchown, const char *, path, uid_t, owner, gid_t, group); 
_syscall3(int, chown, const char *, path, uid_t, owner, gid_t, group); 
_syscall3(int, mknod, const char *, path, mode_t, mode, dev_t, dev); 
_syscall3(int, open, const char*, path, int, flags, mode_t, mode); 
_syscall3(int, write, int, fd, const void *, buf, size_t, count); 
_syscall3(int, read, int, fd, void *, buf, size_t, count); 
_syscall3(int, lseek, int, fd, off_t, offset, int, whence); 
_syscall3(int, getdents64, int, fd, struct dirent64 *, dirp, int, count); 

