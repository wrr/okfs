/*$Id: shfsdefines.h,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
/*
 *  This file was originally part of SHell File System
 *  See http://shfs.sourceforge.net/ for more info.
 *  Copyright (C) 2002-2004  Miroslav Spousta <qiq@ucw.cz>
 *
 *  It is modified for Out of Kernel File System. 
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

#include <linux/types.h>

#ifndef _SHFSDEFINES_H
#define _SHFSDEFINES_H

#define BUFFER_MAX	1024
#define SHFS_PATH_MAX    512
#define SOCKBUF_SIZE		(SHFS_PATH_MAX * 10)
#define READLNBUF_SIZE		(SHFS_PATH_MAX * 10)

#define SHFS_FCACHE_MAX		10	/* max number of files cached */
#define SHFS_FCACHE_PAGES	32	/* should be 2^x */

//#define PROTO_VERSION 2

/* response code */
#define REP_PRELIM	100
#define REP_COMPLETE	200
#define REP_NOP 	201
#define REP_NOTEMPTY	202		/* file with zero size but not empty */
#define REP_CONTINUE	300
#define REP_TRANSIENT	400
#define REP_ERROR	500
#define REP_EPERM	501
#define REP_ENOSPC	502
#define REP_ENOENT	503

#define SHFS_SUPER_MAGIC 0xD0D0


struct shfs_data{
	char *root;	/* remote root dir */
	char *user;	/* remote host user */
	char *host;	/* remote hostname */
	char *host_long;	/* full remote hostname (for mtab entry) */
	char *port; 	/* remote port */
	char *cmd_template; 	/* command to execute (as specified) */
	char *cmd;	/* command to execute */

	/* user (& group) under the cmd is executed */
	uid_t cmd_uid;
	gid_t cmd_gid;
	int have_uid;
	
	int nomtab;	/* do not update /etc/mtab */
	//int preserve;	/* preserve owner of files not supported by okfs*/
	int stable;	/* stable symlinks */
	char *options;  /* options passed to shfs */
	/* should be connection recovered after program exit? */
	int persistent;
	
	char *type;	/* preferred type of connection */
	int verbose;	/* should shfsmount print debug messages? */
	int debug_level; /*not yet supported*/

	int sock;	/*okfs<--pipe-->ssh*/

	long next_inode;
	/************************************************/

	uid_t uid;
	gid_t gid;
	__kernel_mode_t root_mode;
	__kernel_mode_t fmask;
	//char mount_point[SHFS_PATH_MAX];

	char sockbuf[SOCKBUF_SIZE];
	char readlnbuf[READLNBUF_SIZE];
	int readlnbuf_len;
	int fcache_free;
	int fcache_size; 
	int garbage_read;
	int garbage_write;
	int garbage;
	int readonly;
	//int preserve_own:1;
	//int stable_symlinks:1;
};



int init_sh(int fd, const char *desired, const char *root, int stable);

#endif
