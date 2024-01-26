/*$Id: vfs_fd_calls.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/

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

#include <linux/mman.h>
#include <linux/errno.h>
#include "task.h"
#include "debug.h"
#include "entry.h"
#include "wrap.h"
#include "libc.h"
#include "virtualfs.h"
#include "memacc.h"

/*Routines responsible for handling system calls with 
  file descriptor(s) as an argument(s).
*/



/*Called before system calls that perform operations on files 
  related to file descriptor passed to them.*/
void okfs_fd_call(void)
{
	int fd = ARG1;
	
	if (fd >= 0 && fd < OPEN_MAX && slave->fd_tab[fd] &&
	    slave->fd_tab[fd]->fsp){
		TRACE("okfs fd");
		/*Do not pass this call to Linux*/
		dummy_call();
	}
	else{
		/*Linux fd, let him do the job*/
		TRACE("Linux fs %d", fd);
		slave->next_call = NULL;
	}
	
}

/*This function is called before operations on file descriptors but not
  on file related to them (like dup(), close()). 
  Unlike okfs_fd_call() this function always passes control to Linux.
  Linux can't do operations like read() or write() from OKFS fds,
  because for Linux all these fds points to some temporary file, but 
  they are visible for it so, it can dup, and close them.
*/
void okfs_op_on_fd()
{
	int fd = ARG1;
	
	if (fd < 0 || fd >= OPEN_MAX || !slave->fd_tab[fd]){
		TRACE("not file-related descriptor %d", fd);
		slave->next_call = NULL;
	}
	else{
		TRACE("file-related descriptor %d", fd);
	}
}

/*Some  complex (mostly rare used) system calls are not supported
  by OKFS but can be passed to Linux if it is its file descriptor*/
void okfs_only_os(void)
{
	int fd = ARG1;
	TRACE("fd = %ld", ARG1);
	if (fd >= 0 && fd < OPEN_MAX && slave->fd_tab[fd] &&
	    slave->fd_tab[fd]->fsp)
		force_call_return(-EOPNOTSUPP);
}

/*used when slave's file descriptor is being closed (by close(), on exit,
  sometimes by execve)*/
void okfs_close_fd(struct task *tp, int fd)
{
	ASSERT(tp->fd_tab[fd]);
	tp->close_on_exec[fd] = 0;
	tp->fd_tab[fd]->nshared--;	
	ASSERT(tp->fd_tab[fd]->nshared >= 0);
	if (!tp->fd_tab[fd]->nshared){
		TRACE("no more shares removing this fd");
		if (tp->fd_tab[fd]->path)
			free(tp->fd_tab[fd]->path);
		if (tp->fd_tab[fd]->rel_path)
			free(tp->fd_tab[fd]->rel_path);
		free(tp->fd_tab[fd]);
	}
	tp->fd_tab[fd] = NULL;
}


void okfs_after_close(void)
{
	int ret = RETURN_VALUE, fd = ARG1;
	struct okfs_superblock *fsp; 
	
	ASSERT(slave->fd_tab[fd]);
	ASSERT(ret == 0);
	
	if ((fsp = slave->fd_tab[fd]->fsp) && fsp->entry->close
	    && slave->fd_tab[fd]->nshared == 1){
		TRACE("closing %d", fd);
		ret = fsp->entry->close(slave->fd_tab[fd], 
					slave->fsfd[fd]);
		TRACE("done");
		ASSERT(ret == 0);
	}
	okfs_close_fd(slave, fd);
	
}

void okfs_read(void)
{
	int fd = ARG1;
	int ret = 0;
	size_t count = ARG3;
	void * buf;
	struct okfs_superblock *fsp = slave->fd_tab[fd]->fsp;
	struct okfs_file *filep = slave->fd_tab[fd];

	if (!fsp->entry->read){
		set_slave_return(-EOPNOTSUPP);	
		return;
	}
	if (count > 0 && filep->f_pos < filep->ino->statbuf.st_size){
		TRACE("reading from file %s, size = %d count %d, f_pos = %d",
		      filep->path, (int)filep->ino->statbuf.st_size, (int)count,
		      (int)filep->f_pos);
		
		buf = Malloc(count);
		ret = fsp->entry->read(filep, slave->fsfd[fd],buf, count);
		TRACE("readed %d", ret);
		if (ret > 0){
			ASSERT(ret <= count);
			slave_memset((void *)ARG2, buf, ret);		
			filep->f_pos += ret;
		}
		free(buf);
	}
	
	set_slave_return(ret);	
}

void okfs_write(void)
{
	int fd = ARG1;
	int ret;
	size_t count = ARG3;
	void * buf;
	struct okfs_superblock *fsp = slave->fd_tab[fd]->fsp;
	
	if (!fsp->entry->write){
		set_slave_return(-EOPNOTSUPP);
		return;
	}
	if (count){
		buf = Malloc(count);
		slave_memget(buf, (void *)ARG2, count);
		ret = fsp->entry->write(slave->fd_tab[fd], slave->fsfd[fd],
					buf, count);
		if (ret > 0){
			slave->fd_tab[fd]->f_pos += ret;
			slave->fd_tab[fd]->ino->statbuf.st_size += ret;
		}
		free(buf);
	}
	
	set_slave_return(ret);	
}


void okfs_ftruncate(void)
{
	int fd = ARG1;
	off_t length = ARG2;
	struct okfs_superblock *fsp = slave->fd_tab[fd]->fsp;
	int ret;
	
	if (!fsp->entry->truncate)
		ret = -EOPNOTSUPP;
	else {
		ret = fsp->entry->truncate(fsp, slave->fd_tab[fd]->rel_path,
					   length);
		if (ret == 0)
			slave->fd_tab[fd]->ino->statbuf.st_size = ret;
	}
	
	set_slave_return(ret);
}

void okfs_fstatfs(void)
{
	int fd = ARG1;
	struct okfs_superblock *fsp = slave->fd_tab[fd]->fsp;
	struct statfs buf;
	int ret;
	
	if (!fsp->entry->statfs)
		ret = -EOPNOTSUPP;
	else 
		ret = fsp->entry->statfs(fsp, slave->fd_tab[fd]->rel_path,
					 &buf);
	if (ret == 0)
		slave_memset((void *)ARG2, &buf, sizeof(buf));
	
	set_slave_return(ret);
}

void okfs_fchmod(void)
{
	int fd = ARG1;
	mode_t mode = ARG2;
	struct okfs_superblock *fsp = slave->fd_tab[fd]->fsp;
	int ret;
	
	if (!fsp->entry->chmod)
		ret = -EOPNOTSUPP;
	else 
		ret = fsp->entry->chmod(fsp, slave->fd_tab[fd]->rel_path,
					 mode);
	
	set_slave_return(ret);
}

void okfs_fchown(void)
{
	int fd = ARG1;
	uid_t owner = ARG2;
	gid_t group = ARG3;
	struct okfs_superblock *fsp = slave->fd_tab[fd]->fsp;
	int ret;
	
	if (!fsp->entry->lchown)
		ret = -EOPNOTSUPP;
	else 
		ret = fsp->entry->lchown(fsp, slave->fd_tab[fd]->rel_path,
					 owner, group);
			       
	set_slave_return(ret);
}

static int okfs_do_dup(int oldfd, int newfd)
{
	if (newfd < 0)
		return newfd;
	
	ASSERT(slave->fd_tab[oldfd]);

	slave->fd_tab[newfd] = slave->fd_tab[oldfd];
	slave->close_on_exec[newfd] = 0;
	slave->fd_tab[newfd]->nshared++;
	slave->fsfd[newfd] = slave->fsfd[oldfd];
	return newfd;
}

void okfs_dup(void)
{	
	int newfd = RETURN_VALUE;
	if (newfd >= OPEN_MAX)
		set_slave_return(-EMFILE);
	else
		okfs_do_dup(ARG1, newfd);
}

void okfs_dup2(void)
{
	int oldfd = ARG1, newfd = RETURN_VALUE;
	if (newfd < 0)
		return;
	if (newfd >= OPEN_MAX){
		set_slave_return(-EMFILE);
		return;
	}
	
	TRACE("dupping2 %d to %d", oldfd, newfd);
	
	/*if fd2 in use close it*/
	if (slave->fd_tab[newfd])
		okfs_close_fd(slave, newfd);
	
	if (oldfd >= 0 && oldfd < OPEN_MAX && slave->fd_tab[oldfd])
		okfs_do_dup(oldfd, newfd);	
}

#ifndef SEEK_SET
# define SEEK_SET 0
# define SEEK_CUR 1
# define SEEK_END 2
#endif

void okfs_lseek(void)
{
	int fd = ARG1;
	loff_t offset = (loff_t)ARG2;
	int origin = ARG3;
	off_t retval;
	struct okfs_superblock *fsp = slave->fd_tab[fd]->fsp;
	
	retval = -EINVAL;	
	if (fsp->entry->lseek){
		TRACE("fd = %d fsfd = %d", fd, slave->fsfd[fd]);
		retval = fsp->entry->lseek(slave->fd_tab[fd], 
					   slave->fsfd[fd], offset, origin);
		if (retval < 0){
			TRACE("< 0");
			goto error;
		}
	}
	
	if (origin <= 2){
		if (origin == SEEK_SET)
			slave->fd_tab[fd]->f_pos = offset;
		else if (origin == SEEK_CUR)
			slave->fd_tab[fd]->f_pos += offset;
		else if (origin == SEEK_END){
			okfs_arglstat(slave->fd_tab[fd]->path, 
				     &slave->fd_tab[fd]->ino->statbuf);
			slave->fd_tab[fd]->f_pos = 
				slave->fd_tab[fd]->ino->statbuf.st_size + offset;
		}
		TRACE("%d", (int)retval);
		retval = slave->fd_tab[fd]->f_pos;
		TRACE("%d", (int)retval);
		
		if ((loff_t) retval != slave->fd_tab[fd]->f_pos){
			TRACE("overflow");
			retval = -EOVERFLOW;
		}
	}

 error:	
	TRACE("returning %d", (int)retval);
	set_slave_return(retval);
}


void okfs_before_mmap(void)
{
	unsigned long flags = ARG4;
	int fd = ARG5;
	
	if (flags & MAP_ANONYMOUS){
		TRACE("MAP_ANONYMOUS request");
		return;
	}
	if (fd < 0 || fd >= OPEN_MAX || !slave->fd_tab[fd]->fsp)
		return;

	force_call_return(-EOPNOTSUPP);
} 


static void okfs_after_fcntl(void)
{
	int fd = ARG1;
	unsigned cmd = ARG2;
	unsigned long arg = ARG3;
	int ret = RETURN_VALUE;
	
	if (ret < 0)
		return;
	switch(cmd){
	case F_DUPFD:
		TRACE("duping");
		if (ret >= OPEN_MAX)
			set_slave_return(-EINVAL);
		else
			okfs_do_dup(fd, ret);
		break;
	case F_GETFD:
		TRACE("fd = %d clo = %d ret = %d", fd, slave->close_on_exec[fd], ret);
		ASSERT(ret == slave->close_on_exec[fd]);
		break;
	case F_SETFD:
		TRACE("fd = %d %ld", fd, arg);
		slave->close_on_exec[fd] = ((arg & FD_CLOEXEC) > 0);
		break;
	default:
		TRACE("unsupported fcntl flag");
	}
}

void okfs_before_fcntl(void)
{
	int fd = ARG1;
	unsigned cmd = ARG2;
	
	if (fd < 0 || fd >= OPEN_MAX || !slave->fd_tab[fd]){
		TRACE("not file-related descriptor %d", fd);
		return;
	}
	TRACE("file-related descriptor %d %s", fd, 
	      slave->fd_tab[fd]->path);
	
	if (slave->fd_tab[fd]->fsp){
		if (cmd == F_SETLK || cmd == F_SETLKW || cmd == F_GETLK){
			force_call_return(-ENOLCK);			
		}
		else if (cmd == F_NOTIFY || cmd == F_GETLEASE || 
			 cmd == F_SETLEASE || cmd == F_GETOWN ||
			 cmd == F_SETOWN || cmd == F_GETSIG || 
			 cmd == F_SETSIG || cmd == F_GETFL || cmd == F_SETFL){
			force_call_return(-EOPNOTSUPP);
		}
		else{
			TRACE("cmd = %d", cmd);
			slave->next_call = okfs_after_fcntl;
		}
	}
	else
		slave->next_call = okfs_after_fcntl;
}



void okfs_fchdir(void)
{
	int fd = ARG1;
	struct stat buf;
	int ret;
	char *path;
	
	if (fd < 0 || fd >= OPEN_MAX || !slave->fd_tab[fd])
		ret = -EBADF;
	else{
		ASSERT(slave->fd_tab[fd]);
		ASSERT((path = slave->fd_tab[fd]->path));
		
		if (okfs_arglstat(path, &buf) < 0)
			ret = -errno;
		else if (!S_ISDIR(buf.st_mode))
			ret = -ENOTDIR;
	
		else{
			free(slave->cwd);
			slave->cwd = Malloc(strlen(path) + 1);
			strcpy(slave->cwd, path);
			TRACE("new cwd is %s", path);
			ret = 0;
		}
	}
	set_slave_return(ret);
}

