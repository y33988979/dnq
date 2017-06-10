#ifndef _COMMON_H_
#define _COMMON_H_


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <typedef.h>

#include "dnq_config.h"

#define swap16(n) \
   ((n & 0xff) << 8) |\
   ((n & 0xff0000) >> 8)

#define swap32(n) \
   ((n & 0xff) << 24) |\
   ((n & 0xff00) << 8) |\
   ((n & 0xff0000) >> 8) |\
   ((n & 0xff000000) >> 24)
    

#endif /* _COMMON_H_ */

