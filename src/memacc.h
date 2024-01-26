/*$Id: memacc.h,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
#ifndef MEMACC_H
#define MEMACC_H

/*Routines that simplified slave's memory reading/writing*/

void *slave_memset(void *dest, const void *src, int n);
void *slave_memget(void *dest, const void *src, int n);
char *slave_getstr(void *src);
void slave_rpl_mem(void *dest, void *src, int n);
void slave_restore_mem(void);
void slave_rpl_known_str(void *dest, char *new_string, char *orig_string);

#endif
