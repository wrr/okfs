/*$Id: entry.c,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/

/*
 *  This file is part of Out of Kernel File System.
 *
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

#include "debug.h"
#include "okfs_signal.h"
#include "entry.h"
#include "control.h"
#include "wrap.h"
#include "virtualfs.h"
#include "vfs_open.h"
#include "vfs_readdir.h"
#include "vfs_stat.h"

/*Initiation of system call table*/

#define TODO NULL

struct call_entry *call; 

static void 
add_call(int nr, void (*before)(), void (*after)(), const char *name, 
	 int stack_args)
{
	ASSERT(nr < NR_syscalls);
        call[nr].call_before = before;
        call[nr].call_after = after;
        call[nr].name = Strdup(name);
        call[nr].has_stack_args = stack_args;
}


void init_call_table(void)
{
	int i;
	char buf[256];
	call = Calloc(NR_syscalls, sizeof(struct call_entry));
	
	add_call(__NR_exit, NULL, NULL, "exit", 0);
	add_call(__NR_fork, before_fork, after_fork, "fork", 0);
	add_call(__NR_read, okfs_fd_call, okfs_read, "read", 0);
	add_call(__NR_write, okfs_fd_call, okfs_write, "write", 0);
	add_call(__NR_open, okfs_before_open, NULL, "open", 0);
	add_call(__NR_close, okfs_op_on_fd, okfs_after_close, "close", 0);
	add_call(__NR_waitpid, before_wait, NULL, "waitpid", 0);
	add_call(__NR_creat, okfs_before_creat, NULL, "creat", 0);
	add_call(__NR_link, okfs_before_lnk_rnm, okfs_link, "link", 0);
	add_call(__NR_unlink, dummy_call, okfs_unlink, "unlink", 0);
	add_call(__NR_execve, before_execve, after_execve, "execve", 0);
	add_call(__NR_chdir, dummy_call, okfs_chdir, "chdir", 0);
	add_call(__NR_time, NULL, NULL, "time", 0);
	add_call(__NR_mknod, dummy_call, okfs_mknod, "mknod",  0);
	add_call(__NR_chmod, dummy_call, okfs_chmod, "chmod", 0);
	add_call(__NR_lchown, dummy_call, okfs_lchown, "lchown", 0);
	add_call(__NR_break, unimplemented_call, NULL, "break", 0);
#ifdef __NR_oldstat
	add_call(__NR_oldstat, unimplemented_call, NULL, "oldstat", 0);
#endif
	add_call(__NR_lseek, okfs_fd_call, okfs_lseek, "lseek", 0);
	add_call(__NR_getpid, NULL, NULL, "getpid", 0);
	add_call(__NR_mount, NULL, NULL, "mount", 0);
	add_call(__NR_umount, NULL, NULL, "umount", 0);
	add_call(__NR_setuid, NULL, NULL, "setuid", 0);
	add_call(__NR_getuid, NULL, NULL, "getuid", 0);
#ifdef __NR_stime
	add_call(__NR_stime, NULL, NULL, "stime", 0);
#endif
	add_call(__NR_ptrace, unimplemented_call, NULL, "ptrace", 0);
	add_call(__NR_alarm, NULL, NULL, "alarm", 0);
#ifdef __NR_oldfstat
	add_call(__NR_oldfstat, unimplemented_call, NULL, "oldfstat", 0);
#endif
	add_call(__NR_pause, NULL, NULL, "pause", 0);
	add_call(__NR_utime, dummy_call, okfs_utime, "utime", 0);
#ifdef __NR_stty
	add_call(__NR_stty, NULL, NULL, "stty", 0);
#endif
#ifdef __NR_gtty
	add_call(__NR_gtty, NULL, NULL, "gtty", 0);
#endif
	add_call(__NR_access, dummy_call, okfs_access, "access", 0);
#ifdef __NR_nice		 
	add_call(__NR_nice, NULL, NULL, "nice", 0);
#endif
#ifdef __NR_ftime
	add_call(__NR_ftime, unimplemented_call, NULL, "ftime", 0);
#endif
	add_call(__NR_sync, NULL, NULL, "sync", 0);
	add_call(__NR_kill, NULL, NULL, "kill", 0);
	add_call(__NR_rename, okfs_before_lnk_rnm, okfs_rename, "rename", 0);
	add_call(__NR_mkdir, dummy_call, okfs_mkdir, "mkdir", 0);
	add_call(__NR_rmdir, dummy_call, okfs_rmdir, "rmdir", 0);
	add_call(__NR_dup, okfs_op_on_fd, okfs_dup, "dup", 0);
	add_call(__NR_pipe, NULL, NULL, "pipe", 0);
	add_call(__NR_times, NULL, NULL, "times", 0);
#ifdef __NR_prof
	add_call(__NR_prof, NULL, NULL, "prof", 0);
#endif
	add_call(__NR_brk, NULL, NULL, "brk", 0);
	add_call(__NR_setgid, NULL, NULL, "setgid", 0);
	add_call(__NR_getgid, NULL, NULL, "getgid", 0);
	add_call(__NR_signal, NULL, NULL, "signal", 0);
	add_call(__NR_geteuid, NULL, NULL, "geteuid", 0);
	add_call(__NR_getegid, NULL, NULL, "getegid", 0);
	add_call(__NR_acct, dummy_call, okfs_acct, "acct", 0);
#ifdef __NR_umount2
	add_call(__NR_umount2, NULL, NULL, "umount2", 0);
#endif
#ifdef __NR_lock
	add_call(__NR_lock, NULL, NULL, "lock", 0);
#endif
	add_call(__NR_ioctl, okfs_only_os, NULL, "ioctl", 0);
	add_call(__NR_fcntl, okfs_before_fcntl, NULL, "fcntl", 0);
#ifdef __NR_mpx
	add_call(__NR_mpx, NULL, NULL, "mpx", 0);
#endif
	add_call(__NR_setpgid, NULL, NULL, "setpgid", 0);
	add_call(__NR_ulimit, NULL, NULL, "ulimit", 0);
#ifdef __NR_oldolduname
	add_call(__NR_oldolduname, NULL, NULL, "oldolduname", 0);
#endif
	add_call(__NR_umask, NULL, NULL, "umask", 0);
	add_call(__NR_chroot, NULL, NULL, "chroot", 0);
#ifdef __NR_ustat
	add_call(__NR_ustat, NULL, NULL, "ustat", 0);
#endif
	add_call(__NR_dup2, NULL, okfs_dup2, "dup2", 0);
	add_call(__NR_getppid, NULL, NULL, "getppid", 0);
	add_call(__NR_getpgrp, NULL, NULL, "getpgrp", 0);
	add_call(__NR_setsid, NULL, NULL, "setsid", 0);
	add_call(__NR_sigaction, NULL, after_sigaction, "sigaction", 0);
#ifdef __NR_sgetmask
	add_call(__NR_sgetmask, NULL, NULL, "sgetmask", 0);
#endif
#ifdef __NR_ssetmask
	add_call(__NR_ssetmask, NULL, NULL, "ssetmask", 0);
#endif
	add_call(__NR_setreuid, NULL, NULL, "setreuid", 0);
	add_call(__NR_setregid, NULL, NULL, "setregid", 0);
	add_call(__NR_sigsuspend, NULL, NULL, "sigsuspend", 0);
	add_call(__NR_sigpending, NULL, NULL, "sigpending", 0);
#ifdef __NR_sethostname
	add_call(__NR_sethostname, NULL, NULL, "sethostname", 0);
#endif
	add_call(__NR_setrlimit, NULL, NULL, "setrlimit", 0);
	add_call(__NR_getrlimit, NULL, NULL, "getrlimit", 0);
#ifdef __NR_getrusage
	add_call(__NR_getrusage, NULL, NULL, "getrusage", 0);
#endif
	add_call(__NR_gettimeofday, NULL, NULL, "gettimeofday", 0);
	add_call(__NR_settimeofday, NULL, NULL, "settimeofday", 0);
#ifdef __NR_getgroups
	add_call(__NR_getgroups, NULL, NULL, "getgroups", 0);
#endif
#ifdef __NR_setgroups
	add_call(__NR_setgroups, NULL, NULL, "setgroups", 0);
#endif
	add_call(__NR_select, NULL, NULL, "select", 0);
	add_call(__NR_symlink, dummy_call, okfs_symlink, "symlink", 0);
#ifdef __NR_oldlstat
	add_call(__NR_oldlstat, unimplemented_call, NULL, "oldlstat", 0);
#endif
	add_call(__NR_readlink, dummy_call, okfs_readlink, "readlink", 0);
#ifdef __NR_uselib
	add_call(__NR_uselib, TODO, TODO, "uselib", 0);
#endif
#ifdef __NR_swapon
	add_call(__NR_swapon, NULL, NULL, "swapon", 0);
#endif
#ifdef __NR_reboot
	add_call(__NR_reboot, NULL, NULL, "reboot", 0);
#endif
	add_call(__NR_readdir, okfs_only_os, NULL, "readdir", 0);
	add_call(__NR_mmap, okfs_before_mmap, NULL, "mmap", 1);
	add_call(__NR_munmap, NULL, NULL, "munmap", 0);
	add_call(__NR_truncate, dummy_call, okfs_truncate, "truncate", 0);
	add_call(__NR_ftruncate, okfs_fd_call, okfs_ftruncate, "ftruncate", 0);
	add_call(__NR_fchmod, okfs_fd_call, okfs_fchmod, "fchmod", 0);
	add_call(__NR_fchown, okfs_fd_call, okfs_fchown, "fchown", 0);
#ifdef __NR_getpriority
	add_call(__NR_getpriority, NULL, NULL, "getpriority", 0);
#endif
#ifdef __NR_setpriority
	add_call(__NR_setpriority, NULL, NULL, "setpriority", 0);
#endif
#ifdef __NR_profil
	add_call(__NR_profil, NULL, NULL, "profil", 0);
#endif
	add_call(__NR_statfs, dummy_call, okfs_statfs, "statfs", 0);
	add_call(__NR_fstatfs, okfs_fd_call, okfs_fstatfs, "fstatfs", 0);
#ifdef __NR_ioperm
	add_call(__NR_ioperm, NULL, NULL, "ioperm", 0);
#endif
	add_call(__NR_socketcall, NULL, NULL, "socketcall", 0);
#ifdef __NR_syslog
	add_call(__NR_syslog, NULL, NULL, "syslog", 0);
#endif
	add_call(__NR_setitimer, NULL, NULL, "setitimer", 0);
	add_call(__NR_getitimer, NULL, NULL, "getitimer", 0);
	add_call(__NR_stat, dummy_call, okfs_stat, "stat", 0);
	add_call(__NR_lstat, dummy_call, okfs_lstat, "lstat", 0);
	add_call(__NR_fstat, okfs_fd_call, okfs_fstat, "fstat", 0);
#ifdef __NR_olduname
	add_call(__NR_olduname, NULL, NULL, "olduname", 0);
#endif
#ifdef __NR_iopl
	add_call(__NR_iopl, NULL, NULL, "iopl", 0);
#endif
#ifdef __NR_vhangup
	add_call(__NR_vhangup, NULL, NULL, "vhangup", 0);
#endif
#ifdef __NR_idle
	add_call(__NR_idle, NULL, NULL, "idle", 0);
#endif
#ifdef __NR_vm86old
	add_call(__NR_vm86old, NULL, NULL, "vm86old", 0);
#endif
	add_call(__NR_wait4, before_wait, NULL, "wait4", 0);
#ifdef __NR_swapoff
	add_call(__NR_swapoff, NULL, NULL, "swapoff", 0);
#endif
#ifdef __NR_sysinfo
	add_call(__NR_sysinfo, NULL, NULL, "sysinfo", 0);
#endif
#ifdef __NR_ipc
	add_call(__NR_ipc, NULL, NULL, "ipc", 0);
#endif
	add_call(__NR_fsync, okfs_only_os, NULL, "fsync", 0);
	add_call(__NR_sigreturn, NULL, NULL, "sigreturn", 0);
	add_call(__NR_clone, before_clone, NULL, "clone", 0);
#ifdef __NR_setdomainname
	add_call(__NR_setdomainname, NULL, NULL, "setdomainname", 0);
#endif
	add_call(__NR_uname, NULL, NULL, "uname", 0);
#ifdef __NR_modify_ldt
	add_call(__NR_modify_ldt, NULL, NULL, "modify_ldt", 0);
#endif
#ifdef __NR_adjtimex
	add_call(__NR_adjtimex, NULL, NULL, "adjtimex", 0);
#endif

	/*useful for threads?*/
#ifdef __NR_mprotect
	add_call(__NR_mprotect, NULL, NULL, "mprotect", 0); 
#endif
	
	add_call(__NR_sigprocmask, NULL, NULL, "sigprocmask", 0);
#ifdef __NR_create_module
	add_call(__NR_create_module, NULL, NULL, "create_module", 0);
#endif
#ifdef __NR_init_module
	add_call(__NR_init_module, NULL, NULL, "init_module", 0);
#endif
#ifdef __NR_delete_module
	add_call(__NR_delete_module, NULL, NULL, "delete_module", 0);
#endif
#ifdef __NR_get_kernel_syms
	add_call(__NR_get_kernel_syms, NULL, NULL, "get_kernel_syms", 0);
#endif
#ifdef __NR_quotactl
	add_call(__NR_quotactl, NULL, NULL, "quotactl", 0);
#endif
	add_call(__NR_getpgid, NULL, NULL, "getpgid", 0);
	add_call(__NR_fchdir, dummy_call, okfs_fchdir, "fchdir", 0);
#ifdef __NR_bdflush
	add_call(__NR_bdflush, NULL, NULL, "bdflush", 0);
#endif
#ifdef __NR_sysfs
	add_call(__NR_sysfs, NULL, NULL, "sysfs", 0);
#endif
#ifdef __NR_personality
	add_call(__NR_personality, NULL, NULL, "personality", 0);
#endif
#ifdef __NR_afs_syscall
	add_call(__NR_afs_syscall, NULL, NULL, "afs_syscall", 0);
#endif
#ifdef __NR_setfsuid
	add_call(__NR_setfsuid, NULL, NULL, "setfsuid", 0);
#endif
#ifdef __NR_setfsgid
	add_call(__NR_setfsgid, NULL, NULL, "setfsgid", 0);
#endif
#ifdef __NR__llseek
	add_call(__NR__llseek, okfs_only_os, NULL, "_llseek", 0);
#endif
	add_call(__NR_getdents, okfs_fd_call, okfs_getdents, "getdents", 0);
#ifdef __NR__newselect
	add_call(__NR__newselect, NULL, NULL, "_newselect", 0);
#endif
#ifdef __NR_flock
	add_call(__NR_flock, okfs_only_os, NULL, "flock", 0);
#endif
#ifdef __NR_msync
	add_call(__NR_msync, NULL, NULL, "msync", 0);
#endif
#ifdef __NR_readv
	add_call(__NR_readv, okfs_only_os, NULL, "readv", 0);
#endif
#ifdef __NR_writev
	add_call(__NR_writev, okfs_only_os, NULL, "writev", 0);
#endif
	add_call(__NR_getsid, NULL, NULL, "getsid", 0);
#ifdef __NR_fdatasync
	add_call(__NR_fdatasync, okfs_only_os, NULL, "fdatasync", 0);
#endif
#ifdef __NR__sysctl
	add_call(__NR__sysctl, NULL, NULL, "_sysctl", 0);
#endif
#ifdef __NR_mlock
	add_call(__NR_mlock, NULL, NULL, "mlock", 0);
#endif
#ifdef __NR_munlock
	add_call(__NR_munlock, NULL, NULL, "munlock", 0);
#endif
#ifdef __NR_mlockall
	add_call(__NR_mlockall, NULL, NULL, "mlockall", 0);
#endif
#ifdef __NR_munlockall
	add_call(__NR_munlockall, NULL, NULL, "munlockall", 0);
#endif
#ifdef __NR_sched_setparam
	add_call(__NR_sched_setparam, NULL, NULL, "sched_setparam", 0);
#endif
#ifdef __NR_sched_getparam
	add_call(__NR_sched_getparam, NULL, NULL, "sched_getparam", 0);
#endif
#ifdef __NR_sched_setscheduler
	add_call(__NR_sched_setscheduler, NULL, NULL, "sched_setscheduler", 0);
#endif
#ifdef __NR_sched_getscheduler
	add_call(__NR_sched_getscheduler, NULL, NULL, "sched_getscheduler", 0);
#endif
#ifdef __NR_sched_yield
	add_call(__NR_sched_yield, NULL, NULL, "sched_yield", 0);
#endif
#ifdef __NR_sched_get_priority_max
	add_call(__NR_sched_get_priority_max, NULL, NULL, 
		 "sched_get_priority_max", 0);
#endif

#ifdef __NR_sched_get_priority_min
	add_call(__NR_sched_get_priority_min, NULL, NULL, 
		 "sched_get_priority_min", 0);
#endif
#ifdef __NR_sched_rr_get_interval
	add_call(__NR_sched_rr_get_interval, NULL, NULL,
		 "sched_rr_get_interval", 0);
#endif
	add_call(__NR_nanosleep, NULL, NULL, "nanosleep", 0);
#ifdef __NR_mremap
	add_call(__NR_mremap, NULL, NULL, "mremap", 0);
#endif
#ifdef __NR_setresuid
	add_call(__NR_setresuid, NULL, NULL, "setresuid", 0);
#endif
#ifdef __NR_getresuid
	add_call(__NR_getresuid, NULL, NULL, "getresuid", 0);
#endif
#ifdef __NR_vm86
	add_call(__NR_vm86, NULL, NULL, "vm86", 0);
#endif
#ifdef __NR_query_module
	add_call(__NR_query_module, NULL, NULL, "query_module", 0);
#endif
#ifdef __NR_poll
	add_call(__NR_poll, NULL, NULL, "poll", 0);
#endif
#ifdef __NR_nfsservctl
	add_call(__NR_nfsservctl, NULL, NULL, "nfsservctl", 0);
#endif
#ifdef __NR_setresgid
	add_call(__NR_setresgid, NULL, NULL, "setresgid", 0);
#endif
#ifdef __NR_getresgid
	add_call(__NR_getresgid, NULL, NULL, "getresgid", 0);
#endif
#ifdef __NR_prctl
	add_call(__NR_prctl, NULL, NULL, "prctl", 0);
#endif
#ifdef __NR_rt_sigreturn
	add_call(__NR_rt_sigreturn, NULL, NULL, "rt_sigreturn", 0);
#endif
#ifdef __NR_rt_sigaction
	add_call(__NR_rt_sigaction, NULL, after_rt_sigaction, 
		 "rt_sigaction", 0);
#endif
#ifdef __NR_rt_sigprocmask
	add_call(__NR_rt_sigprocmask, NULL, NULL, "rt_sigprocmask", 0);
#endif
#ifdef __NR_rt_sigpending
	add_call(__NR_rt_sigpending, NULL, NULL, "rt_sigpending", 0);
#endif
#ifdef __NR_rt_sigtimedwait
	add_call(__NR_rt_sigtimedwait, NULL, NULL, "rt_sigtimedwait", 0);
#endif
#ifdef __NR_rt_sigqueueinfo
	add_call(__NR_rt_sigqueueinfo, NULL, NULL, "rt_sigqueueinfo", 0);
#endif
#ifdef __NR_rt_sigsuspend
	add_call(__NR_rt_sigsuspend, NULL, NULL, "rt_sigsuspend", 0);
#endif
#ifdef __NR_pread
	add_call(__NR_pread, okfs_only_os, NULL, "pread", 0);
#endif
#ifdef __NR_pwrite
	add_call(__NR_pwrite, okfs_only_os, NULL, "pwrite", 0);
#endif
	add_call(__NR_chown, dummy_call, okfs_chown, "chown", 0);
	add_call(__NR_getcwd, NULL, okfs_getcwd, "getcwd", 0);
#ifdef __NR_capget
	add_call(__NR_capget, NULL, NULL, "capget", 0);
#endif
#ifdef __NR_capset
	add_call(__NR_capset, NULL, NULL, "capset", 0);
#endif
#ifdef __NR_sigaltstack
	add_call(__NR_sigaltstack, NULL, NULL, "sigaltstack", 0);
#endif

	/*TODO*/
#ifdef __NR_sendfile
	add_call(__NR_sendfile, unimplemented_call, NULL, "sendfile", 0); 
#endif

#ifdef __NR_getpmsg
	add_call(__NR_getpmsg, NULL, NULL, "getpmsg", 0);
#endif
#ifdef __NR_putpmsg
	add_call(__NR_putpmsg, NULL, NULL, "putpmsg", 0);
#endif
#ifdef __NR_vfork
	add_call(__NR_vfork, not_supported, NULL, "vfork", 0);
#endif
#ifdef __NR_ugetrlimit
	add_call(__NR_ugetrlimit, NULL, NULL, "ugetrlimit", 0);
#endif
#ifdef __NR_mmap2
	add_call(__NR_mmap2, okfs_before_mmap, NULL, "mmap2", 0);
#endif
	add_call(__NR_truncate64, dummy_call, okfs_truncate, "truncate64", 0);
	add_call(__NR_ftruncate64, okfs_fd_call, okfs_ftruncate, 
		 "ftruncate64", 0);
	add_call(__NR_stat64, dummy_call, okfs_stat64, "stat64", 0);
	add_call(__NR_lstat64, dummy_call, okfs_lstat64, "lstat64", 0);
	add_call(__NR_fstat64, okfs_fd_call, okfs_fstat64, "fstat64", 0);
	add_call(__NR_lchown32, dummy_call, okfs_lchown, "lchown32", 0);
	add_call(__NR_getuid32, NULL, NULL, "getuid32", 0);
	add_call(__NR_getgid32, NULL, NULL, "getgid32", 0);
	add_call(__NR_geteuid32, NULL, NULL, "geteuid32", 0);
	add_call(__NR_getegid32, NULL, NULL, "getegid32", 0);
	add_call(__NR_setreuid32, NULL, NULL, "setreuid32", 0);
	add_call(__NR_setregid32, NULL, NULL, "setregid32", 0);
	add_call(__NR_getgroups32, NULL, NULL, "getgroups32", 0);
	add_call(__NR_setgroups32, NULL, NULL, "setgroups32", 0);
	add_call(__NR_fchown32, okfs_fd_call, okfs_fchown, "fchown32", 0);
	add_call(__NR_setresuid32, NULL, NULL, "setresuid32", 0);
	add_call(__NR_getresuid32, NULL, NULL, "getresuid32", 0);
	add_call(__NR_setresgid32, NULL, NULL, "setresgid32", 0);
	add_call(__NR_getresgid32, NULL, NULL, "getresgid32", 0);
	add_call(__NR_chown32, dummy_call, okfs_chown, "chown32", 0);
	add_call(__NR_setuid32, NULL, NULL, "setuid32", 0);
	add_call(__NR_setgid32, NULL, NULL, "setgid32", 0);
	add_call(__NR_setfsuid32, NULL, NULL, "setfsuid32", 0);
	add_call(__NR_setfsgid32, NULL, NULL, "setfsgid32", 0);
#ifdef __NR_pivot_root
	add_call(__NR_pivot_root, NULL, NULL, "pivot_root", 0);
#endif
#ifdef __NR_mincore
	add_call(__NR_mincore, NULL, NULL, "mincore", 0);
#endif
#ifdef __NR_madvise
	add_call(__NR_madvise, NULL, NULL, "madvise", 0);
#endif
	add_call(__NR_getdents64, okfs_fd_call, okfs_getdents64, 
		 "getdents64", 0);
	add_call(__NR_fcntl64, okfs_before_fcntl, NULL, "fcntl64", 0);
#ifdef __NR_security
	add_call(__NR_security, NULL, NULL, "security", 0);
#endif
#ifdef __NR_gettid
	add_call(__NR_gettid, NULL, NULL, "gettid", 0);
#endif
#ifdef __NR_readahead
	add_call(__NR_readahead, okfs_only_os, NULL, "readahead", 0);
#endif
#ifdef __NR_setxattr
	add_call(__NR_setxattr, not_supported, NULL, "setxattr", 0);
#endif
#ifdef __NR_lsetxattr
	add_call(__NR_lsetxattr, not_supported, NULL, "lsetxattr", 0);
#endif
#ifdef __NR_fsetxattr
	add_call(__NR_fsetxattr, not_supported, NULL, "fsetxattr", 0);
#endif
#ifdef __NR_getxattr
	add_call(__NR_getxattr, not_supported, NULL, "getxattr", 0);
#endif
#ifdef __NR_lgetxattr
	add_call(__NR_lgetxattr, not_supported, NULL, "lgetxattr", 0);
#endif
#ifdef __NR_fgetxattr
	add_call(__NR_fgetxattr, not_supported, NULL, "fgetxattr", 0);
#endif
#ifdef __NR_listxattr
	add_call(__NR_listxattr, not_supported, NULL, "listxattr", 0);
#endif
#ifdef __NR_llistxattr
	add_call(__NR_llistxattr, not_supported, NULL, "llistxattr", 0);
#endif
#ifdef __NR_flistxattr
	add_call(__NR_flistxattr, not_supported, NULL, "flistxattr", 0);
#endif
#ifdef __NR_removexattr
	add_call(__NR_removexattr, not_supported, NULL, "removexattr", 0);
#endif
#ifdef __NR_lremovexattr
	add_call(__NR_lremovexattr, not_supported, NULL, "lremovexattr", 0);
#endif
#ifdef __NR_fremovexattr
	add_call(__NR_fremovexattr, not_supported, NULL, "fremovexattr", 0);
#endif
	add_call(__NR_tkill, NULL, NULL, "tkill", 0);
#ifdef __NR_sendfile64
	add_call(__NR_sendfile64, unimplemented_call, NULL, "sendfile64", 0);
#endif
#ifdef __NR_futex
	add_call(__NR_futex, NULL, NULL, "futex", 0);
#endif
#ifdef __NR_sched_setaffinity
	add_call(__NR_sched_setaffinity, NULL, NULL, "sched_setaffinity", 0);
#endif
#ifdef __NR_sched_getaffinity
	add_call(__NR_sched_getaffinity, NULL, NULL, "sched_getaffinity", 0);
#endif
#ifdef __NR_set_thread_area
	add_call(__NR_set_thread_area, NULL, NULL, "set_thread_area", 0);
#endif
#ifdef __NR_get_thread_area
	add_call(__NR_get_thread_area, NULL, NULL, "get_thread_area", 0);
#endif
#ifdef __NR_io_setup
	add_call(__NR_io_setup, NULL, NULL, "io_setup", 0);
#endif
#ifdef __NR_io_destroy
	add_call(__NR_io_destroy, NULL, NULL, "io_destroy", 0);
#endif
#ifdef __NR_io_getevents
	add_call(__NR_io_getevents, NULL, NULL, "io_getevents", 0);
#endif
#ifdef __NR_io_submit
	add_call(__NR_io_submit, NULL, NULL, "io_submit", 0);
#endif
#ifdef __NR_io_cancel
	add_call(__NR_io_cancel, NULL, NULL, "io_cancel", 0);
#endif
#ifdef __NR_alloc_hugepages
	add_call(__NR_alloc_hugepages, NULL, NULL, "alloc_hugepages", 0);
#endif
#ifdef __NR_free_hugepages
	add_call(__NR_free_hugepages, NULL, NULL, "free_hugepages", 0);
#endif
#ifdef __NR_exit_group
	add_call(__NR_exit_group, NULL, NULL, "exit_group", 0);      
#endif

	for(i = 0; i < NR_syscalls; i++)
		if (!call[i].name){
			sprintf(buf, "unknown system call %d", i);
			call[i].name = Strdup(buf);
		}
}
