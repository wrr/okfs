#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "error.h"

int
main(int argc, char *argv[])
{
	char *cwd;
	if (!(cwd = getcwd(NULL, 0)))
		err_sys("getcwd");
	
	if (chdir("/etc/foo") < 0)
		err_sys("chdir");
	
	if (!(cwd = getcwd(NULL, 0)))
		err_sys("getcwd2");
	
	if (strcmp(cwd, "/etc/foo") != 0)
		err_sys("getcwd3");
	exit(0);
}
