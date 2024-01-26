/*$Id: vfs_stat.h,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
#ifndef _VFS_STAT_H
#define _VFS_STAT_H

void okfs_fstat(void);
void okfs_fstat64(void);

void okfs_lstat(void);
void okfs_lstat64(void);

void okfs_stat(void);
void okfs_stat64(void);

#endif
