/*$Id: enosys_test.c,v 1.2 2006/05/06 22:14:46 jan_wrobel Exp $*/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#include "tests.h"
#include "error.h"

/*check if okfs correctly handles slave's unimplemented system call*/
int
main(void)
{
	int retval;
	
	asm("\
	movl $400, %%eax \n\t\
        int $0x80 \n\t\
        mov %%eax, %0 \n\t"
	    :
	    :"m"(retval)
	    : "eax");
	
	if (retval != -ENOSYS)
		err_quit("wrong value returned (should be -ENOSYS)");
	exit(0);
}
