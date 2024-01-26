/*$Id: control.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
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

/*
 * This file handles fork, wait, execve calls.
 */

#include <linux/wait.h> //__WNOTHREAD etc.
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/errno.h>
#include <linux/sched.h> /*clone() related flags*/
#include <linux/ptrace.h>
#include "task.h"
#include "debug.h"
#include "control.h"
#include "entry.h"
#include "wrap.h"
#include "memacc.h"
#include "libc.h"

/*
 *There is a problem with wait() call because, when traced process's 
 *parent calls wait() to receive status of its not terminated
 *child, Linux returns errno value -ECHILD to this parent.
 *Correct behavior has to be simulated.
 */


/*Called when parent calls wait() with WNOHANG flag and child
  it waits for exists and is running*/
static void after_wait_wnohang(void)
{
	TRACE("wait WNOHANG returns %d", RETURN_VALUE);
	/*When calling process has child but it is still running,
	 *Linux returns -ECHILD as a result of wait() call. 
	 *It has to be changed to 0*/
	if (RETURN_VALUE == -ECHILD){
		set_slave_return(0);
		return;
	}

	/*Child of calling process could terminated in a period
	  of time between setting this routine and it's execution.	  
	  OKFS only has to remove this child from parent's children list. 
	*/
	if (RETURN_VALUE > 0)
		remove_child(RETURN_VALUE);
}

/*called when OKFS knows that parent has child it waits for in a
  zombie state*/
static void after_wait_iszombie(void)
{
	
	/*When parent has terminated child, Linux behaves correctly,
	  all informations about this child can be removed*/
	ASSERT(RETURN_VALUE > 0);
	remove_child(RETURN_VALUE);
	remove_slave(RETURN_VALUE);
}

/*called when OKFS knows that parent has child it waits for in a
  stopped state*/
static void after_wait_isstopped(void)
{
	unsigned int *stat_addr = (unsigned int *) ARG2;
	int status;

	/*Stopped child could terminated in a period
	  of time between setting this routine and it's execution.
	  Or linux could return status of other not stopped child.
	*/	  
	if (RETURN_VALUE != 0 && RETURN_VALUE != -ECHILD){		
		slave_memget(&status, (void *)ARG2, sizeof(int));
		if(RETURN_VALUE > 0 && !wifstopped(status))
			after_wait_iszombie();
		return;
	}

	TRACE("notifying about stopped child");	
	slave_memset(stat_addr, &slave->stopped_child->exit_code, sizeof(int));
	slave->stopped_child->state = SLAVE_STOPPED_AND_NOTIFIED;
	set_slave_return(slave->stopped_child->pid);
}


/*Called when parent calls wait() without WNOHANG flag, and child
  it waits for exists and is running. Outside ptrace environment this
  call would hang until some child would terminate. OKFS has to simulate
  such behavior by stopping parent until one of its child ends running.
*/
static void suspend(void)
{
	slave->restart_call = CALL_NR;/*__NR_wait or __NR_wait4*/
	
	/*Force pause to be called instead of wait, but when pause
	 *returns wait will be restarted.*/
	CALL_NR = __NR_pause;
	Ptrace(PTRACE_SETREGS, slave->pid, 0, &slave->regs);
}


/*More or less from linux/kernel/exit.c sys_wait4.
 *TODO: Code that is responsible for handling threads, has to be added.
 */
void before_wait(void)
{
	pid_t pid = (pid_t)ARG1;
	//unsigned int * stat_addr = (unsigned int*)ARG2;
	int options = (int)ARG3;
	//struct rusage *ru = (struct rusage*) ARG4;
	int flag;
	
	if (options & ~(WNOHANG|WUNTRACED|__WNOTHREAD|__WCLONE|__WALL))
		return;
	
	TRACE("wait searching for child %d", pid);

	flag = 0;
	do{
		struct child *cp;
		struct task *p;
	 	for (cp = slave->childp; cp ;cp = cp->next_child) {
			p = cp->child_task;
			TRACE("checking child %d", p->pid);
			if (pid>0) {
				if (p->pid != pid)
					continue;
			} else if (!pid) {
				if (p->gid != slave->gid)
					continue;
			} else if (pid != -1) {
				if (p->gid != -pid)
					continue;
			}
			/*TODO*/
			/* Wait for all children (clone and not) if __WALL is set;
			 * otherwise, wait for clone children *only* if __WCLONE is
			 * set; otherwise, wait for non-clone children *only*.  (Note:
			 * A "clone" child here is one that reports to its parent
			 * using a signal other than SIGCHLD.) */
			
			/*i have to think about it later!!!:*/
			/*if (((p->exit_signal != SIGCHLD) ^ ((options & __WCLONE) != 0))
			    && !(options & __WALL))
			    continue;*/
			flag = 1;
			switch (p->state) {
			case SLAVE_STOPPED:
				/*if (!p->exit_code)
				  continue;
				  if (!(options & WUNTRACED) && !(p->ptrace & PT_PTRACED))
				continue;*/
				TRACE("stopped child exists");
				if (!(options & WUNTRACED))
					continue;
				/*retval = ru ? getrusage(p, RUSAGE_BOTH, ru) : 0; 
				if (!retval && stat_addr) 
				retval = put_user((p->exit_code << 8) | 0x7f, stat_addr);
				if (!retval) {
					p->exit_code = 0;
					retval = p->pid;
					}*/
				slave->next_call = after_wait_isstopped;

				/*
				  Linux doesn't know that this child
				  is stopped and it could hang.
				*/
				ARG3 |= WNOHANG;
				
				set_slave_args();			
				slave->stopped_child = p;
				return;
			case SLAVE_ZOMBIE:
				TRACE("zombie child exists");
				slave->next_call = after_wait_iszombie;
				return;
			default:
				continue;
			}
		}
		/*if (options & __WNOTHREAD)
			break;
			tsk = next_thread(tsk);*/
	} while (0); //(tsk != slave);
	if (flag) {
		if (options & WNOHANG){
			TRACE("ready child not found, nohang set, returning");
			slave->next_call = after_wait_wnohang;
			return;
		}
		TRACE("child found but it's not ready, suspending");
		suspend();
		/*pause() will be called instead of wait(), when one
		  of this slave's child changes status pause() will be
		  interupted by SIGUNPAUSE and wait() will be restarted*/
		return;
	}
	TRACE("child not found returning");
	return;
}


/*The only way to catch newly created process immediately after it returns 
 *from fork is to force this process to pause after returning.
 *It can be done by saving the code that is after the fork and replacing it
 *with infinite loop:
 *again:
 *                    movl $29, %eax #call pause()
 *                    int $0x80
 *                    jmp again
 *                    nop
 *                    nop
 *                    nop
 *After catching a new child old code is restored as well as contents
 *of registers.
 **/
void before_fork(void)
{
	unsigned long code[] = {0x00001db8, 0xeb80cd00, 0x909090f7}; 
	TRACE("trying to get memory from = %d", slave->pid);
	slave_rpl_mem((void *)slave->regs.eip, code, sizeof(code));
	/*for(i = 0; i < 3; i++)
	  fprintf(stderr,"%08lx ", slave->rpl_code[i]);
		fprintf(stderr, "\n");*/
	TRACE("memory set");
}


void after_fork(void)
{
	pid_t cpid;
	int status;
	struct task *new_child, *oldslave;
	if ((cpid = RETURN_VALUE) < 0)
		return;

	new_child = add_slave(cpid, slave);
	add_child(new_child);
	/*restoring parent's memory*/
	slave_restore_mem();
	
	/*Init tracing of a new child*/
	oldslave = slave;
	slave = new_child;
	
	TRACE("Trying to attach process %d", cpid);
	Ptrace(PTRACE_ATTACH, cpid, 0, 0);
		
	while (waitpid(cpid, &status, 0) <= 0)
		if (errno != EINTR)
			err_sys("waitpid");
	
	ASSERT(wifstopped(status) && wstopsig(status) == SIGSTOP);
	
	Ptrace(PTRACE_SETOPTIONS, cpid, 0,(void*)PTRACE_O_TRACESYSGOOD);
	TRACE("child %d started", cpid);
	set_slave_return(0);
	slave_restore_mem();

	Ptrace(PTRACE_SYSCALL, cpid, 0, 0);
	TRACE("attached to process %d", cpid);

	/*continue tracing of parent*/
	slave = oldslave;
}

#ifndef CLONE_SYSVSEM
# define CLONE_SYSVSEM 0
#endif
/*As for now, okfs doesn't support threads, so any clone call
 *that demands sharing of anything beetwen parent and child will fail.*/
void before_clone(void)
{
	unsigned long clone_flags = ARG1, stack_start = ARG2;
	if ((clone_flags & (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
			    CLONE_VFORK | CLONE_PARENT | CLONE_SYSVSEM)) 
	    || stack_start != 0){
		not_supported();
		return;
	}
	/*CALL_NR = 2;
	  Ptrace(PTRACE_SETREGS, slave->pid, 0, &slave->regs);*/
	
	before_fork();
	slave->next_call = after_fork;	
}

/*only for debugging*/
void before_execve(void)
{
#ifdef DEBUG_OKFS 
	char what[16];
	slave_memget(what, (void *) ARG1, 16);
	what[15] = 0;
	TRACE("calling execve %s", what);
#endif
}

/*
 *When child calls execve ptrace returns 3 times,
 *once before execution of execve and two times after.
 *This extra return occurs only when execve call is successful.
 *This function copes with it.
 */
void after_execve(void)
{
	int status, i;
	TRACE("execve has returned");
	if (RETURN_VALUE < 0)
		return;
	
	for(i = 0; i < OPEN_MAX; i++){
		if (slave->fd_tab[i])
			TRACE("%d is open", i);
		if (slave->fd_tab[i] && slave->close_on_exec[i]){
			okfs_close_fd(slave, i);
		}
	}
	
	Ptrace(PTRACE_SYSCALL, slave->pid, 0, 0);
	while (waitpid(slave->pid, &status, 0) < 0)
		if(errno != EINTR)
			err_sys("waitpid");
	
	/*I don't know why but waitpid doesn't return here SYSCALL_STOP but
	 *SIGTRAP.*/
	if (!(wifstopped(status) && wstopsig(status) == SIGTRAP))
		err_sys("execve wasn't stopped by SIGTRAP as excepted");
}

