#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include "error.h"

int
main(int argc, char *argv[])
{
	if (open(NULL, O_RDONLY) > 0 || errno != EFAULT)
		err_ret("open");
	if (mkdir(NULL, O_RDONLY) > 0 || errno != EFAULT)
		err_ret("mkdir");
	exit(0);
}
