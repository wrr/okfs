#$Id: Makefile,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $
CFLAGS   = -Wall -D_GNU_SOURCE $(DEBUG_FLAG) -I.. -I../.. -I../../lib

LIBC_DEP_OBJS = libc_shfsmount.o
KERNEL_DEP_OBJS = shfs.o shfsmount.o proto.o proc.o
LIBC_DEP_SRCS = $(LIBC_DEP_OBJS:.o=.c)
KERNEL_DEP_SRCS = $(KERNEL_DEP_OBJS:.o=.c)


all: dep $(LIBC_DEP_OBJS) $(KERNEL_DEP_OBJS) 


$(KERNEL_DEP_OBJS): 
	$(CC) $(CFLAGS) -D__KERNEL__ -I$(KERNEL_SOURCES)/include -c $(@:.o=.c)


%.h: %.in
	sed -e '/^[ ]*#/d;/^$$/d;s/\\/\\\\/g;s/\"/\\\"/g;s/^\(.*\)$$/\"\1\\n\"/' <$< | sed -e "s/'/'\\\\\\\\''/g" >$@


dep: .depend
.depend: $(KERNEL_DEP_SRCS) $(LIBC_DEP_SRCS)
	$(CC) -MM $(CFLAGS) -D__KERNEL__ -I$(KERNEL_SOURCES) $(KERNEL_DEP_SRCS) 1> .depend
	$(CC) -MM $(CFLAGS) $(LIBC_DEP_SRCS) 1>> .depend

ifneq ($(wildcard .depend),)
include .depend
endif


