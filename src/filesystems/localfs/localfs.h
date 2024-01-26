/*$Id: localfs.h,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
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

#ifndef _LOCALFS_H
#define _LOCALFS_H

int localfs_mount(struct okfs_superblock*);

int localfs_lstat(struct okfs_superblock*, const char*, struct stat*);
int localfs_stat(struct okfs_superblock*, const char*, struct stat*);
int localfs_readlink(struct okfs_superblock*, const char*, char*, size_t);
int localfs_open(struct okfs_superblock*, const char*, int, mode_t);
int localfs_close(struct okfs_file*, int fd);
off_t localfs_lseek(struct okfs_file*, int fd, off_t offset, int whence);
int localfs_read(struct okfs_file*, int fd, void*, size_t);
int localfs_write(struct okfs_file*, int fd, void*, size_t);
int localfs_readdir(struct okfs_file*, int, struct okfs_dirent **);

int localfs_link(struct okfs_superblock*, const char *, const char *);
int localfs_unlink(struct okfs_superblock*, const char *);
int localfs_symlink(struct okfs_superblock*, const char *, const char *);
int localfs_access(struct okfs_superblock*, const char *, int);
int localfs_rename(struct okfs_superblock*, const char *, const char *);	

int localfs_rmdir(struct okfs_superblock*, const char *);
int localfs_mkdir(struct okfs_superblock*, const char *, mode_t);

int localfs_utime(struct okfs_superblock*, const char *, const struct utimbuf *);
int localfs_chmod(struct okfs_superblock*, const char *, mode_t);
int localfs_lchown(struct okfs_superblock*, char *, uid_t, gid_t);
int localfs_truncate(struct okfs_superblock*, const char *, off_t);
int localfs_statfs(struct okfs_superblock*, const char *, struct statfs *);
#endif
