#$Id: Makefile,v 1.2 2006/05/06 22:14:46 jan_wrobel Exp $
CC       = gcc
CFLAGS = -Wall $(DEBUG_FLAG) -D_GNU_SOURCE 

LIBC_DEP_OBJS = error.o wrap.o okfs_lib.o
KERNEL_DEP_OBJS = okfs_unistd.o
LIBC_DEP_SRCS = $(LIBC_DEP_OBJS:.o=.c)
KERNEL_DEP_SRCS = $(KERNEL_DEP_OBJS:.o=.c)

all: dep $(LIBC_DEP_OBJS) $(KERNEL_DEP_OBJS) 

$(KERNEL_DEP_OBJS): 
	$(CC) $(CFLAGS) -D__KERNEL__ -I$(KERNEL_SOURCES)/include -c $(@:.o=.c)


dep: .depend

.depend: $(KERNEL_DEP_SRCS) $(LIBC_DEP_SRCS)
	$(CC) -MM $(CFLAGS) -I$(KERNEL_SOURCES) $(KERNEL_DEP_SRCS) 1> .depend
	$(CC) -MM $(CFLAGS) $(LIBC_DEP_SRCS) 1>> .depend

ifneq ($(wildcard .depend),)
include .depend
endif
