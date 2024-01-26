#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include "error.h"

inline void test_gettimeofday()
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) < 0)
		err_sys("gettimeofday");
}

inline void test_fork()
{
	pid_t pid;
	if ((pid = fork()) < 0)
		err_sys("fork");
	if (pid == 0){
		exit(0);
	}
}

inline void test_signal()
{
	if (signal(5, SIG_IGN) < 0)
		err_sys("signal");
}

inline void test_getpid()
{
	pid_t pid;
	if ((pid = getpid()) < 0)
		err_sys("getpid");
}

inline void test_getcwd()
{
	char buf[256];
	if (getcwd(buf, 256) < 0)
		err_sys("getcwd");
}


inline void test_chdir()
{
	if (chdir("/usr/local/bin") < 0)
		err_sys("chdir");
}

double r = 2.7;
inline void test_loop(int i)
{
	r *= sin(sin(log(cos(sin((double) i))))) / sin(1234123);
	
}

inline void test_open()
{	
	
	int fd;
	if ((fd = open("/tmp/foo/foo/foo/foo/foo/foo/foo/foo/foo/bar.txt", O_RDONLY)) < 0)
		err_sys("open");
	close(fd);	
}

int fd;
char *buf;
inline void write_setup(int size)
{
	buf = (char *)malloc(size);
	memset(buf, 'A', size);
	if ((fd = open("/net/bar", O_WRONLY |O_CREAT,  S_IRWXU)) < 0)
		err_sys("open");
}
inline void test_write(int size)
{
	
	if (write(fd, buf, size) != size)
		err_sys("write");
	sync();
}

inline void read_setup(int size)
{
	buf = (char *)malloc(size);
	if ((fd = open("/net/bar", O_RDONLY,  S_IRWXU)) < 0)
		err_sys("open");
}

inline void test_read(int size)
{
	
	if (read(fd, buf, size) != size)
		err_sys("read");
	//sync();
	//close(fd);	
}

inline void test_cp(int size)
{
	
	char buf[256];
	sprintf(buf, "./test_cp.sh %d", size);
	if (system(buf) < 0)
		err_sys("system");
	//sync();
	//close(fd);	
}

int
main(int argc, char **argv)
{
	struct timeval start, end, diff;
	int cnt = 0, i;
	
	if (argc != 2)
		err_quit("args");
	cnt = atoi(argv[1]);
	
	
	//printf("executing %d\n", cnt);
	//read_setup(cnt);
	//write_setup(cnt);
	
	gettimeofday(&start, NULL);
	
	for(i = 0; i < cnt; i++){
		test_gettimeofday();
		//test_fork();
		//test_signal();
		//test_getcwd();
		//test_getpid();
		//test_chdir();
		//test_loop(i);
	}
	//test_write(cnt);
	//test_read(cnt);
	//test_cp(cnt);

		
	
	gettimeofday(&end, NULL);
	
	/*if (end.tv_usec < start.tv_usec){
		end.tv_usec += 1000000;
		end.tv_sec -= 1;

		}*/
	timersub(&end, &start, &diff);
	//res = 1000000 * (int)diff.tv_sec + (int)diff.tv_usec;
	
	printf("%d.%06d\n", (int)diff.tv_sec, (int)diff.tv_usec);
	//printf("%f\n", res /  cnt / 1000000.0);
	return 0;
}
