/*$Id: wait1_test.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "error.h"
#include "tests.h"

int
main(void)
{
	pid_t cpid;
	int status = 0;

	if ((cpid = fork()) == 0)
		sleep(1);
	else if (cpid < 0)
		err_sys("fork");
	else{
		if (wait4(-1, &status, 0, NULL) <= 0)
			err_sys("wait for %d", cpid);
		if (!(WIFEXITED(status) && WEXITSTATUS(status) == 0))
			wrong_status(status);
	}
	exit(0);
}
