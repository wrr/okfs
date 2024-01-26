#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "error.h"

int
main(void)
{

	if (mkdir("./localfs_test_dir/.hmmm", 0600) < 0)
		err_sys("mkdir");
	
	if (rmdir("./localfs_test_dir/.hmmm") < 0)
		err_sys("rename");
	exit(0);
}
