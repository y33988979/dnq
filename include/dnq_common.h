#ifndef _DNQ_COMMON_H_
#define _DNQ_COMMON_H_

#include "common.h"

#define swap16(n) \
   ((n & 0xff) << 8) |\
   ((n & 0xff0000) >> 8)

#define swap32(n) \
   ((n & 0xff) << 24) |\
   ((n & 0xff00) << 8) |\
   ((n & 0xff0000) >> 8) |\
   ((n & 0xff000000) >> 24)

S32 dnq_time_init();
U32 dnq_time_now();
U32 dnq_time_now_us();
S32 dnq_system_call(U8 *command);
S32 dnq_init();
S32 dnq_deinit();

#endif /* _DNQ_COMMON_H_ */

