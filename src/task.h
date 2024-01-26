/*$Id: task.h,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
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

#ifndef TASK_H
#define TASK_H

#include <linux/types.h>
#include <linux/signal.h>
#include <linux/user.h>
#include <linux/limits.h>
#include <linux/unistd.h>

/*state of a process: SLAVE_STOPPED, SLAVE_ZOMBIE etc.*/
#include <linux/sched.h> 

#include "filesystems/virtualfs.h"


/*if process was stopped by SIGTRAP because it enters or leaves system call 
 *wait returns such status:*/ 
#define WIFSYSCALL(status) ((status) == ((SIGTRAP | 0x80) << 8 | 0x7f))


#define SLAVE_RUNNING 0
#define SLAVE_STOPPED 1
#define SLAVE_ZOMBIE  2

/*task is stopped and parent has received information about it*/
#define SLAVE_STOPPED_AND_NOTIFIED 4

#define STOPPED_TASK(x) ((x) == SLAVE_STOPPED || (x) == SLAVE_STOPPED_AND_NOTIFIED)

/*This signal is used to wake up traced process from pause() call.
  Let's hope that traced process doesn't use this*/ 
#define SIGUNPAUSE SIGUNUSED

/*
 *ptrace() doesn't allow to cancel system call called by traced process, but
 *it can be changed to harmless one instead.
 */
#define DUMMY_CALL_NR __NR_getpid

/*list of parent's children*/
struct child{
	struct task *child_task;
	struct child *next_child;
};

/*all informations about traced tasks*/
struct task{
	struct task *next_slave; /*next task on the list*/
	struct child *childp; /*children of this task*/
	
	/*pointer to stopped child of this task, used by wait() call*/
	struct task *stopped_child;
	
	pid_t pid, ppid;
	gid_t gid;
	uid_t uid;
	long state; /*running, stopped, zombie, dead*/
	/*value returned to parent of this child as second argument in
	  wait() call*/
	int exit_code; 
	
	/*value saved to be returned as a result of current system call
	  (use force_call_return() to do it)*/
	int retval_saved;

	/*used to restart slave's wait() call when status of one of
	  its child changes*/
	int restart_call;
	
	/*Signals queued to this task when it was in stopped state.*/
	sigset_t sigpending_mask;
	
	/*if true don't notify this process when its child stops. 
	  Changed by sigaction call.*/
	int nocldstop;
	
	/*Needed only to find out which stopping signals are blocked
	 by this process*/
	__sighandler_t sighandler[_NSIG];
	
	/*Content of this task registers*/
	struct user_regs_struct regs;
	/*Used to store content of registers in order to reset it later*/
	struct user_regs_struct save_regs;	

	/*arguments of system call called by this task*/
	long arg[6];
	
	/*Argument of few system calls are placed on stack rather then in
	  registers (mmap)*/
	int has_stack_args;
	
	/*pointer to function that will be called after current system call
	  returns*/
	void (*next_call)(void);

	/*if true this task has called system call, and next status
	 returned by ptrace() for this task will be a result of this call*/
	int during_call;
	
	/*current working directory of this task*/
	char *cwd;

	/*filesystem responsible for handling current system call of this task
	  with path as argument(s)*/
	struct fs_resolve callfs;
	
	/*files related to file descriptors of this task*/
	struct okfs_file *fd_tab[OPEN_MAX];
	
	/*value set by fcntl(F_SETFD ...)*/
	int close_on_exec[OPEN_MAX];

	/*maps file descriptor used by Linux and okfs virtual file system
	to file descriptor used by actual okfs filesystem (for example 
	localfs)*/
	int fsfd[OPEN_MAX];

	/*Here informations about changes in slave's memory are stored in
	order to reverse these changes later*/
	void *rpl_code;
	size_t rpl_code_len;
	void *rpl_dest;
};

extern struct task *slave;

/*not too elegant shortcuts to access and set calls arguments 
  and return values*/
#define RETURN_VALUE slave->regs.eax
#define ARG1 slave->arg[0]
#define ARG2 slave->arg[1]
#define ARG3 slave->arg[2]
#define ARG4 slave->arg[3]
#define ARG5 slave->arg[4]
#define CALL_NR slave->regs.orig_eax

#define set_slave_return(value)do{ \
        RETURN_VALUE = value;\
        set_slave_args();\
}while(0)

void get_slave_args(int callnr);
void set_slave_args(void);

void slave_init(pid_t, pid_t, int);
void change_slave_state(int state, int wait_status);
void add_child(struct task *child_task);
struct task *add_slave(pid_t pid, struct task *parent);
void remove_slave(pid_t pid);
void remove_child(pid_t pid);
struct task *find_slave(pid_t pid);
void slay_slaves(void);


void dummy_call(void);
void unimplemented_call(void);
void not_supported(void);
void force_call_return(int retval);

#endif
