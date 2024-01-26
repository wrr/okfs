/*$Id: alarm_test.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "error.h"

int
main(int argc, char *argv[])
{
	alarm(1);
	pause();
	err_quit("alarm hasn't terminated proccess");
	return(23);
}
