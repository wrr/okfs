/*$Id: wait4_test.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "tests.h"
#include "error.h"

void sig_int(int n)
{
	exit(3);
}

int
main(void)
{
	pid_t pid;
	int status, r;
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = sig_int;
	if (sigaction(SIGINT, &act, NULL) < 0)
		err_sys("sigaction");
	
	if ((pid = fork()) == 0){
		pause();
		exit(0);
	}
	else if (pid < 0)
		err_sys("fork");
	else{
		sleep(1);
		if (kill(pid, SIGSTOP) < 0)
			err_sys("kill 1");
		if (wait4(-1, &status, WUNTRACED, NULL) < 0)
			err_sys("wait 1");
		if (!(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP))
			wrong_status(status);
		if (kill(pid, SIGCONT) < 0)
			err_sys("kill 2");
		sleep(1);
		if ((r = wait4(pid, &status, WNOHANG | WUNTRACED, NULL)) < 0)
			err_sys("wait 2");
		else if (r != 0)
			wrong_status(status);		

		if (kill(pid, SIGINT) < 0)
			err_sys("kill 3");
		
		if (wait4(-1, &status, WUNTRACED, NULL) < 0)
			err_sys("wait 3");
		if (!(WIFEXITED(status) && WEXITSTATUS(status) == 3))
			wrong_status(status);
	}
	exit(0);
}
