/*$Id: shfs.h,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
/*
 *  This is part of SHell File System.
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

#ifndef _SHFS_H
#define _SHFS_H

extern struct call_entry shfs_call[];

struct okfs_superblock;
int shfs_mount(struct okfs_superblock *);

int shfs_lstat(struct okfs_superblock*, const char*, struct stat*);
int shfs_rmdir(struct okfs_superblock*, const char *);
int shfs_mkdir(struct okfs_superblock*, const char *, mode_t);
int shfs_open(struct okfs_superblock*, const char*, int, mode_t);
int shfs_rename(struct okfs_superblock*, const char *, const char *);	
int shfs_readdir(struct okfs_file*, int, struct okfs_dirent **);
int shfs_readlink(struct okfs_superblock*, const char*, char*, size_t);
int shfs_read(struct okfs_file*, int fd, void*, size_t);
int shfs_write(struct okfs_file*, int fd, void*, size_t);
int shfs_truncate(struct okfs_superblock*, const char *, off_t);
int shfs_link(struct okfs_superblock*, const char *, const char *);
int shfs_symlink(struct okfs_superblock*, const char *, const char *);
int shfs_chmod(struct okfs_superblock*, const char *, mode_t);
int shfs_lchown(struct okfs_superblock*, char *, uid_t, gid_t);
int shfs_unlink(struct okfs_superblock*, const char *);
int shfs_utime(struct okfs_superblock*, const char *, const struct utimbuf *);
int shfs_statfs(struct okfs_superblock *, const char *, struct statfs *);

int shfs_close(struct okfs_file*, int fd);
off_t shfs_lseek(struct okfs_file*, int fd, off_t offset, int whence);
int shfs_access(struct okfs_superblock*, const char *, int);

#endif
