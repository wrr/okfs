#ifndef VFS_MOUNT_H
#define VFS_MOUNT_H
void mount_from_fstab(const char *fstab_name);
void add_entry_point(char *mnt_fsname, char *mnt_dir, 
		     char *mnt_type, char *mnt_opts);
#endif
