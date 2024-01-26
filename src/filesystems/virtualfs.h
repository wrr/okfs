/*$Id: virtualfs.h,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/

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

#ifndef VIRTUALFS_H
#define VIRTUALFS_H
#include <asm/statfs.h>
#include <entry.h>
#include "vfs_dcache.h"

struct okfs_file_lhead;
struct okfs_inode_lhead;
struct dir_lhead;

/*Infromation about mounted filesystems*/
struct okfs_superblock{
	/*a pointer to file system specific structure, used to store
	 *information needed by this file system*/
	void *fsdata;
	
	/*next mounted filesystem*/
	struct okfs_superblock *next_fs;
	char *mnt_point;
	
	/*first field in fstab*/
	char *fs_spec;
	
	/*options field in fstab*/
	char *mnt_opts;
	
	int mnt_point_len, fs_spec_len; 
	
	/*Hash tables with pointers to inode (hashed using one of the path to
	  this inode)*/
	struct ipath_lhead* ipath_cache[MAX_HASH];
	struct okfs_dentry_lhead* dcache[MAX_HASH];
	
	/*Hash table with pointers to inodes (hashed by inode nr and dev_nr).
	 */
	struct inr_lhead* inr_cache[MAX_HASH];
	
	/*table of calls used by file system
	 *specified by this mount_point:*/
	struct fs_entry_point *entry; 
};

extern struct okfs_superblock *mounted_fs;
struct okfs_file;
struct dirent64;
struct utimbuf;

/*calls that should be implemented by a filesystem*/
struct fs_entry_point{
	/*name of a filesystem*/
	char *fs_type;
	int (*mount)(struct okfs_superblock*);
	int (*lstat)(struct okfs_superblock*, const char*, struct stat*);
	int (*readlink)(struct okfs_superblock*, const char*, char*, size_t);
	int (*open)(struct okfs_superblock*, const char*, int, mode_t);
	int (*readdir)(struct okfs_file*, int, struct okfs_dirent **);
	int (*close)(struct okfs_file*,int);
	off_t (*lseek)(struct okfs_file*, int, off_t offset, int whence);
	int (*read)(struct okfs_file*, int, void*, size_t);
	int (*write)(struct okfs_file*, int, void*, size_t);
	int (*link)(struct okfs_superblock*, const char *, const char *);
	int (*unlink)(struct okfs_superblock*, const char *);
	int (*access)(struct okfs_superblock*, const char *, int);
	int (*rename)(struct okfs_superblock*, const char *, const char *);
	int (*rmdir)(struct okfs_superblock*, const char *);
	int (*mkdir)(struct okfs_superblock*, const char *, mode_t);
	int (*symlink)(struct okfs_superblock*, const char *, const char *);
	int (*utime)(struct okfs_superblock*, const char *, const struct utimbuf *);
	int (*chmod)(struct okfs_superblock*, const char *, mode_t);
	int (*truncate)(struct okfs_superblock*, const char *, off_t);
	int (*statfs)(struct okfs_superblock*, const char *, struct statfs *);
	int (*lchown)(struct okfs_superblock*, char *, uid_t, gid_t);
};

extern struct fs_entry_point fs_table[];


/*data that is shared between desriptors after dup or fork call*/
struct okfs_file{
	
	/*path to file pointed by file descriptor(if any)*/
	char *path;
	/*path to this file from mount point of fs responsible for this file*/
	char *rel_path;	
	struct okfs_inode *ino; /*inode for this file*/
	
	/*pointer to file system responsible for this file
	  NULL means that it is managed by OS*/
	struct okfs_superblock *fsp; 
		
	/*read/write position*/
	loff_t	f_pos;

	/*how many descriptors are pointing this file*/
	int nshared;
};


/*Information needed to resolve which filesystem is responsible
  for current filesystem related call with path as an argument*/
struct fs_resolve{
	/*pointer to file system specified by first argument of
	  system call*/
        struct okfs_superblock *arg1_fsp;
	/*pointer to file system specified by second argument of
	  system call (if there is any as for example in link call)*/
	struct okfs_superblock *arg2_fsp;
	
	/*path specified by first argument of system call*/
	char *path1;
	int path1_len;
	/*path specified by second argument of system call*/
	char *path2;
	int path2_len;
	
	/*arg1_path and arg2_path after canonicalization:*/
	char *canon_path1, *canon_path2;
	int canon_path1_len, canon_path2_len;
	
	/*arg1_fsp->mnt_point + arg1_realtive_path = arg1_canon_path
	  example: if process calls rmdir(/home/mnt/shfs/../shfs/hello.cpp)
	  and mount point for shfs is /home/mnt/shfs then:
	  arg1_path is /home/mnt/shfs/../shfs/hello.cpp,
	  arg1_canon_path is /home/mnt/shfs/hello.cpp
	  and arg1_relative_path is hello.cpp*/
	char *rel_path1, *rel_path2;
	int rel_path1_len, rel_path2_len;		
};

#define READ_LAST_LINK 1
int proc_path1(int option);
void clean_path1(void);
void okfs_before_lnk_rnm(void);
int okfs_argreadlink(const char *, char*, size_t);
void okfs_fd_call(void);
void okfs_op_on_fd(void);
void okfs_only_os(void);
struct task;
void okfs_close_fd(struct task *, int);

/*filesystem calls supported by okfs*/
void okfs_getcwd(void);
void okfs_chdir(void);
void okfs_fchdir(void);

int okfs_arglstat(const char *, struct stat *);
int okfs_argstat(const char *path, struct stat *buf);
int okfs_argreadlink(const char *path, char *buf, size_t bufsiz);
void okfs_readlink(void);


void okfs_before_close(void);
void okfs_after_close(void);
void okfs_read(void);
void okfs_write(void);
void okfs_pipe(void);
void okfs_link(void);
void okfs_unlink(void);
void okfs_symlink(void);
void okfs_rename(void);
void okfs_lseek(void);
void okfs_access(void);

void okfs_dup(void);
void okfs_dup2(void);
void okfs_before_fcntl(void);
void okfs_before_mmap(void);
void okfs_after_mmap(void);

void okfs_mkdir(void);
void okfs_rmdir(void);
void okfs_utime(void);
void okfs_chmod(void);
void okfs_truncate(void);
void okfs_chown(void);
void okfs_lchown(void);

void okfs_ftruncate(void);
void okfs_fchmod(void);
void okfs_fchown(void);


void okfs_acct(void);
void okfs_mknod(void);
void okfs_statfs(void);
void okfs_fstatfs(void);


#endif

