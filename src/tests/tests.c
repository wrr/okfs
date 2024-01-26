/*$Id: tests.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
#include <sys/types.h>
#include <sys/wait.h> 
#include <string.h>
#include "error.h"
#include "tests.h"
struct task *slave = NULL;

void wrong_status(int status)
{
	if (WIFEXITED(status))
		err_quit("child exited returning status %d\n", 
			 WEXITSTATUS(status));
	if (WIFSTOPPED(status))
		err_quit("child stopped by sginal %s",
			 strsignal(WSTOPSIG(status)));
	
	if(WIFSIGNALED(status))
		err_msg("child killed by signal %s",
		      strsignal(WTERMSIG(status)));

	err_sys("wait returned unknown status");	
}
