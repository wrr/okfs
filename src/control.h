#ifndef CONTROL_H
#define CONTROL_H

void before_fork(void);
void after_fork(void);
void before_clone(void);
void before_wait(void);
void after_execve(void);
void before_execve(void);

#endif
