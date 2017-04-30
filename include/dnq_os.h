#ifndef _DNQ_OS_H_
#define _DNQ_OS_H_

#include "common.h"
#include <pthread.h>
#include <semaphore.h>

typedef struct dnq_task
{
    U8            name[32];
    U32           stacksize;
    U32           pri;
    
    pthread_t     tid;
    
}dnq_task_t;

typedef enum msg_type
{
    MSG_TYPE_1
}msg_type_e;

typedef struct dnq_msg
{
    U32   type;
    U32          code;
    U32          datalen;
    U8           data[64];
}dnq_msg_t;

typedef struct dnq_queue
{
    U32         size;
    U32         used;
    U32         head;
    U32         tail;
    sem_t       sem;
    pthread_mutex_t mutex;
    dnq_msg_t   *msg;
}dnq_queue_t;


dnq_queue_t *dnq_queue_create(U32 size);
void dnq_queue_delete(dnq_queue_t *queue);

S32 dnq_msg_send(dnq_queue_t *queue, dnq_msg_t *msg);
S32 dnq_msg_recv(dnq_queue_t *queue, dnq_msg_t *msg);
S32 dnq_msg_send_timeout(dnq_queue_t *queue, dnq_msg_t *msg, U32 timeout);
S32 dnq_msg_recv_timeout(dnq_queue_t *queue, dnq_msg_t *msg, U32 timeout);
    
dnq_task_t* dnq_os_task_create(U8 *name, U32 stack_size, void *func, void *param);
S32 dnq_os_task_exit(dnq_task_t  *task);


#endif /* _DNQ_OS_H_ */

