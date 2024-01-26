/*$Id: shfsmount.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/

/*
 *  This file was originally part of SHell File System
 *  See http://shfs.sourceforge.net/ for more info.
 *  Copyright (C) 2002-2004  Miroslav Spousta <qiq@ucw.cz>
 *
 *  It is modified for Out of Kernel File System. 
 *  Copyright (C) 2004-2005 Jan Wrobel <wrobel@blues.ath.cx>
 *
 *  Partialy inspired by smbmnt.c
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
 
#include <linux/socket.h>
#include <linux/net.h>
#include <linux/fcntl.h>
#include "shfsdefines.h"
#include "debug.h"
#include "okfs_unistd.h"
#include "okfs_lib.h"
#include "virtualfs.h"
#include "wrap.h"
#include "shfs.h"
#include "libc.h"
#include "proc.h"

#define SHELL		"/bin/sh"

int update_mtab(char *mnt, char *dev, char *opts);

void free_shfs_data(struct shfs_data *dp)
{
	free(dp->cmd);
	free(dp->host);
	free(dp->host_long);
	free(dp->options);
	free(dp);
}

static uid_t get_userid(char *s, gid_t *gid)
{
	int p, g;
	if (id_from_pwd(s, &p, &g) < 0){
		err_msg("Unknown user: %s", s);
		return -1;
	}
	if (gid)
		*gid = g;
	
	return p;
}


static char * strnconcat(char *str, int n, ...)
{
	va_list args;
	char *s;

	va_start(args, n);
	while ((s = va_arg(args, char *)))
		strncat(str, s, n);
	va_end(args);

	return str;
}

static int parse_mode(char *mode)
{
	return ((mode[0]-'0')<<6) + ((mode[1]-'0')<<3) + (mode[2]-'0');
}

static int parse_options(struct shfs_data *dp)
{
	char *s, *r;
	long i, j;
	if (dp->options)
		return 0;
	while ((s = strsep(&dp->options, ","))) {
		/* check for root-only options */
		if ((getuid() != 0) && 
		    !(strncmp(s, "cmd-user=", 9)
		      && strncmp(s, "uid=", 4) && strncmp(s, "gid=", 4) 
		      && strcmp(s, "suid") && strcmp(s, "dev"))){
			err_msg("'%s' used by non-root user!", s);
			return -1;
		}
		
		if (!strncmp(s, "cmd-user=", 9)) {
			if ((dp->cmd_uid = get_userid(s+9, &dp->cmd_gid)) < 0)
				goto ugly_opts;
			dp->have_uid = 1;
		} else if (!strncmp(s, "cmd=", 4)) {
			dp->cmd_template = s+4;
		} else if (!strncmp(s, "port=", 5)) {
			if (strtol(s+5, &r, 10) == 0 || *r){
				err_msg("Invalid port: %s", s+5);
				return -1;
			}
			dp->port = s+5;
		} else if (!strncmp(s, "persistent", 10)) {
			dp->persistent = 1;
		} else if (!strncmp(s, "type=", 5)) {
			dp->type = s+5;
		} else if (!strncmp(s, "stable", 6)) {
			dp->stable = 1;
		} else if (!strncmp(s, "uid=", 4)) {
			uid_t uid = get_userid(s+4, NULL);
			if (uid < 0)
				return -1;
			dp->uid = i; 
		} else if (!strncmp(s, "gid=", 4)) {
			gid_t gid = get_gid(s+4);
			if (gid < 0)
				return -1;
			dp->gid = gid;
		} else if (strncmp(s, "ro", 2) == 0) {
			dp->readonly = 1;
		} else if (strncmp(s, "rw", 2) == 0) {
			dp->readonly = 0;
		} else if (strncmp(s, "suid", 4) == 0) {
			dp->fmask |= S_ISUID|S_ISGID;
		} else if (strncmp(s, "nosuid", 6) == 0) {
			dp->fmask &= ~(S_ISUID|S_ISGID);
		} else if (strncmp(s, "dev", 3) == 0) {
			dp->fmask |= S_IFCHR|S_IFBLK;
		} else if (strncmp(s, "nodev", 5) == 0) {
			dp->fmask &= ~(S_IFCHR|S_IFBLK);
		} else if (strncmp(s, "exec", 4) == 0) {
			dp->fmask |= S_IXUSR|S_IXGRP|S_IXOTH;
		} else if (strncmp(s, "noexec", 6) == 0) {
			dp->fmask &= ~(S_IXUSR|S_IXGRP|S_IXOTH);
		} else if (strncmp(s, "cachesize=", 10) == 0) {
			if (strlen(s + 10) > 5)
				goto ugly_opts;
			i = strtoul(s + 10, &r, 10);
			if (*r)
				goto ugly_opts;
			i--;
			for (j = 0; i; j++)
				i >>= 1;
			dp->fcache_size = (1 << j) * PAGE_SIZE;
		} else if (strncmp(s, "cachemax=", 9) == 0) {
			if (strlen(s + 9) > 5)
				goto ugly_opts;
			i = strtoul(s + 9, &r, 10);
			if (*r)
				goto ugly_opts;
			dp->fcache_free = i;
		} else if (strncmp(s, "debug=", 6) == 0) {
			if (strlen(s+6) > 5)
				goto ugly_opts;
			i = strtoul(s + 6, &r, 10);
			if (*r)
				goto ugly_opts;
			dp->verbose = 1;
			dp->debug_level = i;
			/*not supported: else if (strncmp(p, "ttl=", 4) == 0) {
			if (strlen(p+4) > 10)
				goto ugly_opts;
			i = simple_strtoul(s + 4, &r, 10);
			dp->ttl = i * 1000;
			*/
		} else if (strncmp(s, "rmode=", 6) == 0) {
			if (strlen(s+6) > 3)
				goto ugly_opts;
			dp->root_mode = S_IFDIR | (parse_mode(s+6) & (S_IRWXU | S_IRWXG | S_IRWXO));
		} else{
			err_msg("shfs unrecognized option: ", s);
			return -1;
		}
	}
	return 0;
 ugly_opts:
	err_msg("shfs invalid option: %s", s);
	return -1;
}

static int shfs_data_init(char *opts, struct shfs_data *dp)
{
	dp->options = Strdup(opts);
	/* fill-in default values */
	dp->root = NULL;
	dp->user = NULL;
	dp->host = NULL;
	dp->host_long = NULL;
	dp->port = NULL;
	dp->cmd_template = NULL;
	dp->cmd = NULL;
	dp->cmd_uid = 0;
	dp->cmd_gid = 0;
	dp->have_uid = 0;
	dp->nomtab = 1;
	dp->stable = 0;
	dp->persistent = 0;
	dp->type = NULL;
	dp->verbose = 1;
	dp->debug_level = 1;
	dp->next_inode = 1;
	dp->uid = getuid();
	dp->gid = getgid();
	dp->root_mode = (S_IRUSR | S_IWUSR | S_IXUSR | S_IFDIR);
	dp->fmask = 00177777;
	dp->readlnbuf_len = 0;
	//spin_lock_init(&dp->fcache_lock);
	dp->fcache_free = SHFS_FCACHE_MAX;
	dp->fcache_size = SHFS_FCACHE_PAGES * PAGE_SIZE;
	dp->garbage_read = 0;
	dp->garbage_write = 0;
	dp->garbage = 0;
	dp->readonly = 0;
	
	if (parse_options(dp) < 0){
		free(dp->options);
		return -1;
	}
	return 1;
}

static void parse_fs_spec(char *fs_spec, struct shfs_data *dp)
{
	char *c;
	/*get host*/
	dp->host = Strdup(fs_spec);
	dp->host_long = Strdup(dp->host);
	if ((c = strchr(dp->host, '@'))) {
		*c = '\0';
		dp->user = dp->host;
		dp->host = c + 1;
	}
	
	/*
	 * stuart at gnqs.org
	 * the colon is used as the separator
	 * between the hostname and the root dir
	 * this makes it work more like scp(1)
	 */	
	if ((c = strchr(dp->host, ':'))) {
		*c = '\0';
		dp->root = c + 1;
	} else if (*dp->host == '/') {
		dp->root = dp->host;
		dp->host = NULL;
	}
}

static int setup_command(struct shfs_data *dp)
{
	char *tmp, *s;
	dp->cmd = Malloc(BUFFER_MAX);
	if (dp->cmd_template) {
		tmp = dp->cmd_template;
		*dp->cmd = '\0';
		while ((s = strsep(&tmp, "%"))) {
			strnconcat(dp->cmd, BUFFER_MAX, s, NULL);
			if (tmp) {
				switch (*tmp) {
				case 'u':
					if (dp->user)
						strnconcat(dp->cmd,BUFFER_MAX, 
							   dp->user, NULL);
					tmp++;
					break;
				case 'h':
					if (dp->host)
						strnconcat(dp->cmd,BUFFER_MAX, 
							   dp->host, NULL);
					tmp++;
					break;
				case 'P':
					if (dp->port)
						strnconcat(dp->cmd,BUFFER_MAX, 
							   dp->port, NULL);
					tmp++;
					break;
				default:
					strnconcat(dp->cmd, BUFFER_MAX, 
						   "%", NULL);
					break;
				}
			}
		}
	} else {
		if (!dp->host){
			err_msg("Unknown remote host");
			return -1;
		}
		snprintf(dp->cmd, BUFFER_MAX, "exec ssh %s%s %s%s %s %s", 
			 dp->port ? "-p " : "", dp->port ? dp->port : "",
			 dp->user ? "-l " : "", dp->user ? dp->user : "",
			 dp->host, SHELL);
	}
	return 1;
}



static int
create_socket_sh(struct shfs_data *dp)
{
	pid_t child;
	int fd[2], null;
	char *execv[] = { "sh", "-c", NULL, NULL };

	/* ensure fd 0-2 are open */
	do{
		if ((null = open("/dev/null", 0, 0)) < 0)
			err_sys("open(/dev/null): %s", strerror(errno));
	}while (null < 3);
	close(null);
	
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == -1)
		err_sys("socketpair");
	if ((child = Fork()) == 0) {
		Dup2(fd[0], 0);/*STDIN_FILENO*/
		Dup2(fd[0], 1);/*STDOUT_FILENO*/
		close(fd[0]);
		close(fd[1]);

		null = open("/dev/tty", O_WRONLY, 0);
		if (null >= 0) {
			/* stderr is /dev/tty (because of autofs) */
			Dup2(null, 2);/*STDERR_FILENO*/
			close(null);
		}

		if (dp->have_uid) 
			Setgid(dp->cmd_gid);
		else 
			dp->cmd_uid = getuid();

		Setuid(dp->cmd_uid);
		
		execv[2] = dp->cmd;
		execvp("/bin/sh", execv);
		err_sys("exec: %s %s %s", execv[0], execv[1], execv[2]);
	}
	close(fd[0]);
	close(null);
	if (!init_sh(fd[1], dp->type, dp->root, dp->stable)) {
		close(fd[1]);
		return -1;
	}
	return fd[1];
}


int shfs_mount(struct okfs_superblock *mi)
{
	struct shfs_data *dp;

	dp = (struct shfs_data*)Malloc(sizeof(struct shfs_data));
	dp->options = Strdup(mi->mnt_opts);
	if (shfs_data_init(mi->mnt_opts, dp) < 0){
		free(dp);
		return -1;
	}
	
	parse_fs_spec(mi->fs_spec, dp);

	if (setup_command(dp) < 0)
		goto error;


	TRACE("cmd: %s", dp->cmd);
	TRACE("user: %s, host: %s, root: %s, mnt: %s, port: %s, cmd-user: %d",
	      dp->user, dp->host, dp->root, mi->mnt_point, 
	      dp->port, dp->cmd_uid);
	
	if ((dp->sock = create_socket_sh(dp)) < 0){
		err_ret("Cannot create connection");
		goto error;
	}
	
        if (!dp->nomtab && 
	    update_mtab(mi->mnt_point, dp->host_long ? dp->host_long : 
			"(none)", mi->mnt_opts) < 0)
		goto error;
	
	mi->fsdata = dp;
	return 1;
 error:
	free_shfs_data(dp);
	return -1;
}
