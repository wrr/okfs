#ifndef _PROC_H
#define _PROC_H

#include "shfsdefines.h"

int sock_write(struct shfs_data *info, const void *buf, int count);
int sock_read(struct shfs_data *info, void *buffer, int count);
int sock_readln(struct shfs_data *info, char *buffer, int count);
int reply(char *s);
void set_garbage(struct shfs_data *info, int write, int count);

#endif	/* _PROC_H */
