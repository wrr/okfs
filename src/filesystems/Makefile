#$Id: Makefile,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $
CFLAGS   = -Wall $(DEBUG_FLAG) -D_GNU_SOURCE -I.. -I../lib
SUBDIRS = localfs shfs
.PHONY: $(SUBDIRS)

LIBC_DEP_OBJS = vfs_libcmount.o
KERNEL_DEP_OBJS = vfs_namei.o vfs_dcache.o vfs_mount.o vfs_open.o vfs_readdir.o vfs_stat.o vfs_fd_calls.o vfs_path_calls.o
LIBC_DEP_SRCS = $(LIBC_DEP_OBJS:.o=.c)
KERNEL_DEP_SRCS = $(KERNEL_DEP_OBJS:.o=.c)


all: dep $(LIBC_DEP_OBJS) $(KERNEL_DEP_OBJS) $(SUBDIRS) 


$(KERNEL_DEP_OBJS): 
	$(CC) $(CFLAGS) -D__KERNEL__ -I$(KERNEL_SOURCES)/include -c $(@:.o=.c)

export

$(SUBDIRS): 
	$(MAKE) -C $@


dep: .depend
.depend: $(KERNEL_DEP_SRCS) $(LIBC_DEP_SRCS)
	$(CC) -MM $(CFLAGS) -D__KERNEL__ -I$(KERNEL_SOURCES) $(KERNEL_DEP_SRCS) 1> .depend
	$(CC) -MM $(CFLAGS) $(LIBC_DEP_SRCS) 1>> .depend

ifneq ($(wildcard .depend),)
include .depend
endif