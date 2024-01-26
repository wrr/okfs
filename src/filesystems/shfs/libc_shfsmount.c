/*$Id: libc_shfsmount.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
/*
 *  This file was originally part of SHell File System
 *  See http://shfs.sourceforge.net/ for more info.
 *  Copyright (C) 2002-2004  Miroslav Spousta <qiq@ucw.cz>

 *  It is modified for Out of Kernel File System. 
 *  Copyright (C) 2004-2005 Jan Wrobel <wrobel@blues.ath.cx>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License Version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mntent.h>
#include <unistd.h>
#include "stdio.h"
#include "error.h"

/*this file uses glibc name space so it should be separated*/
int
update_mtab(char *mnt, char *dev, char *opts)
{
	int fd;
	FILE *mtab;
	struct mntent ment;

        ment.mnt_fsname = dev;
        ment.mnt_dir = mnt;
        ment.mnt_type = "shfs";
        ment.mnt_opts = opts;
        ment.mnt_freq = 0;
        ment.mnt_passno = 0;
	
        if ((fd = open(MOUNTED"~", O_RDWR|O_CREAT|O_EXCL, 0600)) == -1) {
                err_ret("Can't get "MOUNTED"~ lock file");
                return -1;
        }
        close(fd);
        if ((mtab = setmntent(MOUNTED, "a+")) == NULL) {
                err_ret("Can't open " MOUNTED);
                return -1;
        }
        if (addmntent(mtab, &ment) == 1) {
                err_ret("Can't write mount entry");
                return -1;
        }
        if (fchmod(fileno(mtab), 0644) == -1) {
                err_ret("Can't set perms on "MOUNTED);
                return -1;
        }
        endmntent(mtab);
        if (unlink(MOUNTED"~") == -1) {
                err_ret("Can't remove "MOUNTED"~");
                return -1;
        }
	return 1;
}
