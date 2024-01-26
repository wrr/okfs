#include <stdio.h>
#include <stdlib.h>
#include "error.h"

int
main(void)
{
	if (fopen("/etc/shadow", "w") == NULL)
		err_sys("fopen");
	exit(0);
}
