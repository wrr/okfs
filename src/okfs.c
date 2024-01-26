/*$Id: okfs.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/

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

#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include "vfs_open.h"
#include "vfs_mount.h"
#include "task.h"
#include "okfs_signal.h"
#include "libc.h"
#include "wrap.h"
#include "entry.h"
#include "fake_root.h"
#include "debug.h"

static struct option const long_options[] = {
	{ "fakeroot", no_argument, NULL, 'r' },
	{ "fstab", required_argument, NULL, 'f' },
	{ "tmpdir", required_argument, NULL, 't' },
	{ "help", no_argument, NULL, 'h' },
	{ 0, 0, 0, 0 }
};

extern char *optarg;
extern int optind, opterr, optopt;

//TODO: solve errno problem
int errno;

static void set_default_handlers()
{
	int i;
	for(i = 1; i < _NSIG; i++)
		if (i != SIGKILL && i != SIGSTOP)
			simple_sigaction(SIGINT, SIG_DFL, 0);
}

static void sigint(int sig)
{
	exit(1);	
}

static void init_signal_handlers()
{
	Simple_sigaction(SIGPIPE, SIG_IGN, SA_RESTART);
	Simple_sigaction(SIGINT, sigint, SA_RESTART);
	Simple_sigaction(SIGQUIT, sigint, SA_RESTART);
	Simple_sigaction(SIGABRT, sigint, SA_RESTART);
}


static void usage(char * progname)
{
	stderr_print(
"Out of Kernel FileSystem \n"
"UNSTABLE, DEVELOPMENT VERSION\n"
"THIS SOFTWARE COMES WITH ABSOLUTELY NO WARRANTY! USE AT YOUR OWN RISK!\n\n"
"Usage: %s [options] [progname]\n\n"
"Options supported:\n"
"   --fstab file or -f file    Mount filesystems specified in file\n"
"   --fakeroot or -r           Fake root environment\n"
"   --tmpdir file or -t file   Specify temporary directory to use\n"
"   --help or -h               Print this help screen\n\n"
"[progname]     Specify program with arguments to run, default bash\n", progname);
	exit(0);
}

/*
  Calls function responsible for handling system call that slave is just
  entering or leaving.
*/
static void handle_syscall()
{
	Ptrace(PTRACE_GETREGS, slave->pid, 0, &slave->regs);
	if (CALL_NR > 0 && CALL_NR < NR_syscalls){
		get_slave_args(CALL_NR);	     
	}
	else 
		/*some strange, unknown, not existing system call*/
		get_slave_args(0);
	
	if(!slave->during_call){
		/*slave enters system call*/
		if (CALL_NR > NR_syscalls)
			TRACE("before unimplemented call nr = %ld",
			      CALL_NR);
		else 
			TRACE("before %s call %ld", 
			      call[CALL_NR].name, RETURN_VALUE);
		
		slave->during_call = CALL_NR;
		
		if (CALL_NR > NR_syscalls)
			/*better not to pass this call to kernel*/
			unimplemented_call();
		else{
			slave->next_call = call[CALL_NR].call_after;
			if (call[CALL_NR].call_before)
				/*call handler for this call*/
				call[CALL_NR].call_before();
		}
	}
	else{
		/*slave leaves system call*/
		
		/*
		  Handle slave's call that could be different than one
		  executed by kernel (e.g. pause() instead of wait()),
		  so call numer in eax could be wrong and should be restored.
		*/
		CALL_NR = slave->during_call;
		
		slave->during_call = 0;
		if (slave->restart_call){
			TRACE("pause returned");
			/*force call saved in restart_call to be 
			  called again*/
			CALL_NR = slave->restart_call;
			RETURN_VALUE = -ERESTARTSYS;
			set_slave_args();
			slave->restart_call = 0;
		}
		else if (slave->next_call)
			/*call handler for this call*/
			slave->next_call();
		slave->next_call = NULL;

		/*debugging*/
		if (CALL_NR > NR_syscalls)
			TRACE("after unimplemented call nr = %ld",
			      CALL_NR);
		else{
			if (RETURN_VALUE < 0)
				TRACE("after %s call = %s",
				      call[CALL_NR].name,
				      strerror(-RETURN_VALUE));
			else
				TRACE("after %s call = %ld",
				      call[CALL_NR].name,
				      RETURN_VALUE);
		}
	}
}

/*
 * Checks if traced process receives signal, exited or was killed.
 * returns signal to deliver to this process by next ptrace() call
 * or 0 if none. 
 */
static int check_status(int status)
{
	
	if (wifstopped(status))
		/*traced process receives signal, check if it isn't
		  one that has to be specialy handled (e.g. SIGSTOP,
		  SICONT), if not, return this signal to be delivered
		  by next ptrace() call.
		 */
		return(process_signal(wstopsig(status), status));
	
	if(wifexited(status)){
		TRACE("child %d exited returning status %d", 
		      slave->pid, wexitstatus(status));
		change_slave_state(SLAVE_ZOMBIE, status);
	}	
	else if(wifsignaled(status)){
		TRACE("child %d killed by signal %s",
		      slave->pid,
		      strsignal(wtermsig(status)));
		change_slave_state(SLAVE_ZOMBIE, status);
	}
	return 0;
}


/*This is a main loop of okfs. It is responsible of recognizing
  which of all slave processes stopped and why.
  There are following reasons that causes traced process to be stopped:
  -process receives signal
  -process exited
  -process was terminated by signal
  -process enters system call
  -process leaves system call
*/
static void guard_slaves()
{
	int status, sig = 0;
	pid_t pid;
		
	for(;;){
		if (slave && slave->state != SLAVE_ZOMBIE){
			/*If this slave was stopped all signals
			  queued to it have to be delivered*/
			deliver_pending_signals();
			Ptrace(PTRACE_SYSCALL, slave->pid, 0, (void *)sig);
		}

		while ((pid = wait(&status)) <= 0){
			if (errno != EINTR)
				err_sys("wait");
		}
		slave = find_slave(pid);
		
		if (!WIFSYSCALL(status)){
			/*slave exited, terminated or stopped 
			  by signal delivery*/
			sig = check_status(status);
			continue;
		}	
		/*slave enters or leaves system call*/		
		sig = 0;

		handle_syscall();		
	}
}

/*
  @exec_cmd - program to run
  @exec_arg - arguments to this program
*/
static void run_first_slave(char *exec_cmd, char **exec_arg)
{
	int status;
	pid_t pid;
	
	if ((pid = Fork()) == 0){
		TRACE("executing %s", exec_cmd);
		Ptrace(PTRACE_TRACEME, 0, 0, 0);
		Close(3);
		execvp(exec_cmd, exec_arg);
		err_sys("execlp %s", exec_cmd);
	}
	
	init_signal_handlers();
	slave = add_slave(pid, NULL);
	atexit(slay_slaves);

	while(wait(&status) < 0)
		if (errno != EINTR)
			err_sys("wait");		
	
	if (!(wifstopped(status) && wstopsig(status) == SIGTRAP))
		err_quit("can't trap children");
	TRACE("attached to process %d", slave->pid);	
	/*this option provides a way for the tracing parent to distinguish
	  between a syscall stop and SIGTRAP delivery */
	Ptrace(PTRACE_SETOPTIONS, slave->pid, 0,
	       (void*)PTRACE_O_TRACESYSGOOD);			
}
 
int main(int argc, char **argv)
{
	char *exec_cmd = "bash", **exec_arg, c;
	char *tmpdir = Strdup("/tmp");

	init_call_table();
	loginit(LOGFILE);
	set_default_handlers();
	
	while ((c = getopt_long(argc, argv, "rf:t:h", long_options,
				(int *) 0)) != EOF) {
		switch (c) {
		case 'r':
			fake_root_init();
			break;
		case 'f':
			mount_from_fstab(optarg);
			break;
		case 't':
			free(tmpdir);
			tmpdir = Strdup(optarg);
			break;
		case 'h':
		default:
			usage(argv[0]);
		}
	}
	if (optind < argc){
		int i;
		exec_cmd = argv[optind];
		exec_arg = (char **)Malloc((argc - optind + 1) * 
					   sizeof(char**));
		/*TODO: these args don't really work because 
		  getopt_long treats them as args to okfs, so for example
		  ./okfs ls -al causes error*/
		
		for(i = optind; i < argc; i++)
			exec_arg[i - optind] = argv[i];
		exec_arg[i - optind] = NULL;
	}
	else{
		exec_arg = Malloc(2 * sizeof(char *));
		exec_arg[0] = exec_cmd;
		exec_arg[1] = NULL;
	}
	
	create_tmpfile(tmpdir);
	free(tmpdir);
	
	run_first_slave(exec_cmd, exec_arg);		
	guard_slaves();

	exit(0);
}

