/*$Id: wait2_test.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <assert.h>

#include "tests.h"
#include "error.h"

int
main(void)
{
	pid_t pid;
	int status;
	
	asm("\
	movl $2, %%eax \n\t\
        int $0x80 \n\t\
        mov %%eax, %0 \n\t"
	    :
	    :"m"(pid)
	    : "eax");
	if (pid == 0){
		if (kill(getpid(), SIGSTOP) < 0)
			err_sys("kill");
		exit(0);
	}
	else if (pid < 0)
		err_sys("fork");
	else{
		int flag = WUNTRACED;
		if (waitpid(pid, &status, flag) != pid)
			err_sys("waitpid");
		assert(flag == WUNTRACED);
		
		if (!(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP))
			wrong_status(status);
		if (kill(pid, SIGURG) < 0)
			err_sys("kill");
		if (kill(pid, SIGCONT) < 0)
			err_sys("kill");
		if (waitpid(pid, &status, WUNTRACED) != pid)
			err_sys("waitpid");
		if (!(WIFEXITED(status) && WEXITSTATUS(status) == 0))
			wrong_status(status);
	}
	exit(0);
}


