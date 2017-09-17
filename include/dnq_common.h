#ifndef _DNQ_COMMON_H_
#define _DNQ_COMMON_H_

#include "common.h"


S32 dnq_time_init();
U32 dnq_time_now();
U32 dnq_time_now_us();
S32 dnq_system_call(U8 *command);
S32 dnq_reboot();
S32 dnq_init();
S32 dnq_deinit();

#endif /* _DNQ_COMMON_H_ */

