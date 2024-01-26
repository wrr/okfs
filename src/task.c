/*$Id: task.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
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
#include <linux/errno.h> 
#include <linux/string.h>
#include <linux/ptrace.h>
#include "memacc.h"
#include "entry.h"
#include "wrap.h"
#include "debug.h"
#include "libc.h"
#include "okfs_lib.h"
#include "okfs_unistd.h"

/*tasks control functions*/
struct task *slave;
static struct task *first_task;

/*Change state of current slave to given one.
  @state - new state of task
  @wait_status - value that will be returned to parent of this task
  as second argument in waitpid() call*/
void change_slave_state(int state, int wait_status)
{
	int i;
	TRACE("changing state to %d", state);
	slave->state = state;
	slave->exit_code = wait_status;

	if (state == SLAVE_ZOMBIE){
		TRACE("closing all fds");
		for(i = 0; i < OPEN_MAX; i++)
			if (slave->fd_tab[i])
				okfs_close_fd(slave, i); 
	}
	
	if (slave->ppid != 1){
		/*notify parent about new status*/
		if (state == SLAVE_ZOMBIE || state == SLAVE_STOPPED)
			kill(slave->ppid, SIGUNPAUSE);
	}	
	else if (state == SLAVE_ZOMBIE){
		TRACE("removing orphaned slave");
		remove_slave(slave->pid);
	}
}

/*remove all infromation about traced task with given pid*/
void remove_slave(pid_t pid)
{
	struct task *rt, *p;
	struct child *cp, *tmp;

	if (first_task->pid == pid){
		rt = first_task;
		first_task = first_task->next_slave;

	}
	else
		for(p = first_task; p->next_slave; p = p->next_slave)
			if (p->next_slave->pid == pid){
				rt = p->next_slave;
				p->next_slave = p->next_slave->next_slave;
				break;
			}
	if (rt == slave)
		slave = NULL;

	free(rt->cwd);
	for(cp = rt->childp; cp;){
		cp->child_task->ppid = 1;
		if (cp->child_task->state == SLAVE_ZOMBIE)
			remove_slave(cp->child_task->pid);
		tmp = cp;
		cp = cp->next_child;
		free(tmp);
	}
	
	
	free(rt);
	/*no more process to control*/
	if (first_task == NULL)
		exit(0);
}

/*remove child of current slave that has given pid*/
void remove_child(pid_t pid)
{
	struct child *tmp, *p;
	if (slave->childp->child_task->pid == pid){
		tmp = slave->childp;
		slave->childp->child_task->ppid = 1;
		slave->childp = slave->childp->next_child;
		free(tmp);
		return;
	}
	for(p = slave->childp; p; p = p->next_child)
		if (p->next_child->child_task->pid == pid){
			tmp = p->next_child;
			tmp->child_task->ppid = 1;
			p->next_child = p->next_child->next_child;
			free(tmp);
			return;
		}
	err_quit("remove child fatal error");
}

/*add new slave to trace*/
struct task *add_slave(pid_t pid, struct task *parent)
{
	struct task *ns = (struct task *) Malloc(sizeof(struct task));
	int i;

	ns->next_slave = first_task;
	first_task = ns;
	ns->childp = NULL;
	ns->stopped_child = NULL;
	ns->pid = pid;

	memset(&ns->callfs, 0, sizeof(ns->callfs));
	sigemptyset(&ns->sigpending_mask);
	
	if (parent){
		ASSERT(!parent->callfs.arg1_fsp && !parent->callfs.arg2_fsp &&
		       !parent->callfs.path1 && !parent->callfs.path2 && 
		       !parent->callfs.canon_path1 && !parent->callfs.canon_path2 
		       && !parent->callfs.rel_path1 && !parent->callfs.rel_path2);
		memset(&ns->callfs, 0, sizeof(struct fs_resolve));
		ns->ppid = parent->pid;
		ns->gid = parent->gid;
		ns->uid = parent->uid;
		for(i = 1; i < _NSIG; i++)
			ns->sighandler[i] = parent->sighandler[i];
		ns->nocldstop = parent->nocldstop;
		ns->cwd = Strdup(parent->cwd);
		ns->regs = parent->regs;
		ns->save_regs = parent->save_regs;
		ns->has_stack_args = 0;

		for(i = 0; i < sizeof(ns->arg) / sizeof(long); i++)
			ns->arg[i] = slave->arg[i];
		
		if (parent->rpl_code){
			ns->rpl_code_len = parent->rpl_code_len;
			ns->rpl_dest = parent->rpl_dest;
			ns->rpl_code = Malloc(ns->rpl_code_len);
			okfs_memcpy(ns->rpl_code, parent->rpl_code, 
			       ns->rpl_code_len);
		}

		/*child shares parent's descriptiors*/
		okfs_memcpy(ns->fd_tab, parent->fd_tab, 
		       OPEN_MAX * sizeof(struct okfs_file *));
		okfs_memcpy(ns->fsfd, parent->fsfd, 
		       OPEN_MAX * sizeof(int));
		okfs_memcpy(ns->close_on_exec, parent->close_on_exec, 
		       OPEN_MAX * sizeof(int));
		
		for(i = 0; i < OPEN_MAX; i++)
			if (ns->fd_tab[i]){
				TRACE("child inherits %d fd", i);
				ns->fd_tab[i]->nshared++;
			}
	}
	else{
		for(i = 1; i < _NSIG; i++)
			ns->sighandler[i] = SIG_DFL;
		
		ns->cwd = getcwd(NULL, 0);
		ns->ppid = 1;
		ns->gid = getgid();
		ns->uid = getuid();
		memset(ns->fd_tab, 0, OPEN_MAX * sizeof(struct okfs_file *));
		memset(ns->fsfd, 0, OPEN_MAX * sizeof(int));
		memset(ns->close_on_exec, 0, OPEN_MAX * sizeof(int));
		ns->nocldstop = 0;
		
		ns->rpl_code_len = 0;
		ns->rpl_dest = NULL;
		ns->rpl_code = NULL;
	}

	ns->state = SLAVE_RUNNING;
	ns->exit_code = 0;
	ns->retval_saved = 0;
	ns->restart_call = 0;
	ns->next_call = NULL;
	ns->during_call = 0;
	return ns;
}

/*add new child to list of current slave's children*/
void add_child(struct task *child_task)
{
	struct child *new_child = (struct child *)Malloc(sizeof(struct child));
	new_child->child_task = child_task;
	new_child->next_child = slave->childp;
	slave->childp = new_child;
}

/*
  When okfs interrupts it should kill all its slaves not only detach them.
  Detaching may cause problems since they can have messed up memory.
*/
void slay_slaves(void)
{
	struct task *p;
	for(p = first_task; p; p = p->next_slave){
		TRACE("killing process %d", p->pid);
		kill(p->pid, SIGKILL);		
		okfs_ptrace(PTRACE_DETACH, p->pid, 0, 0);
	}
}

struct task *find_slave(pid_t pid)
{
	struct task *p;
	for(p = first_task; p; p = p->next_slave)
		if (p->pid == pid)
			return(p);
	err_quit("child with pid %d doesn't exist", pid);
	return(NULL);
}

/*Get arguments of slave's call. For most calls arguments are in registers,
  but there are few that have more than 5 arguments and they are passed on
  stack*/
void get_slave_args(int callnr)
{
	if (!call[callnr].has_stack_args){
		slave->arg[0] = slave->regs.ebx;
		slave->arg[1] = slave->regs.ecx;
		slave->arg[2] = slave->regs.edx;
		slave->arg[3] = slave->regs.esi;
		slave->arg[4] = slave->regs.edi;
		slave->has_stack_args = 0;
	}
	else{
		/*only 6 args are now supported. Is there a system call
		 that uses more?*/
		slave_memget(slave->arg, (void *)slave->regs.esp+sizeof(int), 
			  sizeof(slave->arg));
		/*for(i = 0; i < 6; i++)
		  TRACE("%08lx ", slave->arg[i]);*/
		slave->has_stack_args = 1;
	}
}

/*set system call arguments for current slave*/
void set_slave_args(void)
{
	if (slave->has_stack_args){
		slave_memset((void *)slave->regs.esp + sizeof(int), slave->arg,
			  sizeof(slave->arg));
	}
	else{
		slave->regs.ebx = slave->arg[0];
		slave->regs.ecx = slave->arg[1]; 
		slave->regs.edx = slave->arg[2];  
		slave->regs.esi = slave->arg[3];  
		slave->regs.edi = slave->arg[4];  
	}
	Ptrace(PTRACE_SETREGS, slave->pid, 0, &slave->regs);\
}

static void return_saved(void)
{
	set_slave_return(slave->retval_saved);
}

/*do not execute current call, force RETVAL do be returned to slave*/
void force_call_return(int retval)
{
	TRACE("Child will receive call return value: %d", retval);
	slave->retval_saved = retval;
	slave->next_call = return_saved;
	dummy_call();
}

/*do not execute current call, force -ENOSYS do be returned to slave*/
void unimplemented_call(void)
{
	TRACE("Unimplemented call nr = %ld", CALL_NR);
	force_call_return(-ENOSYS);	
}

/*do not execute current call, force -EOPNOTSUPP do be returned to slave*/
void not_supported(void)
{
	force_call_return(-EOPNOTSUPP);	
}

/*execute some neutral call instead of a current one*/
void dummy_call(void)
{
        CALL_NR = DUMMY_CALL_NR;
	set_slave_args();
}
