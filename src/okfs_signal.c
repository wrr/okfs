/*$Id: okfs_signal.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
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
 * This file handles signal delivery to slave process. 
 * It isn't trivial task because SIGSTOP doesn't stop traced process
 * and OKFS has to simulate this stop.
 */

#include <linux/signal.h>
#include <linux/ptrace.h>
#include "task.h"
#include "debug.h"
#include "memacc.h"
#include "okfs_signal.h"
#include "okfs_unistd.h"
#include "libc.h"
#include "wrap.h"

#define NOT_SIG_DFL SIG_DFL + 1

/*In order to stop traced process when it receives SIGSTOP
 *or other stopping signal, it's forced to call pause(). 
 *It's done by  replacing code in traced process memory 
 *with this one:
 *                    movl $29, %eax #call pause()
 *                    int $0x80
 *                    nop
 *After receiving SIGCONT old code is restored as well as content
 *of registers.
 **/
static void stop_slave(int status)
{
	unsigned long code[] = {0x00001db8, 0x9080cd00};
	struct task *parent;
	TRACE("stopping slave");
	slave_rpl_mem((void *)slave->regs.eip, code, sizeof(code));
	slave->save_regs = slave->regs;
	
	change_slave_state(SLAVE_STOPPED, status);
	TRACE("notifing parent");
	if ((parent = find_slave(slave->ppid)) && !parent->nocldstop)
		if (kill(slave->ppid, SIGCHLD) < 0)
			TRACE("fail");
	    
}

static void wakeup_slave(void)
{
	int i;
	unsigned long code[] = {0x00001db8, 0x9080cd00};
	unsigned long rpl_code[2];
	
	TRACE("waking up slave");
	slave_memget(rpl_code, (void *)slave->save_regs.eip, sizeof(code));
	for(i = 0; i < 2; i++)
		ASSERT(code[i] == rpl_code[i]);
	slave_restore_mem();
	Ptrace(PTRACE_SETREGS, slave->pid, 0, &slave->save_regs);

	change_slave_state(SLAVE_RUNNING, 0);
	TRACE("done");
}

/*When process receives SIGCONT all signals that were queued 
 *when it was stopped have to be delivered*/
void deliver_pending_signals(void)
{
	int i;
	for(i = 1; i < _NSIG; i++)
		if (sigismember(&slave->sigpending_mask, i)){
			TRACE("killing child with signal %d", i);
			Kill(slave->pid, i);
			sigdelset(&slave->sigpending_mask, i);
		}
}

int process_signal(int sig, int status)
{
	/*SIGUNPAUSE isn't delivered to traced process because it is
	 *used only to inform parrent about changes of children's status*/
	if (sig == SIGUNPAUSE)
		return(0);

	if (STOPPED_TASK(slave->state)){
		TRACE("child is in stopped state");
		if (sig == SIGCONT){
			wakeup_slave();
			return(SIGCONT);
		}
		
		/*When process is stopped and it receives next stopping
		  signal it is ignored not queued.*/
		if (!STOPPING_SIG(sig))
			sigaddset(&slave->sigpending_mask, sig);
		return(0);
	}
	else{
		TRACE("child is not in stopped state but %d", slave->state);
		if (STOPPING_SIG(sig) && slave->sighandler[sig] == SIG_DFL){
			stop_slave(status);
			TRACE("slave stopped state = %d", slave->state);
			/*There is some error in 2.6.* kernels that
			  cause SIG_STOP to be delivered twice to traced
			  process. To avoid this okfs doesn't
			  push SIG_STOP to traced process (this signal can't 
			  be catched so there is nothing wrong about this 
			  behavior).*/
			//if (sig == SIGSTOP)
			//return(0);
		}
		return sig;
	}
}		


/*OKFS has to keep track of sigaction call in order to know
  for which stopping signals it has to peform default action.
  It has to also know when slave has to be notified when its child
  stops (what's its action for SIGCHLD).
*/
void after_rt_sigaction(void)
{
	int signum = ARG1;
	struct sigaction act, *actp = (struct sigaction *) ARG2;
	if (RETURN_VALUE < 0 || actp == NULL)
		return;
	if (STOPPING_SIG(signum)){
		slave_memget(&act, actp, sizeof(struct sigaction));
		TRACE("handler for %s has changed", strsignal(signum));
		if (act.sa_handler == SIG_DFL){
			slave->sighandler[signum] = SIG_DFL;
			TRACE("now it is SIG_DFL");
		}
		else 
			slave->sighandler[signum] = NOT_SIG_DFL;
		ASSERT(signum != SIGSTOP);
	}
	else if (signum == SIGCHLD){
		slave_memget(&act, actp, sizeof(struct sigaction));
		slave->nocldstop = ((act.sa_flags & SA_NOCLDSTOP) == 
				     SA_NOCLDSTOP);
		TRACE("slave %s be notified about stopping of its children",
		      slave->nocldstop ? "wont": "will");
	}
}
	

void after_sigaction(void)
{
	int signum = ARG1;
	struct old_sigaction act, *actp = (struct old_sigaction *) ARG2;

	if (RETURN_VALUE < 0 || actp == NULL)
		return;
	if (STOPPING_SIG(signum)){
		slave_memget(&act, actp, sizeof(struct old_sigaction));
		TRACE("handler for %s has changed", strsignal(signum));
		if (act.sa_handler == SIG_DFL){
			slave->sighandler[signum] = SIG_DFL;
			TRACE("now it is SIG_DFL");
		}
		else 
			slave->sighandler[signum] = NOT_SIG_DFL;
		if (signum == SIGSTOP)
			err_sys("after_sigaction: sig == SIGSTOP");
	}
	else if (signum == SIGCHLD){
		slave_memget(&act, actp, sizeof(struct old_sigaction));
		slave->nocldstop = ((act.sa_flags & SA_NOCLDSTOP) == 
				      SA_NOCLDSTOP);
		TRACE("slave %s be notified about stopping of its children",
		      slave->nocldstop? "wont": "will");
	}
}
