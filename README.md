# OKFS (Out of Kernel File System)

NOTE: The project is no longer maintained. The last version of the
Linux kernel it was tested with was 2.6.x.

OKFS is an exprimental Linux virtual filesystem that works in the user
space and doesn't require root priviledges. OKFS includes implementation
of two concrete file system: SHFS - for mounting remote directories
via the SSH protocol, localFS - for mounting local folders in different
locations.

OKFS utilizes Linux ptrace system call. For more technical details see
[doc/okfs_idea.txt](doc/okfs_idea.txt)

See [COPYING](./COPYING) file for licence.
See [INSTALL](./INSTALL) for information how to compile OKFS.

If you manage to compile OKFS type 'okfs -h' for a list of supported
options. See doc/fstab.sample for information how to tell OKFS to
mount something somewhere.

OKFS uses code from other open source programs. Thanks go to:

Miroslav Spousta <qiq@ucw.cz> -  a lot of code from his SHFS kernel 
module is used.
Florin Malita <mail@go.ro> - An idea of LocalFS is taken from his LUFS.
OKFS uses, with some little changes, code from canonicalize() function
from GLIB C, for which thanks are going to Free Software Foundation.
Code of memcpy and memmove is taken from Linus Torvald's kernel
routines.

Author of OKFS:
Jan Wrobel <jan@mixedbit.org>
