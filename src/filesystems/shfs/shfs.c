/*$Id: shfs.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/	
/*
 *  This file was originally part of SHell File System
 *  See http://shfs.sourceforge.net/ for more info.
 *  Copyright (C) 2002-2004  Miroslav Spousta <qiq@ucw.cz>
 *
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
 
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/dirent.h>
#include <linux/kdev_t.h>
#include <linux/time.h>
#include <linux/errno.h>
#include <linux/dcache.h>
#include "virtualfs.h"
#include "vfs_readdir.h"
#include "debug.h"
#include "wrap.h"
#include "okfs_unistd.h"
#include "libc.h"
#include "shfs.h"
#include "proc.h"

/* directory list fields (ls -lan) */
#define DIR_COLS   9
#define DIR_PERM   0
#define DIR_NLINK  1
#define DIR_UID    2
#define DIR_GID    3
#define DIR_SIZE   4
#define DIR_MONTH  5
#define DIR_DAY    6
#define DIR_YEAR   7
#define DIR_NAME   8


/* aaa'aaa -> aaa'\''aaa */
static int convert_path(const char *src, char *dst)
{
	char *s;
	char *r = dst;
	int q = 0;
	if (strlen(src) > SHFS_PATH_MAX - 1)
		return -ENAMETOOLONG;

	strcpy(dst, src);
	
	while (*r) {
		if (*r == '\'')
			q++;
		r++;
	}
	s = r+(3*q);
	if ((s-dst+1) > SHFS_PATH_MAX)
		return 0;
	
	*(s--) = '\0';
	r--;
	while (q) {
		if (*r == '\'') {
			s -= 3;
			s[0] = '\'';
			s[1] = '\\';
			s[2] = '\'';
			s[3] = '\'';
			q--;
		} else {
			*s = *r;
		}
		s--;
		r--;
	}
	return 1;
}

static int do_command(struct shfs_data *info, char *cmd, char *args, ...)
{
	va_list ap;
	int result;
	char *s;

	
	s = info->sockbuf;
	strcpy(s, cmd); s += strlen(cmd);
	if (SOCKBUF_SIZE - strlen(cmd) < 2){
		result = -ENAMETOOLONG;
		goto error;
	}
	strcpy(s, " "); s++;

	va_start(ap, args);
	result = vsnprintf(s, SOCKBUF_SIZE - (s - info->sockbuf), args, ap);
	va_end(ap);
	if (result < 0 || strlen(info->sockbuf) + 2 > SOCKBUF_SIZE) {
		result = -ENAMETOOLONG;
		goto error;
	}
	s += result;
	strcpy(s, "\n");
	
	TRACE("#%s", info->sockbuf);
	result = sock_write(info, info->sockbuf, strlen(info->sockbuf));
	if (result < 0)
		goto error;
	sock_readln(info, info->sockbuf, SOCKBUF_SIZE);
	switch (reply(info->sockbuf)) {
	case REP_COMPLETE:
		result = 0;
		break;
	case REP_EPERM:
		result = -EPERM;
		break;
	case REP_ENOENT:
		result = -ENOENT;
		break;
	case REP_NOTEMPTY:
		result = 1;
		break;
	default:
		result = -EIO;
		break;
	}

error:
	return result;
}


/*as for now mode is ignored*/
int shfs_mkdir(struct okfs_superblock *fsi, const char *path, mode_t mode)
{
	struct shfs_data *info = (struct shfs_data *) fsi->fsdata;
	char newpath[SHFS_PATH_MAX];

	if (!convert_path(path, newpath))
		return -ENAMETOOLONG;
	
	TRACE("mkdir %s", newpath);
	return do_command(info, "s_mkdir", "'%s'", newpath);
}

int shfs_rmdir(struct okfs_superblock *fsi,  const char *path)
{
	struct shfs_data *info = (struct shfs_data *) fsi->fsdata;
	char newpath[SHFS_PATH_MAX];

	if (!convert_path(path, newpath))
		return -ENAMETOOLONG;	

	TRACE("rmdir %s", newpath);
	return do_command(info, "s_rmdir", "'%s'", newpath);
}

int shfs_rename(struct okfs_superblock *fsi, const char *path1, const char *path2)
{
	struct shfs_data *info = (struct shfs_data *) fsi->fsdata;
	char newpath2[SHFS_PATH_MAX], newpath1[SHFS_PATH_MAX];;

	if (!convert_path(path1, newpath1) || !convert_path(path2, newpath2))
		return -ENAMETOOLONG;

	TRACE("rename %s -> %s", newpath1, newpath2);
	
	return do_command(info, "s_mv", "'%s' '%s'", newpath1, newpath2);
}

/* returns NULL if not enough space */
static inline char *get_ugid(struct shfs_data *info, char *str, int max)
{
	/*char buffer[1024];*/
	char *s = str;
	
	if (max < 2)
		return NULL;
	strcpy(s, " "); s++;
	/*	
	if (info->preserve_own) {
	...
	not supported
	...
	*/
	return s;
}

static int parse_dir(char *s, char **col)
{
	int c = 0;
	int is_space = 1;
	int device = 0;

	while (*s) {
		if (c == DIR_COLS)
			break;
		
		if (*s == ' ') {
			if (!is_space) {
				if ((c-1 == DIR_SIZE) && device) {
					while (*s && (*s == ' '))
						s++;
					while (*s && (*s != ' '))
						s++;
				}
				*s = '\0';
				is_space = 1;
			}
		} else {
			if (is_space) {
				/* (b)lock/(c)haracter device hack */
				col[c++] = s;
				is_space = 0;
				if ((c-1 == DIR_PERM) && ((*s == 'b')||(*s == 'c'))) {
					device = 1;
				}

			}
		}
		s++;
	}
	return c;
}

static unsigned int get_this_year(void)
{
	unsigned long s_per_year = 365*24*3600;
	unsigned long delta_s = 24*3600;

	unsigned int year = time(NULL) / (s_per_year + delta_s/4);
	return 1970 + year; 
}

static unsigned int get_this_month(void) 
{
	int month = -1;
	long secs = time(NULL) % (365*24*3600+24*3600/4);
	static long days_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (get_this_year() % 4) {
		days_month[1] = 28;
	} else {
		days_month[1] = 29;
	}
	while (month < 11 && secs >= 0) {
		month++;
		secs-=24*3600*days_month[month];
	}
	return (unsigned int)month;
}

static int get_month(char *mon)
{
	static char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	unsigned int i;

	for (i = 0; i<12; i++)
		if (strcmp(mon, months[i]) == 0) {
			TRACE("%s: %u", mon, i);
			return i;
		}

	return -1;
}

/*full path = mntpoint + dir(from mount point) + "/" + filename*/
static inline void add_to_cache(struct okfs_superblock *fsp, const char *dir, 
				const char *file, struct stat *statbuf)
{
	int len = fsp->mnt_point_len;
	char *full;
	struct okfs_inode *inop;
	
	if (dir)
		len += strlen(dir);

	if (*file){
		len += strlen(file);
		len += 1;/*'/'*/
	}
	
	full = (char *)Malloc(len + 1);
	strcpy(full, fsp->mnt_point);
	if (dir)
		strcat(full, dir);

	if (*file){
		strcat(full, "/");
		strcat(full, file);
	}

	if (!(inop = cache_find_inode(fsp, full, len))){
		statbuf->st_ino = 
			((struct shfs_data *)fsp->fsdata)->next_inode++;
		cache_add_inode(fsp, full, len, statbuf, NULL);
	}
	else
		statbuf->st_ino = inop->statbuf.st_ino;
	free(full);
}

static int
do_ls(struct okfs_superblock *fsp, const char *path, struct stat *statp,
      struct okfs_dirent **okfs_dir)
{
	struct shfs_data *info = 
		(struct shfs_data *) fsp->fsdata;
	char *col[DIR_COLS], file[SHFS_PATH_MAX];
	struct stat statbuf;
	struct qstr name;
	unsigned int year, mon, day, hour, min;
	unsigned int this_year = get_this_year();
	unsigned int this_month = get_this_month();
	int device, month;
	umode_t mode;
	char *b, *s, *command = statp ? "s_stat" : "s_lsdir";
	int result;

	if (!convert_path(path, file))
		return -ENAMETOOLONG;
	
	s = info->sockbuf;
	strcpy(s, command); s += strlen(command);
	s = get_ugid(info, s, SOCKBUF_SIZE - strlen(command));
	if (!s) {
		result = -ENAMETOOLONG;
		goto out;
	}
	if (s - info->sockbuf + strlen(file) + 4 > SOCKBUF_SIZE) {
		result = -ENAMETOOLONG;
		goto out;
	}
	strcpy(s, "'"); s++;
	strcpy(s, file); s += strlen(file);
	strcpy(s, "'"); s++;
	strcpy(s, "\n");

	TRACE("%s", info->sockbuf);
	result = sock_write(info, (void *)info->sockbuf, strlen(info->sockbuf));
	if (result < 0)
		goto out;

	while ((result = sock_readln(info, info->sockbuf, SOCKBUF_SIZE)) > 0) {
		switch (reply(info->sockbuf)) {
		case REP_COMPLETE:
			result = 0;
			goto out;
		case REP_EPERM:
			result = -EPERM;
			goto out;
		case REP_ENOENT:
			result = -ENOENT;
			goto out;
		case REP_ERROR:
			result = -EIO;
			goto out;
		}

		result = parse_dir(info->sockbuf, col);
		if (result != DIR_COLS)
			continue;		/* skip `total xx' line */
		
		memset(&statbuf, 0, sizeof(statbuf));
		s = col[DIR_NAME];
		if (!strcmp(s, ".") || !strcmp(s, ".."))
			continue;
		name.name = s;
		/* name.len is assigned later */

		s = col[DIR_PERM];
		mode = 0; device = 0;
		switch (s[0]) {
		case 'b':
			device = 1;
			if ((info->fmask & S_IFMT) & S_IFBLK)
				mode = S_IFBLK;
			else
				mode = S_IFREG;
			break;
		case 'c':
			device = 1;
			if ((info->fmask & S_IFMT) & S_IFCHR)
				mode = S_IFCHR;
			else
				mode = S_IFREG;
		break;
		case 's':
		case 'S':			/* IRIX64 socket */
			mode = S_IFSOCK;
			break;
		case 'd':
			mode = S_IFDIR;
			break;
		case 'l':
			mode = S_IFLNK;
			break;
		case '-':
			mode = S_IFREG;
			break;
		case 'p':
			mode = S_IFIFO;
			break;
		}
		if (s[1] == 'r') mode |= S_IRUSR;
		if (s[2] == 'w') mode |= S_IWUSR;
		if (s[3] == 'x') mode |= S_IXUSR;
		if (s[3] == 's') mode |= S_IXUSR | S_ISUID;
		if (s[3] == 'S') mode |= S_ISUID;
		if (s[4] == 'r') mode |= S_IRGRP;
		if (s[5] == 'w') mode |= S_IWGRP;
		if (s[6] == 'x') mode |= S_IXGRP;
		if (s[6] == 's') mode |= S_IXGRP | S_ISGID;
		if (s[6] == 'S') mode |= S_ISGID;
		if (s[7] == 'r') mode |= S_IROTH;
		if (s[8] == 'w') mode |= S_IWOTH;
		if (s[9] == 'x') mode |= S_IXOTH;
		if (s[9] == 't') mode |= S_ISVTX | S_IXOTH;
		if (s[9] == 'T') mode |= S_ISVTX;
		statbuf.st_mode = S_ISREG(mode) ? mode & info->fmask : mode;
		
		statbuf.st_uid = strtoul(col[DIR_UID], NULL, 10);
		statbuf.st_gid = strtoul(col[DIR_GID], NULL, 10);
		
		if (!device) {
			statbuf.st_size = strtoull(col[DIR_SIZE],
							 NULL, 10);
		} else {
			unsigned short major, minor;
			statbuf.st_size = 0;
			major = (unsigned short) strtoul(col[DIR_SIZE],
								&s, 10);
			while (*s && (!isdigit(*s)))
				s++;
			minor = (unsigned short) strtoul(s, NULL, 10);
			statbuf.st_rdev = MKDEV(major, minor);
		}
		statbuf.st_dev = 73;/*not too corect*/
		statbuf.st_nlink = strtoul(col[DIR_NLINK], NULL, 10);
		statbuf.st_blksize = 4096;
		statbuf.st_blocks = (statbuf.st_size + 511) >> 9;
		
		month = get_month(col[DIR_MONTH]);
		/* some systems have month/day swapped (MacOS X) */
		if (month < 0) {
			day = strtoul(col[DIR_MONTH], NULL, 10);
			mon = get_month(col[DIR_DAY]);
		} else {
			mon = (unsigned) month;
			day = strtoul(col[DIR_DAY], NULL, 10);
		}
		
		s = col[DIR_YEAR];
		if (!strchr(s, ':')) {
			year = strtoul(s, NULL, 10);
			hour = 12;
			min = 0;
		} else {
			year = this_year;
			if (mon > this_month) 
				year--;
			b = strchr(s, ':');
			*b = 0;
			hour = strtoul(s, NULL, 10);
			min = strtoul(++b, NULL, 10);
		}
		statbuf.st_atime = statbuf.st_mtime = statbuf.st_ctime = 
			mktime(year, mon + 1, day, hour, min, 0);
		
		/*statbuf.st_atime.tv_nsec = statbuf.st_mtime.tv_nsec = 
		  statbuf.st_ctime.tv_nsec = 0;*/

		if (S_ISLNK(mode) && ((s = strstr(name.name, " -> "))))
			*s = '\0';
		name.len = strlen(name.name);
		TRACE("Name: %s, mode: %o, size: %lu, nlink: %d, month: %d, day: %d, year: %d, hour: %d, min: %d (time: %lu)", name.name, statbuf.st_mode, statbuf.st_size, statbuf.st_nlink, mon, day, year, hour, min, statbuf.st_atime);

		if (statp){
			if (*path == '/')
				add_to_cache(fsp, NULL, path + 1, &statbuf);
			else
				add_to_cache(fsp, NULL, path, &statbuf);
			*statp = statbuf;
		}
		else{
			/*TODO
			  here DT_UNKNOWN can be changed to something else
			  because we have enough informations about this entry
			  (in stat buffer). 
			*/
			add_to_cache(fsp, path, name.name, &statbuf);
			okfs_add_dirent(okfs_dir, name.name, statbuf.st_ino, 
				       DT_UNKNOWN);				}
	}
out:
	return result;
}

int shfs_readdir(struct okfs_file *filep, int fd, struct okfs_dirent **dirp)
{
	ino_t curi, pari;
	curi = get_ino(filep->fsp, filep->path);
	pari = get_parent_ino(filep->fsp, filep->path);
	okfs_add_dirent(dirp, "." , curi, DT_DIR);
	okfs_add_dirent(dirp, ".." , pari, DT_DIR);
	
	return do_ls(filep->fsp, filep->rel_path, NULL, dirp);
	
}

int shfs_lstat(struct okfs_superblock *fsi, const char *path, struct stat *buf)
{
	TRACE("path %s", path);
	return do_ls(fsi, path, buf, NULL);
}


/* returns 1 if file is "non-empty" but has zero size */
int 
shfs_open(struct okfs_superblock *fsi, const char *path, int flags, mode_t mode)
{
	char bufmode[3] = "";
	struct shfs_data *info = 
		(struct shfs_data *) fsi->fsdata;
	char file[SHFS_PATH_MAX];

	if (!convert_path(path, file))
		return -ENAMETOOLONG;	
	
	if (flags & O_CREAT){
		int ret = do_command(info, "s_creat", "'%s' %o", file, 
				     mode & S_IALLUGO);
		if (ret < 0)
			return ret;
	}
		
	if ((flags & O_RDONLY) == O_RDONLY)
		strcpy(bufmode, "R");
	else if ((flags & O_WRONLY) == O_WRONLY)
		strcpy(bufmode, "W");
	else{
		TRACE("%08x", flags);
		strcpy(bufmode, "RW");
	}

	TRACE("Open: %s (%s)", file, bufmode);
	return do_command(info, "s_open", "'%s' %s", file, bufmode);
}



/* data should be aligned (offset % count == 0), ino == 0 => normal read, != 0 => slow read */
int shfs_read(struct okfs_file *fdp, int fd, void *buffer, size_t count)
{
	struct shfs_data *info = 
		(struct shfs_data *) fdp->fsp->fsdata;
	unsigned offset = fdp->f_pos;
	unsigned long ino = 0;
	unsigned bs = 1, count2, offset2 = offset;
	int result;
	char *s;
	char file[SHFS_PATH_MAX];
	/****/
	TRACE("trying to read = %d", count);
	ASSERT(fdp->ino->statbuf.st_size >= offset);
	if (fdp->ino->statbuf.st_size - offset < count)
		count = fdp->ino->statbuf.st_size - offset;
	/****/
	count2 = count;

	if (!convert_path(fdp->rel_path, file))
		return -ENAMETOOLONG;		

	/* read speedup if possible */
	if (count && !(offset % count)) {
		bs = count;
		offset2 = offset / count;
		count2 = 1;
	}

	
	s = info->sockbuf;
	if (ino){
		strcpy(s, "s_sread"); 
		s += strlen("s_sread");
	}else{
		strcpy(s, "s_read"); 
		s += strlen("s_read");
	}
	
	s = get_ugid(info, s, SOCKBUF_SIZE - strlen(info->sockbuf));
	if (!s) {
		result = -ENAMETOOLONG;
		goto error;
	}
	if (ino) {
		result = snprintf(s, SOCKBUF_SIZE - (s - info->sockbuf), 
				  "'%s' %u %u %u %u %u %lu\n", file, offset, 
				  count, bs, offset2, count2, ino);
	} else {
		result = snprintf(s, SOCKBUF_SIZE - (s - info->sockbuf), 
				  "'%s' %u %u %u %u %u\n", file, offset,
				  count, bs, offset2, count2);
	}
	if (result < 0) {
		result = -ENAMETOOLONG;
		goto error;
	}

	TRACE("<%s", info->sockbuf);
	result = sock_write(info, (void *)info->sockbuf, strlen(info->sockbuf));
	if (result < 0)
		goto error;

	result = sock_readln(info, info->sockbuf, SOCKBUF_SIZE);
	if (result < 0) {
		set_garbage(info, 0, count);
		goto error;
	}
	switch (reply(info->sockbuf)) {
	case REP_PRELIM:
		break;
	case REP_EPERM:
		result = -EPERM;
		goto error;
	case REP_ENOENT:
		result = -ENOENT;
		goto error;
	default:
		set_garbage(info, 0, count);
		result = -EIO;
		goto error;
	}
	if (ino) {
		result = sock_readln(info, info->sockbuf, SOCKBUF_SIZE);
		if (result < 0)
			goto error;
		count = strtoul(info->sockbuf, NULL, 10);
	}

	result = sock_read(info, buffer, count);
	TRACE("sock_read = %d", result);
	if (result < 0)
		goto error;
	else
		ASSERT(result == count);
	result = sock_readln(info, info->sockbuf, SOCKBUF_SIZE);
	if (result < 0)
		goto error;
	switch (reply(info->sockbuf)) {
	case REP_COMPLETE:
		break;
	case REP_EPERM:
		result = -EPERM;
		goto error;
	case REP_ENOENT:
		result = -ENOENT;
		goto error;
	default:
		result = -EIO;
		goto error;
	}

	result = count;
 error:

	TRACE("READED %d", result);
	return result;
}

int shfs_write(struct okfs_file *fdp, int fd, void *buffer, size_t count)
{
	struct shfs_data *info = 
		(struct shfs_data *) fdp->fsp->fsdata;
	unsigned offset = fdp->f_pos;
  	unsigned offset2 = offset, bs = 1;
	char file[SHFS_PATH_MAX];
	int result;
	unsigned long ino = fdp->ino->statbuf.st_ino;
	char *s;

	if (!convert_path(fdp->rel_path, file))
		return -ENAMETOOLONG;	
	
	if (!count)
		return 0;
	
	TRACE(">%s[%u, %u]", file, (unsigned)offset, (unsigned)count);
	if (offset % info->fcache_size == 0) {
		offset2 = offset / info->fcache_size;
		bs = info->fcache_size;
	}
		
	s = info->sockbuf;
	strcpy(s, "s_write"); s += strlen("s_write");
	s = get_ugid(info, s, SOCKBUF_SIZE - strlen(info->sockbuf));
	if (!s) {
		result = -ENAMETOOLONG;
		goto error;
	}

	result = snprintf(s, SOCKBUF_SIZE - (s - info->sockbuf), 
			  "'%s' %u %u %u %u %lu\n", file, offset, count, 
			  bs, offset2, ino);
	
	if (result < 0) {
		result = -ENAMETOOLONG;
		goto error;
	}

	TRACE(">%s", info->sockbuf);
	result = sock_write(info, (void *)info->sockbuf, strlen(info->sockbuf));
	if (result < 0)
		goto error;
	result = sock_readln(info, info->sockbuf, SOCKBUF_SIZE);
	if (result < 0) {
		set_garbage(info, 1, count);
		goto error;
	}
	switch (reply(info->sockbuf)) {
	case REP_PRELIM:
		break;
	case REP_EPERM:
		result = -EPERM;
		goto error;
	case REP_ENOENT:
		result = -ENOENT;
		goto error;
	default:
		result = -EIO;
		goto error;
	}

	result = sock_write(info, buffer, count);
	if (result < 0)
		goto error;
	result = sock_readln(info, info->sockbuf, SOCKBUF_SIZE);
	if (result < 0)
		goto error;
	switch (reply(info->sockbuf)) {
	case REP_COMPLETE:
		break;
	case REP_EPERM:
		result = -EPERM;
		goto error;
	case REP_ENOENT:
		result = -ENOENT;
		goto error;
	case REP_ENOSPC:
		result = -ENOSPC;
		set_garbage(info, 1, count);
		goto error;
	default:
		result = -EIO;
		goto error;
	}

	result = count;
error:
	TRACE(">%d", result);
	return result;
}


int 
shfs_readlink(struct okfs_superblock *fsi, const char *path, char *buf, size_t bufsize)
{
	char *s;
	int result = 0;
	int len;
	struct shfs_data *info = (struct shfs_data *) fsi->fsdata;
	char name[SHFS_PATH_MAX];

	if (!convert_path(path, name))
		return -ENAMETOOLONG;

	s = info->sockbuf;
	strcpy(s, "s_readlink"); s += strlen("s_readlink");
	s = get_ugid(info, s, SOCKBUF_SIZE - strlen(info->sockbuf));
	if (!s) {
		result = -ENAMETOOLONG;
		goto error;
	}
	if (s - info->sockbuf + strlen(name) + 4 > SOCKBUF_SIZE) {
		result = -ENAMETOOLONG;
		goto error;
	}
	strcpy(s, "'"); s++;
	strcpy(s, name); s += strlen(name);
	strcpy(s, "'"); s++;
	strcpy(s, "\n");

	TRACE("Readlink %s", info->sockbuf);
	result = sock_write(info, (void *)info->sockbuf, 
			    strlen(info->sockbuf));
	
	if (result < 0)
		goto error;
	len = result = sock_readln(info, info->sockbuf, SOCKBUF_SIZE);
	if (result < 0)
		goto error;

	switch (reply(info->sockbuf)) {
	case REP_COMPLETE:
		result = -EIO;
		goto error;
	case REP_EPERM:
		result = -EINVAL;
		goto error;
	}

	strncpy(buf, info->sockbuf, len);

	result = sock_readln(info, info->sockbuf, SOCKBUF_SIZE);
	if (result < 0)
		goto error;
	switch (reply(info->sockbuf)) {
	case REP_COMPLETE:
		result = 0;
		break;
	case REP_EPERM:
		result = -EINVAL;
		goto error;
	default:
		info->garbage = 1;
		result = -EIO;
		goto error;
	}
error:
	if (result == 0)
		result = len;

	return result;
}


int shfs_truncate(struct okfs_superblock *fsi, const char *path, off_t size)
{
	struct shfs_data *info = (struct shfs_data *) fsi->fsdata;
	unsigned seek = 1;
	char file[SHFS_PATH_MAX];

	if (!convert_path(path, file))
		return -ENAMETOOLONG;
	
	TRACE("Truncate %s %u", file, (unsigned)size);
	
	/* dd doesn't like bs=0 */
	if (size == 0) {
		seek = 0;
		size = 1;
	}
	return do_command(info, "s_trunc", "'%s' %u %u", file, (unsigned) size, seek);
}

int shfs_link(struct okfs_superblock *fsi, const char *path1, const char *path2)
{
	struct shfs_data *info = (struct shfs_data *) fsi->fsdata;
	char old[SHFS_PATH_MAX], new[SHFS_PATH_MAX];

	if (!convert_path(path1, old) || !convert_path(path2, new))
		return -ENAMETOOLONG;
	
	TRACE("Link %s -> %s", old, new);
	return do_command(info, "s_ln", "'%s' '%s'", old, new);
}

int shfs_symlink(struct okfs_superblock *fsi, const char *path1, const char *path2)
{
	struct shfs_data *info = (struct shfs_data *) fsi->fsdata;
	char old[SHFS_PATH_MAX], new[SHFS_PATH_MAX];
	
	if (!convert_path(path1, old) || !convert_path(path2, new))
		return -ENAMETOOLONG;
	
	TRACE("Symlink %s -> %s", old, new);
	return do_command(info, "s_sln", "'%s' '%s'", old, new);
}

int shfs_chmod(struct okfs_superblock *fsi, const char *path, mode_t mode)
{
	struct shfs_data *info = (struct shfs_data *) fsi->fsdata;
	char file[SHFS_PATH_MAX];

	if (!convert_path(path, file))
		return -ENAMETOOLONG;
	
	TRACE("Chmod %o %s", mode, file);
	return do_command(info, "s_chmod", "'%s' %o", file, mode);
}


int shfs_lchown(struct okfs_superblock *fsi, char *path, uid_t user, gid_t group)
{
	struct shfs_data *info = (struct shfs_data *) fsi->fsdata;
	char file[SHFS_PATH_MAX];
	int ret = 0;
	
	if (!convert_path(path, file))
		return -ENAMETOOLONG;

	TRACE("lchown %s user = %u group = %u", file, user, group);
	if (user >=0)
		ret = do_command(info, "s_chown", "'%s' %u", file, user);
	if (ret >= 0 && group >= 0)
		ret = do_command(info, "s_chgrp", "'%s' %u", file, group);
	return ret;
}

int shfs_unlink(struct okfs_superblock *fsi, const char *path)
{
	struct shfs_data *info = (struct shfs_data *) fsi->fsdata;
	char file[SHFS_PATH_MAX];

	if (!convert_path(path, file))
		return -ENAMETOOLONG;

	TRACE("Remove %s", file);
	return do_command(info, "s_rm", "'%s'", file);
}


/* this code is borrowed from dietlibc */

inline static int isleap(int year) {
  /* every fourth year is a leap year except for century years that are
   * not divisible by 400. */
/*  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)); */
  return (!(year%4) && ((year%100) || !(year%400)));
}

/* days per month -- nonleap! */
const short __spm[12] =
  { 0,
    (31),
    (31+28),
    (31+28+31),
    (31+28+31+30),
    (31+28+31+30+31),
    (31+28+31+30+31+30),
    (31+28+31+30+31+30+31),
    (31+28+31+30+31+30+31+31),
    (31+28+31+30+31+30+31+31+30),
    (31+28+31+30+31+30+31+31+30+31),
    (31+28+31+30+31+30+31+31+30+31+30),
  };

/* seconds per day */
#define SPD 24*60*60

static char *
strctime(time_t time, char *s)
{
	time_t i;
	int sec, min, hour;
	int day, month, year;
	time_t work = time % (SPD);
	
	sec = work % 60; work /= 60;
	min = work % 60; hour = work / 60;
	work = time / (SPD);

	for (i = 1970; ; i++) {
		time_t k = isleap(i) ? 366 : 365;
		if (work >= k)
			work -= k;
		else
			break;
	}
	year = i;

	day=1;
	if (isleap(i) && (work > 58)) {
		if (work == 59)
			day = 2; /* 29.2. */
		work -= 1;
	}

	for (i = 11; i && (__spm[i] > work); i--)
		;
	month = i;
 	day += work - __spm[i];

	sprintf(s, "%.4d%.2d%.2d%.2d%.2d.%.2d", year, month+1, day, hour, min, sec);

	return s;
}


int
shfs_utime(struct okfs_superblock *fsi, const char *path, const struct utimbuf *buf)
{
	struct shfs_data *info = (struct shfs_data *) fsi->fsdata;
	char str[20];
	char file[SHFS_PATH_MAX];
	time_t atime, mtime;
	int ret;

	if (!convert_path(path, file))
		return -ENAMETOOLONG;
	
	if (buf){
		atime = buf->actime;
		mtime = buf->modtime;
	}
	else
		atime = mtime = time(NULL);

	strctime(atime, str);	

	TRACE("Settime %s %s", str, file);
	if (atime == mtime)
		return do_command(info, "s_settime", "'%s' am %s", file, str);
	
	ret = do_command(info, "s_settime", "'%s' a %s", file, str);
	if (ret >= 0){
		strctime(mtime, str);		
		ret = do_command(info, "s_settime", "'%s' m %s", file, str);
	}
	return ret;
}

int shfs_statfs(struct okfs_superblock *fsi, const char *path, struct statfs *attr)
{
	struct shfs_data *info = (struct shfs_data *) fsi->fsdata;
	char *s, *p;
	int result = 0;

	attr->f_type = SHFS_SUPER_MAGIC;
	attr->f_bsize = 4096;
	attr->f_blocks = 0;
	attr->f_bfree = 0;
	attr->f_bavail = 0;
	attr->f_files = 1;
	attr->f_bavail = 1;
	attr->f_namelen = SHFS_PATH_MAX;
	
	s = info->sockbuf;
	strcpy(s, "s_statfs"); s += strlen("s_statfs");
	s = get_ugid(info, s, SOCKBUF_SIZE - strlen(info->sockbuf));
	if (!s) {
		result = -ENAMETOOLONG;
		goto error;
	}
	strcpy(s, "\n");

	TRACE("Statfs %s", info->sockbuf);
	result = sock_write(info, (void *)info->sockbuf, strlen(info->sockbuf));
	if (result < 0)
		goto error;
	result = sock_readln(info, info->sockbuf, SOCKBUF_SIZE);
	if (result < 0)
		goto error;

	s = info->sockbuf;
	if ((p = strsep(&s, " ")))
		attr->f_blocks = strtoull(p, NULL, 10);
	if ((p = strsep(&s, " ")))
		attr->f_bfree = attr->f_blocks - strtoull(p, NULL, 10);
	if ((p = strsep(&s, " ")))
		attr->f_bavail = strtoull(p, NULL, 10);

	result = sock_readln(info, info->sockbuf, SOCKBUF_SIZE);
	if (result < 0)
		goto error;
	switch (reply(info->sockbuf)) {
	case REP_COMPLETE:
		result = 0;
		break;
	case REP_EPERM:
		result = -EPERM;
		goto error;
	default:
		info->garbage = 1;
		result = -EIO;
		goto error;
	}
error:
	return result;
}

/*TODO*/
int
shell_finish(struct okfs_superblock *fsi)
{
	struct shfs_data *info = (struct shfs_data *) fsi->fsdata;
	
	TRACE("Finish");
	return do_command(info, "s_finish", "");
}
