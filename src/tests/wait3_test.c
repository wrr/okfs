/*$Id:&*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include "tests.h"
#include "error.h"

void sig_int(int n)
{
	return;
}

int
main(void)
{
	pid_t cpid;
	int status;
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = sig_int;
	if (sigaction(SIGINT, &act, NULL) < 0)
		err_sys("sigaction");
	
	if ((cpid = fork()) == 0){
		sleep(1);
		kill(getppid(), SIGINT);
	}
	else if (cpid < 0)
		err_sys("fork");
	else{
		if (wait4(cpid, &status, WUNTRACED, NULL) < 0){
			if (errno != EINTR)
				err_sys("wait");
		}
		else
			wrong_status(status);
	}
	exit(0);
}
