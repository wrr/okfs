#$Id: Makefile,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $
CC       = gcc
CFLAGS   = -Wall -D_GNU_SOURCE -I.. -I../lib
.PHONY: testing
TESTS = fork_test wait1_test wait2_test wait3_test wait4_test wait5_test alarm_test getcwd_test_fs rmdir_test_fs errno_test_fs stat_test_fs open_test_fs pipe_test_fs readdir_test_fs fdopen_test_fs nocldstop_test enosys_test

all: $(TESTS)
$(TESTS): ../lib/error.o tests.o
tests.o: tests.h

check: all
	./testall.sh

clean: 
	rm -f $(TESTS) log;
