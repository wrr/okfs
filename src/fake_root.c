/*$Id: fake_root.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/

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

/*
 * OKFS can fake root environment
 */

#include "entry.h"
#include "task.h"
#include "debug.h"

static void rootid(void)
{
	TRACE("faking uid 0 slave->pid = %d", slave->pid);
	set_slave_return(0);
}

void fake_root_init(void)
{
	call[__NR_getuid].call_after = rootid;
	call[__NR_getgid].call_after = rootid;
	call[__NR_geteuid].call_after = rootid;
	call[__NR_getegid].call_after = rootid;
	call[__NR_getuid32].call_after = rootid;
	call[__NR_getgid32].call_after = rootid;
	call[__NR_geteuid32].call_after = rootid;
	call[__NR_getegid32].call_after = rootid;
}

