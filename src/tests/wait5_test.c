/*$Id: wait5_test.c,v 1.2 2006/05/06 22:14:46 jan_wrobel Exp $*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "tests.h"
#include "error.h"

int
main(void)
{
	pid_t pid1, pid2;
	int status, r;
	if ((pid1 = fork()) == 0){
		exit(0);
	}
	else if (pid1 < 0)
		err_sys("fork1");
	
	if ((pid2 = fork()) == 0){
		raise(SIGSTOP);
		exit(0);
	}
	else if (pid2 < 0)
		err_sys("fork2");
	
	else{
		sleep(1);
		if ((r = wait4(-1, &status, WUNTRACED, NULL)) < 0)
			err_sys("wait");
		else if (r != pid2 && r != pid1)
			wrong_status(status);
		kill(pid2, SIGKILL);
	}
	exit(0);
}
