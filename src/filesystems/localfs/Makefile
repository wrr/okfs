#$Id: Makefile,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $
CFLAGS   = -Wall $(DEBUG_FLAG) -D_GNU_SOURCE -I.. -I../.. -I../../lib

KERNEL_DEP_OBJS = localfs.o
KERNEL_DEP_SRCS = $(KERNEL_DEP_OBJS:.o=.c)

all: dep $(KERNEL_DEP_OBJS) 

$(KERNEL_DEP_OBJS): 
	$(CC) $(CFLAGS) -D__KERNEL__ -I$(KERNEL_SOURCES)/include -c $(@:.o=.c)

dep: .depend
.depend: $(KERNEL_DEP_SRCS)
	$(CC) -MM $(CFLAGS) -D__KERNEL__ -I$(KERNEL_SOURCES) $(KERNEL_DEP_SRCS) 1> .depend

ifneq ($(wildcard .depend),)
include .depend
endif
