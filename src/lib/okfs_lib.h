#ifndef OKFS_LIB_H
#define OKFS_LIB_H
/*Useful functions that doesn't fit anywhere else.*/
#ifdef MAX
#undef MAX
#endif
#define MAX(x, y) ((x) > (y)) ? (x) : (y)

#ifdef MIN
#undef MIN
#endif
#define MIN(x, y) ((x) < (y)) ? (x) : (y)


/*
 *These functions can't be taken from glibc because there are defined 
 *in linux headers with different arguments than in glibc.
 */
void *okfs_memcpy(void *dest, const void *src, int count);
void *okfs_memmove(void *dest, const void *src, int count);
void *okfs_mempcpy(void *dest, const void *src, int count);

/****************************************************************/

int is_dir(const char *path);
int id_from_pwd(char *, int *, int *);
int get_gid(char *);

#endif
