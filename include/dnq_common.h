#ifndef _DNQ_OS_H_
#define _DNQ_OS_H_

#include "common.h"
#include <pthread.h>

#define POLY 0x8005

U16 crc16(U8 *addr, U32 num ,U32 crc_initial);

S32 dnq_init();
S32 dnq_deinit();


#endif /* _DNQ_OS_H_ */

