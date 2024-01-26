/*$Id: vfs_libcmount.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/

/*
 *  This file is part of Out of Kernel File System.
 *
 *  Copyright (C) 2004-2005 Jan Wrobel <wrobel@blues.ath.cx>
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
#include <stdio.h>
#include <mntent.h>
#include "vfs_mount.h"
#include "error.h"


void mount_from_fstab(const char *fstab_name)
{
	FILE *fstab;
	struct mntent *mp;
	
	if (!(fstab = setmntent(fstab_name, "r")))
		err_sys("setmntent %s", fstab_name);
	
	for(;(mp = getmntent(fstab));)
		add_entry_point(mp->mnt_fsname, mp->mnt_dir,
				mp->mnt_type, mp->mnt_opts);

	endmntent(fstab);
}
