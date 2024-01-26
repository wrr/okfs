/*$Id: vfs_open.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/

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
#include <linux/fcntl.h>
#include "task.h"
#include "debug.h"
#include "entry.h"
#include "libc.h"
#include "wrap.h"
#include "virtualfs.h"
#include "memacc.h"
#include "vfs_open.h"
#include "okfs_unistd.h"


/*
  Save information about new slave's file descriptor.
  @fsp - filesystem responsible for this fd (NULL -> Linux)
  @osfd - fd value visible by Linux and passed to slave process
  @fsfd - fd returned by filesystem when it opened this file. It is passed
  to this filesystem when calling functions related to this fd.
  Curently localfs uses fsfd value but shfs doesn't need it. 
  @canon_path - canonical path to file related to this fd
  @rel_path - path to this file from filesystem mount point.
 */
static void add_new_fd(struct okfs_superblock *fsp, int osfd, int fsfd, char *canon_path, char *rel_path)
{
	ASSERT(osfd >= 0 && osfd < OPEN_MAX);
	ASSERT(slave->fd_tab[osfd] == NULL);
	ASSERT(canon_path);
	
	slave->fd_tab[osfd] = Malloc(sizeof(struct okfs_file));
	slave->fd_tab[osfd]->nshared = 1;
	slave->fd_tab[osfd]->f_pos = 0;
	slave->fsfd[osfd] = fsfd;	
	
	slave->fd_tab[osfd]->path = Strdup(canon_path);
	
	if (rel_path)
		slave->fd_tab[osfd]->rel_path = Strdup(rel_path);
	else
		slave->fd_tab[osfd]->rel_path = NULL;
			
	slave->fd_tab[osfd]->fsp = fsp;

	if (fsp)
		slave->fd_tab[osfd]->ino = cache_find_inode(fsp,  canon_path, 
							strlen(canon_path));
}

static char *tmpf;
static void remove_tmpfile(void)
{
	unlink(tmpf);
}

/*
  Creates a temporary file to which all open() calls, related to files
  for which okfs is responsible instead of Linux, are redirected.
  So every descriptor of file managed by one of OKFS filesystems, is for Linux 
  descriptor pointing to this temp. file.
*/
void create_tmpfile(const char *dirname)
{
	int fd;
	int tmpf_len;
	
	tmpf_len = strlen(dirname) + strlen("/.okfs") + 6;
	tmpf = Malloc(tmpf_len + 1);
	strcpy(tmpf, dirname);
	strcat(tmpf, "/.okfs");
	strcat(tmpf, "XXXXXX");
	
	if ((fd = mkstemp(tmpf)) < 0)
		err_sys("can't  create temporary file %s. Try to pass correct temporary dir to which you have write access with -t option", tmpf);
	
	if (chmod(tmpf,  S_IRUSR | S_IWUSR) < 0)
		err_sys("chmod to temporary file %s error", tmpf);
	
	if (write(fd, "Nocna lampka wstrzymala oddech, dzwon ciszy bil na alarm, reka uniosla sie, blysnela stal", 89) < 0)
		err_sys("can't write to temporary file %s", tmpf);
	
	atexit(remove_tmpfile);
	TRACE("file %s created", tmpf);
}

static int okfs_do_open(int flags, mode_t mode)
{
	int ret, fexist;
	struct stat buf;
	struct fs_resolve *resp =  &slave->callfs;
	struct okfs_superblock *fsp = resp->arg1_fsp;
	struct okfs_inode *ip = NULL;
	struct fs_entry_point *ep;
	ASSERT(fsp);
	ep = fsp->entry;
	
	/*does this fs support all calls needed to open a file?*/
	if (!ep->open || !ep->truncate || !ep->chmod || !ep->lstat){
		ret = -EOPNOTSUPP;
		goto error;
	}
	
	/*is stat info about this file in cache?*/
	ip = cache_find_inode(fsp, resp->canon_path1, resp->canon_path1_len);
	if (ip)
		fexist = 1;
	else{
		fexist = (ep->lstat(fsp, resp->rel_path1,  &buf) == 0);
		if (fexist)
			ip = cache_add_inode(fsp, resp->canon_path1,
					     resp->canon_path1_len, &buf, NULL);
	}
		
	
	if (flags & O_CREAT){
		if ((flags & O_EXCL) && fexist){
			ret = -EEXIST;
			goto error;
		}	
		
		/*not virtual file systems will receive O_CREAT
		  only when file really doesn't exist*/
		if (fexist)
			flags &= ~O_CREAT;
	}
	
	if (flags & O_TRUNC){
		if (fexist && ((flags & O_RDWR) == O_RDWR ||
			       (flags & O_WRONLY) == O_WRONLY))
			ep->truncate(fsp, resp->rel_path1, 0);
		flags &= ~O_TRUNC;
	}

	ret = fsp->entry->open(fsp, resp->rel_path1, flags, mode);

	if (ret >= 0){
		if (!ip){
			/*now file has to exist*/
			int debug = ep->lstat(fsp, resp->rel_path1,  &buf);
			ASSERT(debug == 0);
			ip = cache_add_inode(fsp, resp->canon_path1, 
					  resp->canon_path1_len, &buf, NULL);
		} 
		ASSERT(ip);		

		if (!fexist && (flags & O_CREAT))
			cache_add_entry(fsp, slave->callfs.canon_path1);
	}

 error:
	return ret;
}


static void okfs_after_okfs_open(void)
{
	int flags = ARG2;
	int ret = RETURN_VALUE;
	struct fs_resolve *fop = &slave->callfs;

	if (ret >= OPEN_MAX)
		ret = -ENFILE;
	else if (ret >= 0){
		add_new_fd(fop->arg1_fsp, ret, slave->retval_saved, 
			   fop->canon_path1, fop->rel_path1);
		ASSERT(slave->fd_tab[ret]);
		if (flags & O_APPEND)
			slave->fd_tab[ret]->f_pos = 
				slave->fd_tab[ret]->ino->statbuf.st_size;
	}
	
	CALL_NR = slave->save_regs.orig_eax;
	ARG2 = slave->save_regs.ecx;
	ARG3 = slave->save_regs.edx;
	RETURN_VALUE = ret;
	set_slave_args();
	slave_restore_mem();
	clean_path1();
}

/*OKFS has to keep track of what files Linux is opening, only to be able
 *to determine what is current working directory after fchdir() call,
 *with Linux fd as an argument
 */
static void okfs_after_linux_open(void)
{
	int ret = RETURN_VALUE;
	struct fs_resolve *fop = &slave->callfs;

	ASSERT(fop->arg1_fsp == NULL);

	if (ret >= OPEN_MAX)
		ret = -ENFILE;
	else if (ret >= 0)
		add_new_fd(NULL, ret, ret, fop->canon_path1,
			   fop->rel_path1);
	
	slave_restore_mem();
	clean_path1();
}


void okfs_before_open(void)
{
	int ret;
	if (proc_path1(READ_LAST_LINK) < 0){
		force_call_return(-errno);
		clean_path1();
		return;
	}
	
	TRACE("open called orig path was %s canonicalized is %s", 
	      slave->callfs.path1, slave->callfs.canon_path1);
	
	if (slave->callfs.arg1_fsp){
		/*file managed by okfs*/
		if ((ret = okfs_do_open(ARG2, ARG3)) < 0){
			force_call_return(ret);			
			clean_path1();
			return;
		}
		slave_rpl_known_str((void *) ARG1, tmpf, slave->callfs.path1);
		slave->save_regs = slave->regs;
		slave->retval_saved = ret;
		ARG2 = O_RDONLY | O_CREAT;
		ARG3 = S_IRUSR | S_IWUSR;
		set_slave_args();
		slave->next_call = okfs_after_okfs_open;		
	}
	else{
		/*file managed by Linux*/

		/*Path has to be changed because cwd can be different
		  for Linux that it really is.
		 */
		slave_rpl_known_str((void *) ARG1, slave->callfs.canon_path1, 
				    slave->callfs.path1);
		slave->next_call = okfs_after_linux_open;
	}
}

/*the same as okfs_before_open but with different args*/
void okfs_before_creat(void)
{
	int ret;
	if (proc_path1(READ_LAST_LINK) < 0){
		force_call_return(-errno);
		clean_path1();
		return;
	}
	
	TRACE("open called orig path was %s canonicalized is %s", 
	      slave->callfs.path1, slave->callfs.canon_path1);
	
	if (slave->callfs.arg1_fsp){
		if ((ret = okfs_do_open(O_CREAT | O_WRONLY | O_TRUNC, ARG2)) 
		    < 0){
			force_call_return(ret);			
			clean_path1();
			return;
		}
		slave_rpl_known_str((void *) ARG1, tmpf, slave->callfs.path1);
		slave->save_regs = slave->regs;
		slave->retval_saved = ret;
		ARG2 = O_RDONLY | O_CREAT;
		ARG3 = S_IRUSR | S_IWUSR;
		CALL_NR = NR_OPEN;
		set_slave_args();
		slave->next_call = okfs_after_okfs_open;		
	}
	else{
		slave_rpl_known_str((void *) ARG1, slave->callfs.canon_path1, 
				    slave->callfs.path1);
		slave->next_call = okfs_after_linux_open;
	}
}
