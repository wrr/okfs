/*$Id: error.h,v 1.2 2006/05/06 22:14:46 jan_wrobel Exp $*/
#ifndef ERROR_H
#define ERROR_H 

#define LOGFILE "./log"

/*error reporting/debugging routines, similar to these introduced
  by Richard Stevens in his "Advanced Programming in the Unix Environment"*/

void loginit(const char *);
void err_ret_func(const char *funcname, const char *fmt, ...);
void err_msg_func(const char *funcname, const char *fmt, ...);
void err_sys_func(const char *funcname, const char *fmt, ...);
void err_quit_func(const char *funcname, const char *fmt, ...);
void stderr_print(const char *fmt, ...);

void trace_func(const char *funcname, int pid, const char *fmt, ...);
/*void dump_registers(struct user_regs_struct);*/

#define err_sys(x...) err_sys_func(__func__, x)
#define err_msg(x...) err_msg_func(__func__, x)
#define err_ret(x...) err_ret_func(__func__, x)
#define err_quit(x...) err_quit_func(__func__, x)



#endif
