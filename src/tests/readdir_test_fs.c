#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include "error.h"

int
main(void)
{
	DIR *dp;
	if (!(dp = opendir("./localfs_test_dir/foo")))
		err_sys("opendir");
	if (!readdir(dp))
		err_sys("readdir");
	rewinddir(dp);
	exit(0);
}
