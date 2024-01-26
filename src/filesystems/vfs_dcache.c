/*$Id: vfs_dcache.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/

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

#include <linux/fs.h>
#include <linux/dcache.h>
#include "vfs_dcache.h"
#include "virtualfs.h"
#include "vfs_readdir.h"
#include "task.h"
#include "debug.h"
#include "libc.h"
#include "wrap.h"


/*compute hash from a file name*/
static inline unsigned int
name_hash(const unsigned char *name, unsigned int len)
{
	/*defined in kernel*/
	return full_name_hash(name, len) % MAX_HASH;
}

/*
  Find inode corresponding to a given file.
 */
struct okfs_inode *
cache_find_inode(struct okfs_superblock *fsp, const char *path, int len)
{
	int hash = name_hash(path, len);
	struct ipath_lhead *head= fsp->ipath_cache[hash];
	while(head){
		if (strcmp(head->path, path) == 0)
			break;
		head = head->next;
	}
	if (head)
		return head->ino;
	
	return NULL;
}


/*get inode number of a directory in which given file is located*/
ino_t 
get_parent_ino(struct okfs_superblock *fsp, char *path)
{
	char *sp, *newpath;
	struct okfs_inode *inop;
	struct stat buf;
	ino_t ret;
	
	if (strcmp(path, "/") == 0){
		inop = cache_find_inode(fsp, path, strlen(path));
		ASSERT(inop);
		ret = inop->statbuf.st_ino;
	}
	else{
		/*example: /usr/sbin -> /usr*/
		sp = strrchr(path, '/');
		ASSERT(sp);
		*sp = 0;
		if (strlen(path) == 0)
			newpath = "/";
		else
			newpath = path;
		
		TRACE("trying to find %s", newpath);
		
		inop = cache_find_inode(fsp, newpath, strlen(newpath));
		if (inop)
			ret = inop->statbuf.st_ino;
		else{
			okfs_arglstat(newpath, &buf);
			ret = buf.st_ino;
		}
			
		*sp = '/';
	}
	return ret;
}

/*
  get inode number of a file on a given filesystem.
 */
inline ino_t 
get_ino(struct okfs_superblock *fsp, const char *path)
{
	struct okfs_inode *inop = cache_find_inode(fsp, path, strlen(path));
	ASSERT(inop);
	return inop->statbuf.st_ino;
}

/*
  add to cache path to new inode.
*/
static inline void
path_insert(struct okfs_superblock *fsp, const char *path, int pathlen, 
	    struct okfs_inode *inop)
{
	int hash = name_hash(path, pathlen);	
	struct ipath_lhead *phead = 
		(struct ipath_lhead *) Malloc(sizeof(struct ipath_lhead));
	phead->path = Strdup(path);
	phead->ino = inop;
	phead->next = fsp->ipath_cache[hash];
	fsp->ipath_cache[hash] = phead;
}

/*remove from cache path to inode*/
static inline void
path_remove(struct okfs_superblock *fsp, const char *path)
{
	int hash = name_hash(path, strlen(path));
	struct ipath_lhead *p1, *p2;
	if (strcmp(fsp->ipath_cache[hash]->path, path) == 0){
		p1 = fsp->ipath_cache[hash];
		fsp->ipath_cache[hash] = p1->next;
		free(p1->path);
		free(p1);
	}
	else{
		p2 = fsp->ipath_cache[hash]->next;
		while(p2 && strcmp(p2->path, path) != 0){
			p1 = p2;
			p2 = p2->next;
		}
		if (p2){
			p1->next = p2->next;
			free(p2->path);
			free(p2);
		}
	}
}

/*find if in cache is inode with the same device and inode number*/
static struct okfs_inode *
same_ino_find(struct okfs_superblock *fsp, dev_t dev, ino_t ino)
{
	int hash = dev * ino % MAX_HASH;
	struct inr_lhead *head = fsp->inr_cache[hash];
	while(head){
		if (head->ino->statbuf.st_ino == ino && 
		    head->ino->statbuf.st_dev == dev)
			break;
		head = head->next;
	}
	if (head)
		return head->ino;
	return NULL;
}

/*creates new inode and stores it in cache*/
static struct okfs_inode *
creat_inode(struct okfs_superblock *fsp, const struct stat *buf, const char *symlnk)
{
	int hash = (buf->st_dev * buf->st_ino) % MAX_HASH;
	struct inr_lhead *newi;
	TRACE("new inode hash = %d", hash);
	newi = (struct inr_lhead *) Malloc(sizeof(struct inr_lhead));
	newi->ino = (struct okfs_inode *) Malloc(sizeof(struct okfs_inode));
	newi->ino->statbuf = *buf;
	if (symlnk)
		newi->ino->symlink = Strdup(symlnk);
	else 
		newi->ino->symlink = NULL;
	newi->next = fsp->inr_cache[hash];
	fsp->inr_cache[hash] = newi;
	return newi->ino;
}

/*
  Add information about inode to cache. Check if file related to this
  inode is already in cache (with different path) if so, don't create
  new inode for it, but use existing one.
  Caller should make shure that this inode is not already in cache
  with the same path (cache_find_inode()).
  pathlen == strlen(path);
*/
struct okfs_inode *
cache_add_inode(struct okfs_superblock *fsp, const char *path, int pathlen,
		const struct stat *statbuf, const char *symlink)
{
	struct okfs_inode *inop; 

	if (!(inop = same_ino_find(fsp, statbuf->st_dev, statbuf->st_ino)))
		inop = creat_inode(fsp, statbuf, symlink);
	
	path_insert(fsp, path, pathlen, inop);
	TRACE("inserting inode %s", path);
	return inop;
}

/*
  Retrieve dentry structure, corresponding to directory pointed by @path,
  if it is in chache.
 */
static struct okfs_dentry*
cache_find_dentry(struct okfs_superblock *fsp, char *path)
{
	int hash = name_hash(path, strlen(path));
	struct okfs_dentry_lhead *head= fsp->dcache[hash];
	while(head){
		if (strcmp(head->entry->d_name, path) == 0)
			break;
		head = head->next;
	}
	if (head)
		return head->entry;
	
	return NULL;
}

/*
  Retrieve dirent structure, corresponding to directory pointed by @path,
  if it is in chache.
 */
inline struct okfs_dirent*
cache_find_dirent(struct okfs_superblock *fsp, char *path)
{
	
	struct okfs_dentry *dp= cache_find_dentry(fsp, path);
	return (dp ? dp->d_dirent : NULL);
}

/*Insert information about directory located at @path, on filesystem
  pointed by @fsp to cache.
*/
struct okfs_dirent*
cache_add_dirent(struct okfs_superblock *fsp, const char *path, struct okfs_dirent *dp)
{
	int hash = name_hash(path, strlen(path));
	struct okfs_dentry_lhead *dhead = 
		(struct okfs_dentry_lhead *) Malloc(sizeof(struct okfs_dentry_lhead));
	dhead->entry = (struct okfs_dentry*)Malloc(sizeof(struct okfs_dentry));
	dhead->entry->d_name = Strdup(path);
	dhead->entry->d_dirent = dp;
	dhead->next = fsp->dcache[hash];
	fsp->dcache[hash] = dhead;
	return dhead->entry->d_dirent;
}


/*
  @fsp - filesystem given file belongs to
  @path - path to a file
  If content of directory in which this new file is located is in cache,
  add information about this file to it.
*/
void
cache_add_entry(struct okfs_superblock *fsp, char *path)
{
	char *sp, *newpath;
	struct okfs_inode *ip;
	struct okfs_dentry *entry;
	struct stat buf;
	
	/*example: /usr/sbin -> /usr*/
	sp = strrchr(path, '/');
	ASSERT(sp);
	*sp = 0;
	
	if (strlen(path) == 0)
		newpath = "/";
	else
		newpath = path;
	
	TRACE("trying to find %s", newpath);
	
	entry = cache_find_dentry(fsp, newpath);
	if (entry){
		struct okfs_dirent *tmp = entry->d_dirent;
		TRACE("found");
		entry->d_dirent = 
			(struct okfs_dirent *)Malloc(sizeof(struct okfs_dirent));
		entry->d_dirent->next_dir = tmp;
		
		*sp = '/';
		TRACE("looking for file %s", path);
		if (okfs_arglstat(path, &buf) != 0){
			/*file doesn't exists*/
			free(entry->d_dirent);
			entry->d_dirent = tmp;
			return;
		}
		*sp = 0;
		
		entry->d_dirent->ino = buf.st_ino;
		entry->d_dirent->type = DT_UNKNOWN;/*TODO ->read it from buf*/
		entry->d_dirent->name = Strdup(sp + 1);
		entry->d_dirent->name_len = strlen(entry->d_dirent->name);
		if ((ip = cache_find_inode(fsp, newpath, strlen(newpath)))){
			ip->statbuf.st_size += entry->d_dirent->name_len;
			ip->statbuf.st_atime = ip->statbuf.st_mtime = 
				time(NULL);
		}
	}
	
	*sp = '/';
}

/*
    @fsp - filesystem given file belongs to
    @path - path to a file
    If content of directory in which this file is located is in cache,
    remove information about this file from it.
 */
void
cache_remove_entry(struct okfs_superblock *fsp, char *path)
{
	char *sp, *newpath;
	struct okfs_dentry *entry;
	
	/*example: /usr/sbin -> /usr*/
	path_remove(fsp, path);

	sp = strrchr(path, '/');
	ASSERT(sp);
	*sp = 0;
	
	if (strlen(path) == 0)
		newpath = "/";
	else
		newpath = path;
	
	TRACE("trying to find %s", newpath);
	
	entry = cache_find_dentry(fsp, newpath);
	if (entry){
		struct okfs_dirent *p1 = entry->d_dirent, *p2;
		if (strcmp(entry->d_dirent->name, sp + 1) == 0){
			entry->d_dirent = entry->d_dirent->next_dir;
			free(p1->name);
			free(p1);
		}
		else{
			p2 = entry->d_dirent->next_dir;
			while(strcmp(p2->name, sp + 1) != 0){
				ASSERT(p2->next_dir);
				p1 = p2;
				p2 = p2->next_dir;
			}
			p1->next_dir = p2->next_dir;
			free(p2->name);
			free(p2);
		}
	}
	
	*sp = '/';
}



/*
  If given file, that belongs to a filesystem pointed by @fsp, is in
  cache, change its size to @size. 
  Routines that uses file descriptor as an argument do not have to do it,
  because they have direct access to stat structure of a file. 
  Truncate() uses this.
*/
void
change_file_size(struct okfs_superblock *fsp, const char *path, off_t size)
{
	struct okfs_inode *inop;
	if ((inop = cache_find_inode(fsp, path, strlen(path)))){
		inop->statbuf.st_size = size;
		inop->statbuf.st_atime = time(NULL);
		inop->statbuf.st_mtime = time(NULL);
	}
}
