/*$Id: vfs_namei.h,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
#ifndef VFS_NAMEI_H
#define VFS_NAMEI_H


char *canonical(const char*, const char*, int);
#define canonicalize(cwd, name) canonical(cwd, name, 1)
#define lcanonicalize(cwd, name) canonical(cwd, name, 0)

#endif
