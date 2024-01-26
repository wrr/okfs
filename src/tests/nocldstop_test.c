/*$Id:&*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

#include "tests.h"
#include "error.h"

int sigok = 0;

void sig_chld(int n)
{
	if (!sigok)
		err_quit("sigchld received");
}

int
main(void)
{
	pid_t cpid;
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NOCLDSTOP;
	act.sa_handler = sig_chld;
	if (sigaction(SIGCHLD, &act, NULL) < 0)
	err_sys("sigaction");
	
	if ((cpid = fork()) == 0){
		kill(getpid(), SIGSTOP);
	}
	else if (cpid < 0)
		err_sys("fork");
	else{
		sleep(1);
		act.sa_handler = NULL;
		sigok = 1;
		kill(cpid, SIGCONT);
	}
	exit(0);
}
