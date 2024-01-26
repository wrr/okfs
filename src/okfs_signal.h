/*$Id: okfs_signal.h,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
#ifndef OKFS_SIGNAL_H
#define OKFS_SIGNAL_H


#define STOPPING_SIG(sig) ((sig) == SIGSTOP || (sig) == SIGTSTP || \
                           (sig) == SIGTTIN || (sig) == SIGTTOU)


int process_signal(int signal, int status);
void after_sigaction(void);
void after_rt_sigaction(void);
void deliver_pending_signals(void);

#endif
