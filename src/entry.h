/*$Id: entry.h,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
#ifndef ENTRY_H
#define ENTRY_H

#include <linux/unistd.h>

#ifndef NR_syscalls /*2.4.* */
# define NR_syscalls __NR_exit_group + 1
#endif


void init_call_table(void);


struct call_entry{
	/*this function is called before Linux gets control and 
	  executes system call specified by this entry*/
	void (*call_before)(void);
	
	/*this function is called after system call specified by this entry
	  returns*/
	void (*call_after)(void);
	
	/*name of a system call, useful for debugging*/
	char* name;
	/*some system calls use memory block to pass
	  arguments instead of registers*/
	int has_stack_args;
};

/*Table with entry points for all system calls*/
extern struct call_entry *call; 

#endif
