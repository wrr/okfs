#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "error.h"

int
main(int argc, char *argv[])
{
	int i, uid;
	pid_t pid;
	for(i = 0; i < 200; i++)
		if ((pid = fork()) < 0)
			err_sys("fork");
		else if (pid == 0){
			if ((uid = getuid()) != 0)
				err_quit("test failed uid == %d", uid);
			exit(0);
		}
	while (wait(NULL) > 0);
	exit(0);
}
