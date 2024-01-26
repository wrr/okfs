#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include "error.h"

int
main(int argc, char *argv[])
{
	struct stat64 buf, buf3;
	struct stat buf2;
	int fd, ret;
	if (stat64("./", &buf) < 0)
		err_ret("stat64 ./");
	if ((fd = open("/etc/test_file", O_RDONLY)) < 0)
		err_sys("open");
	
	if (fstat64(fd, &buf3) < 0)
		err_sys("fstat64");
	
	if (lstat64("/etc/test_file", &buf) < 0)
		err_ret("lstat64");

	if (lstat64("/etc/test_file", &buf) < 0)
		err_ret("lstat64 2nd");

	if ((ret = lstat("/etc/test_file", &buf2)) < 0)
		err_ret("lstat %d", ret);
	if (buf2.st_ino != buf.st_ino){
		err_quit("inode number mismatch %d %d\n", 
			buf2.st_ino, buf.st_ino);
	}
	assert(buf3.st_ino == buf.st_ino);

	if (stat64("/etc", &buf) < 0)
		err_ret("stat");
	if (!S_ISDIR(buf.st_mode))
		err_quit("stat /etc is not a dir!");
	exit(0);
}
