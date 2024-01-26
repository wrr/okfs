/*$Id: debug.h,v 1.3 2006/05/06 22:14:46 jan_wrobel Exp $*/
#ifndef DEBUG_H
#define DEBUG_H

#include "error.h"


/*debug options:*/

#undef DEBUG_OKFS/*don't print debug informations*/
//#define DEBUG_OKFS 

#define DEBUG_FUNCTIONS ""
/*#define DEBUG_ALL 0 print debug informations only from DEBUG_FUNCTIONS*/

/*print debug information from all functions except
*this specified in NOT_DEBUG_FUNCTIONS*/
#define DEBUG_ALL 1
#define NOT_DEBUG_FUNCTIONS "canonicalize"


#include "task.h"

#ifdef DEBUG_OKFS

/*dump debug messages to a log file*/
#define TRACE(x...) do{\
	if ((DEBUG_ALL && !strstr(NOT_DEBUG_FUNCTIONS, __func__)) ||\
	    strstr(DEBUG_FUNCTIONS, __func__)){\
                 if (slave)\
		       trace_func(__func__, slave->pid, x);\
                 else\
                       trace_func(__func__, 0, x);\
	    } }while(0)

#define ASSERT(x) if (!(x)) err_quit("ASSERTion %s in function %s failed", #x, __func__);

#else  /*DEBUG_OKFS*/
#define TRACE(x...) 
#define ASSERT(x)

#endif /*DEBUG_OKFS*/

#endif


