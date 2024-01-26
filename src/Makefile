#$Id: Makefile,v 1.4 2006/05/06 22:14:45 jan_wrobel Exp $

#kernel source tree. 
KERNEL_SOURCES=/usr/src/linux-2.6.12.4


DEBUG_FLAG = #-g -DDEBUG
#-fprofile-arcs -ftest-coverage
CC       = gcc
CFLAGS = -Wall -O2 $(DEBUG_FLAG) -D_GNU_SOURCE -I. -I./filesystems -I./lib
SUBDIRS = lib filesystems tests


.PHONY: testing clean stats $(SUBDIRS)

KERNEL_DEP_OBJS = okfs.o memacc.o control.o entry.o okfs_signal.o task.o fake_root.o
KERNEL_DEP_SRCS = $(KERNEL_DEP_OBJS:.o=.c)


LINK = $(KERNEL_DEP_OBJS) lib/*.o filesystems/*.o filesystems/localfs/*.o filesystems/shfs/*.o  


all: dep okfs 

okfs: $(KERNEL_DEP_OBJS) $(SUBDIRS)
	$(CC) $(CFLAGS) -o $@ $(LINK)

$(KERNEL_DEP_OBJS): 
	$(CC) $(CFLAGS) -D__KERNEL__ -I$(KERNEL_SOURCES)/include -c $(@:.o=.c)

export 

$(SUBDIRS):
	$(MAKE) -C $@


check:	clean_cover
	$(MAKE) -C tests check

clean_cover:
	find . \( -name '*.da' -o -name '*.gcov' \) -type f|xargs rm -f;

clean: clean_cover
	find . \( -name '*.bb' -o -name '*.bbg' -o -name '*.o' -o -name '*~' -o -name '.depend' -o -name '*.gcno' -o -name 'okfs' -o -name 'log' \) -type f|xargs rm -f;		
	$(MAKE) -C tests clean;

stats:
	date >> stats;
	find . \( -name '*.c' -o -name '*.h' \) -print|xargs cat|wc >> stats;
	tail stats;

dep: .depend

.depend: $(KERNEL_DEP_SRCS)
	$(CC) -MM $(CFLAGS) -D__KERNEL__ -I$(KERNEL_SOURCES) $(KERNEL_DEP_SRCS) 1> .depend

ifneq ($(wildcard .depend),)
include .depend
endif
