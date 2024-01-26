/*$Id*/
/*
 * proc.c
 *
 * Miscellaneous routines, socket read/write.
 */
 
#include <linux/errno.h>
#include "shfsdefines.h"
#include "okfs_unistd.h"
#include "okfs_lib.h"
#include "libc.h"
#include "debug.h"
#include "wrap.h"
#include "proc.h"

static int clear_garbage(struct shfs_data *);

int sock_write(struct shfs_data *info, const void *buffer, int count)
{
	int fd = info->sock;
	int c, result = 0;

	if (fd < 0)
		return -EIO;
	if (info->garbage) {
		result = clear_garbage(info);
		if (result < 0)
			return result;
	}
	
	c = count;
	
	do {
		if ((result = write(fd, buffer, c)) < 0) {
			TRACE("error: %d", result);
			if (result == -EAGAIN)
				continue;
			info->sock = -1;
			break;
		}
		buffer += result;
		c -= result;
	} while (c > 0);

	TRACE(">%d", result);
	if (result < 0)
		set_garbage(info, 1, c);
	else
		result = count;
	return result;
}

#define BUFFER info->readlnbuf
#define LEN    info->readlnbuf_len

int sock_read(struct shfs_data *info, void *buffer, int count)
{
	int fd = info->sock;
	int c, result = 0;
	
	if (fd < 0)
		return -EIO;
	if (info->garbage) {
		result = clear_garbage(info);
		if (result < 0)
			return result;
	}
	c = count;
	if (LEN > 0) {
		if (count > LEN)
			c = LEN;
		okfs_memcpy(buffer, BUFFER, c);
		buffer += c;
		LEN -= c;
		if (LEN > 0)
			okfs_memmove(BUFFER, BUFFER+c, LEN);
		c = count - c;
	}

	/* don't block reading 0 bytes */
	if (c == 0)
		return count;
	
	do {
		
		if (!(result = read(fd, buffer, c))){
			/*  peer has closed socket */
			result = -EIO;
		}
		if (result < 0) {
			TRACE("error: %d", result);
			if (result == -EAGAIN)
				continue;
			info->sock = -1;
			break;
		}
		buffer += result;
		c -= result;
	} while (c > 0);

	TRACE("<%d", result);
	if (result < 0)
		set_garbage(info, 0, c);
	else
		result = count;
	return result;
}

int sock_readln(struct shfs_data *info, char *buffer, int count)
{
	int fd = info->sock;
	int c, l = 0, result;
	char *nl;

	if (fd < 0)
		return -EIO;
	if (info->garbage) {
		result = clear_garbage(info);
		if (result < 0)
			return result;
	}
	while (1) {
		nl = memchr(BUFFER, '\n', LEN);
		if (nl) {
			*nl = '\0';
			strncpy(buffer, BUFFER, count-1);
			buffer[count-1] = '\0';
			c = LEN-(nl-BUFFER+1);
			if (c > 0)
				memmove(BUFFER, nl+1, c);
			LEN = c;
			
			TRACE("<%s", buffer);
			return strlen(buffer);
		}
		TRACE("miss(%d)", LEN);
		c = READLNBUF_SIZE - LEN;
		if (c == 0) {
			BUFFER[READLNBUF_SIZE-1] = '\n';
			continue;
		}

		if (!(result = read(fd, BUFFER + LEN, c))) {
			/* peer has closed socket */
			result = -EIO;
		}
		if (result < 0) {
			TRACE("error: %d", result);
			if (result == -EAGAIN)
				continue;
			info->sock = -1;
			set_garbage(info, 0, c);
			return result;
		}
		LEN += result;

		/* do not lock while reading 0 bytes */
		if (l++ > READLNBUF_SIZE && LEN == 0)
			return -EINVAL;
	}
}

int reply(char *s)
{
	if (strncmp(s, "### ", 4))
		return 0;
	return strtoul(s+4, NULL, 10);
}


void set_garbage(struct shfs_data *info, int write, int count)
{
	info->garbage = 1;
	if (write)
		info->garbage_write = count;
	else
		info->garbage_read = count;
}


static int clear_garbage(struct shfs_data *info)
{
	static unsigned long seq = 12345;
	char buffer[256];
	int i, c, state, garbage;
	int result;

	garbage = info->garbage_write;
	if (garbage)
		memset(buffer, ' ', sizeof(buffer));
	TRACE(">%d", garbage);
	while (garbage > 0) {
		c = garbage < sizeof(buffer) ? garbage : sizeof(buffer);
		info->garbage = 0;
		result = sock_write(info, buffer, c);
		if (result < 0) {
			info->garbage_write = garbage;
			goto error;
		}
		garbage -= result;
	}
	info->garbage_write = 0;
	garbage = info->garbage_read;
	TRACE("<%d", garbage);
	while (garbage > 0) {
		c = garbage < sizeof(buffer) ? garbage : sizeof(buffer);
		info->garbage = 0;
		result = sock_read(info, buffer, c);
		if (result < 0) {
			info->garbage_read = garbage;
			goto error;
		}
		garbage -= result;
	}
	info->garbage_read = 0;
		
	info->garbage = 0;
	sprintf(buffer, "\n\ns_ping %lu", seq);
	result = sock_write(info, (void *)buffer, strlen(buffer));
	if (result < 0)
		goto error;
	TRACE("reading..");
	state = 0;
	for (i = 0; i < 100000; i++) {
		info->garbage = 0;
		result = sock_readln(info, buffer, sizeof(buffer));
		if (result < 0)
			goto error;
		if (((state == 0) || (state == 1)) && reply(buffer) == REP_PRELIM) {
			state = 1;
		} else if (state == 1 && strtoul(buffer, NULL, 10) == (seq-1)) {
			state = 2;
		} else if (state == 2 && reply(buffer) == REP_NOP) {
			TRACE("cleared");
			return 0;
		} else {
			state = 0;
		}
	}
error:
	info->garbage = 1;
	TRACE("failed");
	return result;
}

