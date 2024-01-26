#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include "error.h"

int
main(int argc, char *argv[])
{
	if (open("/etc/test_file", O_RDONLY) < 0)
		err_sys("open");
	exit(0);
}
