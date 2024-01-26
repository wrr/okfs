#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include "error.h"

int
main(int argc, char *argv[])
{
	int pfd[2], fd, pfd2[2];
	if (pipe(pfd) < 0)
		err_sys("pipe");
	if ((fd = open("/dev/null", O_RDONLY)) < 0)
		err_sys("open");
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, pfd2) == -1)
		err_sys("socketpair");
	assert(pfd[0] != fd && pfd[1] != fd && pfd2[0] != fd && pfd2[1] != fd
	       && pfd[0] != pfd[1] && pfd[0] != pfd2[0] && pfd[0] != pfd2[1]
	       && pfd[1] != pfd2[0] && pfd[1] != pfd2[1] && 
	       pfd2[2] != pfd2[1]);
	exit(0);
}
