#ifndef _DNQ_OS_H_
#define _DNQ_OS_H_

#include "common.h"
#include <pthread.h>

typedef struct dnq_task
{
    U8            name[32];
    U32           stacksize;
    U32           pri;
    
    pthread_t     tid;
    
}dnq_task_t;

dnq_task_t* dnq_os_task_create(U8 *name, U32 stack_size, void *func, void *param);
S32 dnq_os_task_exit(dnq_task_t  *task);


#endif /* _DNQ_OS_H_ */

