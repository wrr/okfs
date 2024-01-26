/*$Id: vfs_path_calls.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/

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
#include <linux/errno.h>
#include <linux/utime.h>
#include "task.h"
#include "debug.h"
#include "entry.h"
#include "wrap.h"
#include "libc.h"
#include "okfs_unistd.h"
#include "vfs_namei.h"
#include "virtualfs.h"
#include "memacc.h"

/*Routines responsible for handling system calls with path to a file as 
  an argument(s). They resolve to which filesystem this file belongs
  and delegate calls to it.
*/


/*Process path pointed by first argument of slave's system call.
 *Convert this path to canonical (starting with '/' without '..', '.' and 
 *links). 
 *if OPTION equals READ_LAST_LINK and file specified by this path is a link,
 *path will be replaced by path to file pointed by this link. Links
 *inside path are always resolved.
 *Determine which filesystem is responsible for file specified by this path.
 */
int proc_path1(int option)
{
	struct okfs_superblock *fsp;
	struct fs_resolve *resp = &slave->callfs;
	char *rpath;
	int len;
	
	resp->arg1_fsp = NULL;
	
	/*memory leak detection*/
	ASSERT(!resp->rel_path1 && !resp->path1 && !resp->canon_path1);

	if (!ARG1){
		errno = EFAULT;
		return -1;
	}
	
	resp->path1 = slave_getstr((void *) ARG1);
	resp->path1_len = strlen(resp->path1);
	TRACE("path is = cwd = %s ", resp->path1, slave->cwd);
	
	if (option & READ_LAST_LINK)
		resp->canon_path1 =  canonicalize(slave->cwd, resp->path1);
	else
		resp->canon_path1 = lcanonicalize(slave->cwd, resp->path1);
	if (!resp->canon_path1)
		return -1;
	
	resp->canon_path1_len = strlen(resp->canon_path1);
	resp->rel_path1 = NULL;
	
	for(fsp = mounted_fs; fsp; fsp = fsp->next_fs)		
		if (strncmp(fsp->mnt_point, resp->canon_path1, 
			    fsp->mnt_point_len) == 0){
			
			resp->arg1_fsp = fsp;
			len =  resp->canon_path1_len - 
				resp->arg1_fsp->mnt_point_len;
			
			rpath = Malloc(len + 1);
			strcpy(rpath, resp->canon_path1 + 
			       resp->arg1_fsp->mnt_point_len);
			
			TRACE("relative path = %s", rpath);
			resp->rel_path1_len = len;
			resp->rel_path1 = rpath;
			break;
		}
	return 0;
}

/*The same as proc_path1 but process path pointed by second argument
  of slave's system call*/
int proc_path2(int option)
{
	struct okfs_superblock *fsp;
	struct fs_resolve *resp = &slave->callfs;
	char *rpath;
	int len;
	
	resp->arg2_fsp = NULL;
	/*memory leak detection*/
	ASSERT(!resp->rel_path2 && !resp->path2 && !resp->canon_path2);
	
	if (!ARG2){
		errno = EFAULT;
		return(-1);
	}
	
	resp->path2 = slave_getstr((void *) ARG2);
	resp->path2_len = strlen(resp->path2);
	
	if (option & READ_LAST_LINK)
		resp->canon_path2 = canonicalize(slave->cwd, resp->path2);
	else 
		resp->canon_path2 = lcanonicalize(slave->cwd, resp->path2);	
	if (!resp->canon_path2)
		return(-1);
	
	resp->canon_path2_len = strlen(resp->canon_path2);
	resp->rel_path2 = NULL;

	for(fsp = mounted_fs; fsp; fsp = fsp->next_fs)
		if (strncmp(fsp->mnt_point, resp->canon_path2, 
			    fsp->mnt_point_len) == 0){
			resp->arg2_fsp = fsp;
			len = 	resp->canon_path2_len -
				resp->arg2_fsp->mnt_point_len;

			rpath = Malloc(len + 1);
			strcpy(rpath, resp->canon_path2 + 
			       resp->arg2_fsp->mnt_point_len);
			
			resp->rel_path2_len = len;
			resp->rel_path2 = rpath;
			break;
		}
	return 0;
}

/*
 *Result of every successful proc_path1() call should be cleaned
 *by this call when no longer needed.
 */
void clean_path1(void)
{

	if (slave->callfs.path1)
		free(slave->callfs.path1);
	if (slave->callfs.canon_path1)
		free(slave->callfs.canon_path1);
	if (slave->callfs.rel_path1)
		free(slave->callfs.rel_path1);
	slave->callfs.path1 = NULL;
	slave->callfs.canon_path1 = NULL;
	slave->callfs.rel_path1 = NULL;
	slave->callfs.arg1_fsp = NULL;
}

/*
 *Result of every successful proc_path2() call should be cleaned
 *by this call when no longer needed.
 */
void clean_path2(void)
{
	if (slave->callfs.path2)
		free(slave->callfs.path2);
	if (slave->callfs.canon_path2)
		free(slave->callfs.canon_path2);
	if (slave->callfs.rel_path2)
		free(slave->callfs.rel_path2);
	slave->callfs.path2 = NULL;
	slave->callfs.canon_path2 = NULL;
	slave->callfs.rel_path2 = NULL;
	slave->callfs.arg2_fsp = NULL;
}


/*Does the same think as Linux readlink(), but operates also on files
  visible only for OKFS.
  This function is used by canonicalize() when removing links from path
  to a file. 
 */
int okfs_argreadlink(const char *path, char *buf, size_t bufsize)
{
	struct okfs_superblock *fsp;
	int ret;
	ASSERT(path);
	for(fsp = mounted_fs; fsp; fsp = fsp->next_fs)
		if (strncmp(fsp->mnt_point, path, fsp->mnt_point_len)
		    == 0)
			break;
	if (!fsp)
		return readlink(path, buf, bufsize);
	else if (fsp->entry->readlink){
		ret =  fsp->entry->readlink(fsp, path + fsp->mnt_point_len, 
					    buf, bufsize);
		if (ret < 0){
			errno = -ret;
			ret = -1;
		}
		return ret;
	}
	else{
		errno = EIO;
		return -1;
	}
}

/*unlike okfs_argreadlink() this function operates on arguments in slave's
  memory and puts results to it*/
void okfs_readlink(void)
{
	struct okfs_superblock *fsp;
	char *buf;
	int ret;
	size_t bufsize = ARG3;
	
	if (proc_path1(!READ_LAST_LINK) < 0){
		TRACE("error");
		ret = -errno;
		goto end;
	}
	if (bufsize <= 0){
		ret = -EINVAL;
		goto end;
	}
	
	buf = (char *)Malloc(bufsize);
	if ((fsp = slave->callfs.arg1_fsp)){
		if (!fsp->entry->readlink)
			ret = -EOPNOTSUPP;
		else
			ret = fsp->entry->readlink(fsp,
				  slave->callfs.rel_path1, buf, bufsize);
	}
	else{
		ret = readlink(slave->callfs.canon_path1, buf, bufsize);
		if (ret < 0)
			ret = -errno;
	}
	
	if (ret > 0)
		slave_memset((void *)ARG2, buf, ret);
	
	free(buf);
 end:
	set_slave_return(ret);
	clean_path1();
}

/*Used before link() and rename() calls.
  Both calls don't resolve last symlink in paths*/
void okfs_before_lnk_rnm(void)
{
	if (proc_path1(!READ_LAST_LINK) < 0){
		force_call_return(-errno);
		clean_path1();
		return;
	}
	if  (proc_path2(!READ_LAST_LINK) < 0){
		force_call_return(-errno);
		clean_path1();
		clean_path2();
		return;
	}
	
	
	if (slave->callfs.arg1_fsp == slave->callfs.arg2_fsp)
		dummy_call();
	
	else if (slave->callfs.arg1_fsp != slave->callfs.arg2_fsp){
		/*files specified by path1 and path2 are not on the 
		 *same filesystem.
		 *return EXDEV error to calling process*/
		clean_path1();
		clean_path2();		
		force_call_return(-EXDEV);
	}
}

void okfs_link(void)
{
	struct okfs_superblock *fsp = slave->callfs.arg1_fsp;
	int ret;

	ASSERT(fsp == slave->callfs.arg2_fsp);
	ret = -EPERM;
	if (!fsp){
		ret = link(slave->callfs.canon_path1,
			   slave->callfs.canon_path2);
		if (ret < 0)
			ret = -errno;

	}       	
	else if (fsp->entry->link){
		ret = fsp->entry->link(fsp, 
				       slave->callfs.rel_path1,
				       slave->callfs.rel_path2);
		if (ret == 0)
			cache_add_entry(fsp, slave->callfs.canon_path2);
	}

	
	set_slave_return(ret);
	
	clean_path1();
	clean_path2();
}

void okfs_rename(void)
{
	int ret = -EOPNOTSUPP;
	struct okfs_superblock *fsp = slave->callfs.arg1_fsp;

	ASSERT(fsp == slave->callfs.arg2_fsp);
	if (!fsp){
		ret = rename(slave->callfs.canon_path1,
			     slave->callfs.canon_path2);
		if (ret < 0)
			ret = -errno;
	}
	else if (fsp->entry->rename){
		ret = fsp->entry->rename(fsp, slave->callfs.rel_path1,	       
					 slave->callfs.rel_path2);
		if (ret == 0){
			cache_remove_entry(fsp, slave->callfs.canon_path1);
			cache_add_entry(fsp, slave->callfs.canon_path2);
		}
		
	}
	set_slave_return(ret);
	
	clean_path1();
	clean_path2();
}

void okfs_symlink(void)
{
	int ret;
	char *oldpath;
	struct okfs_superblock *fsp;
	
	if (proc_path2(!READ_LAST_LINK) < 0){
		ret = -errno;
		goto end;
	}
	
	/*first argument of symlink doesn't need to be canonical path*/
	oldpath = slave_getstr((void *) ARG1);
	
	if ((fsp = slave->callfs.arg2_fsp)){
		if (!fsp->entry->symlink)
			ret = -EOPNOTSUPP;
		else
			ret = fsp->entry->symlink(fsp, oldpath,
						  slave->callfs.rel_path2);
		if (ret == 0)
			cache_add_entry(slave->callfs.arg2_fsp,
				       slave->callfs.canon_path2);
	}
	else{
		ret = symlink(oldpath, slave->callfs.canon_path2);
		TRACE("symlinking %s to %s", oldpath, 
		      slave->callfs.canon_path2);
		if (ret < 0)
			ret =-errno;
	}
	
	free(oldpath);
 end:
	set_slave_return(ret);
	clean_path2();
}

void okfs_unlink(void)
{
	int ret;
	struct fs_entry_point *ep;
	if (proc_path1(!READ_LAST_LINK) < 0){
		ret = -errno;
		goto clean;
	}
	if (slave->callfs.arg1_fsp){
		ep = slave->callfs.arg1_fsp->entry; 
		if (ep->unlink)
			ret = ep->unlink(slave->callfs.arg1_fsp, 
					 slave->callfs.rel_path1);
		else
			ret = -EOPNOTSUPP;

		if (ret == 0)
			cache_remove_entry(slave->callfs.arg1_fsp,
					  slave->callfs.canon_path1);
	}
	else
		if ((ret = unlink(slave->callfs.canon_path1)) < 0)
			ret = -errno;
	
 clean:
	clean_path1();
	set_slave_return(ret);
}

void okfs_access(void)
{
	int ret;
	struct okfs_superblock *fsp;
	
	if (proc_path1(READ_LAST_LINK) < 0){
		TRACE("forcing error");
		ret = -errno;
		goto end;
	}
	
	fsp = slave->callfs.arg1_fsp;
	if (fsp){
		if (!fsp->entry->access)
			ret = -EOPNOTSUPP;
		else
			ret = fsp->entry->access(fsp, 
					       slave->callfs.rel_path1, ARG2);
	}
	else{
		ret = access(slave->callfs.canon_path1, ARG2);
		if (ret < 0)
			ret = -errno;
	}

 end:	
	set_slave_return(ret);
	clean_path1();
}


void okfs_chdir(void)
{
	struct stat buf;
	struct fs_resolve *resp = &slave->callfs;
	
	TRACE(" ");
	proc_path1(READ_LAST_LINK);
	if (!resp->canon_path1 || okfs_arglstat(resp->canon_path1, &buf) < 0)
		set_slave_return(-errno);

	else if (!S_ISDIR(buf.st_mode))
		 set_slave_return(-ENOTDIR);
	
	else{
		free(slave->cwd);
		slave->cwd = Malloc(resp->canon_path1_len+1);
		strcpy(slave->cwd, resp->canon_path1);
		TRACE("new cwd is %s", slave->cwd);
		set_slave_return(0);
	}
	clean_path1();	
}

void okfs_getcwd(void)
{
	char *buf = (char *)ARG1;
	unsigned long size = ARG2;
	int cwdlen = strlen(slave->cwd);
	TRACE("cwd = %s", slave->cwd);
	if (size < cwdlen + 1){
		set_slave_return(-ERANGE);
		return;
	}
	slave_memset(buf, slave->cwd, cwdlen + 1);
	set_slave_return(cwdlen + 1);
}

void okfs_mkdir(void)
{
	int ret;
	mode_t mode = ARG2;
	struct okfs_superblock *fsp;
	
	if (proc_path1(READ_LAST_LINK) < 0){
		TRACE("forcing error");
		ret = -errno;
		goto end;
	}

	if ((fsp = slave->callfs.arg1_fsp)){
		if (!fsp->entry->mkdir)
			ret = -EOPNOTSUPP;
		else
			ret = fsp->entry->mkdir(fsp, 
						slave->callfs.rel_path1, mode);
		if (ret == 0){
			cache_add_entry(fsp, slave->callfs.canon_path1);
		}
	}
	else{
		ret = mkdir(slave->callfs.canon_path1, mode);
		if (ret < 0)
			ret = -errno;
	}
	
 end:
	set_slave_return(ret);
	clean_path1();
}

void okfs_rmdir(void)
{
	int ret;
	struct okfs_superblock *fsp;
	
	if (proc_path1(READ_LAST_LINK) < 0){
		ret = -errno;
		goto end;
	}

	if ((fsp = slave->callfs.arg1_fsp)){
		if (!fsp->entry->rmdir)
			ret = -EOPNOTSUPP;
		else
			ret = fsp->entry->rmdir(fsp, slave->callfs.rel_path1);
		
		if (ret == 0){
			cache_remove_entry(fsp, slave->callfs.canon_path1);
		}
	}
	else{
		ret = rmdir(slave->callfs.canon_path1);
		if (ret < 0)
			ret = -errno;
	}
	
 end:
	set_slave_return(ret);
	clean_path1();
}

void okfs_utime(void)
{
	int ret;
	struct okfs_superblock *fsp;
	struct utimbuf buf, *bufp;

	if (proc_path1(READ_LAST_LINK) < 0){
		ret = -errno;
		goto end;
	}

	if (!ARG2)
		bufp = NULL;
	else{
		bufp = &buf;
		slave_memget(bufp, (void*)ARG2, sizeof(buf));
	}

	if ((fsp = slave->callfs.arg1_fsp)){
		if (!fsp->entry->utime)
			ret = -EOPNOTSUPP;
		else
			ret = fsp->entry->utime(fsp, slave->callfs.rel_path1,
						bufp);
	}
	else{
		ret = utime(slave->callfs.canon_path1, bufp);
		if (ret < 0)
			ret = -errno;
	}
	
 end:
	set_slave_return(ret);
	clean_path1();
}


void okfs_chmod(void)
{
	int ret;
	mode_t mode = ARG2;
	struct okfs_superblock *fsp;
	
	if (proc_path1(READ_LAST_LINK) < 0){
		TRACE("forcing error");
		ret = -errno;
		goto end;
	}

	if ((fsp = slave->callfs.arg1_fsp)){
		if (!fsp->entry->chmod)
			ret = -EOPNOTSUPP;
		else
			ret = fsp->entry->chmod(fsp, slave->callfs.rel_path1, 
						mode);
	}
	else{
		ret = chmod(slave->callfs.canon_path1, mode);
		if (ret < 0)
			ret = -errno;
	}
	
 end:
	set_slave_return(ret);
	clean_path1();
}

void okfs_truncate(void)
{
	int ret;
	off_t length = ARG2;
	struct okfs_superblock *fsp;

	if (proc_path1(READ_LAST_LINK) < 0){
		TRACE("forcing error");
		ret = -errno;
		goto end;
	}

	if ((fsp = slave->callfs.arg1_fsp)){
		if (!fsp->entry->truncate)
			ret = -EOPNOTSUPP;
		else
			ret = fsp->entry->truncate(fsp, 
				       slave->callfs.rel_path1, length);
		if (ret == 0)	       
			change_file_size(fsp, slave->callfs.canon_path1, length);
	}

	else{
		ret = truncate(slave->callfs.canon_path1, length);
		if (ret < 0)
			ret = -errno;
	}
	
 end:
	set_slave_return(ret);
	clean_path1();
}

void okfs_dochown(int opt)
{
	int ret;
	uid_t owner = ARG2;
	gid_t group = ARG3;
	struct okfs_superblock *fsp;
	
	if (proc_path1(opt) < 0){
		ret = -errno;
		goto end;
	}

	if ((fsp = slave->callfs.arg1_fsp)){
		if (!fsp->entry->lchown)
			ret = -EOPNOTSUPP;
		else
			ret = fsp->entry->lchown(fsp, slave->callfs.rel_path1,
						 owner, group);
	}
	else{
		ret = lchown(slave->callfs.canon_path1, owner, group);
		if (ret < 0)
			ret = -errno;
	}
	
 end:
	set_slave_return(ret);
	clean_path1();
}

void okfs_lchown(void)
{
	okfs_dochown(!READ_LAST_LINK);
}

void okfs_chown(void)
{
	okfs_dochown(READ_LAST_LINK);
}

void okfs_mknod(void)
{
	int ret;
	mode_t mode = ARG2;
	dev_t dev = ARG3;
	
	if (proc_path1(READ_LAST_LINK) < 0){
		ret = -errno;
		goto end;
	}

	if (slave->callfs.arg1_fsp)
		ret = -EOPNOTSUPP;
	else{
		ret = mknod(slave->callfs.canon_path1, mode, dev);
		if (ret < 0)
			ret = -errno;
	}
	
 end:
	set_slave_return(ret);
	clean_path1();
}

void okfs_acct(void)
{
	int ret;
	
	if (proc_path1(READ_LAST_LINK) < 0){
		ret = -errno;
		goto end;
	}

	if (slave->callfs.arg1_fsp)
		ret = -EOPNOTSUPP;
	else{
		ret = acct(slave->callfs.canon_path1);
		if (ret < 0)
			ret = -errno;
	}
	
 end:
	set_slave_return(ret);
	clean_path1();
}

void okfs_statfs(void)
{
	int ret;
	struct statfs buf;
	struct okfs_superblock *fsp;
	
	if (proc_path1(READ_LAST_LINK) < 0){
		TRACE("forcing error");
		ret = -errno;
		goto end;
	}

	if ((fsp = slave->callfs.arg1_fsp)){
		if (!fsp->entry->statfs)
			ret = -EOPNOTSUPP;
		else
			ret = fsp->entry->statfs(fsp, 
						 slave->callfs.rel_path1,&buf);
	}
	else{
		ret = statfs(slave->callfs.canon_path1, &buf);
		if (ret < 0)
			ret = -errno;
	}
	

	if (ret == 0)
		slave_memset((void *)ARG2, &buf, sizeof(buf));
 end:
	set_slave_return(ret);
	clean_path1();
}
