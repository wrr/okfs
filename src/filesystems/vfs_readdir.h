#ifndef _READDIR_H
#define _READDIR_H

/*
  Dirent structure passed by okfs to filesystems.
  It is used to make filesystems' code independent from one of the dirent 
  structure defined in kernel.
*/
struct okfs_dirent{
	/*inode number of this file*/
	long ino;
	/*type of this file, can be DT_UNKNOWN*/
	long type;
	/*null terminated file name*/
	char *name;
	int name_len;
	struct okfs_dirent *next_dir;
};


void
okfs_add_dirent(struct okfs_dirent **head, const char *name, ino_t ino, 
		unsigned int d_type);

void okfs_getdents(void);
void okfs_getdents64(void);

#endif
