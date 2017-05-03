#ifndef _DNQ_OS_H_
#define _DNQ_OS_H_

#include "common.h"
#include <pthread.h>
#include <semaphore.h>


#define QUEUE_MSG_SIZE 64
#define QUEUE_SIZE_MAX 15

typedef enum msg_class
{
    MSG_CLASS_KEYPAD = 1,
    MSG_CLASS_LCD,
    MSG_CLASS_MCU,
    MSG_CLASS_RABBITMQ,
   
}msg_class_e;

typedef struct dnq_msg
{
    U32          Class;      /* msg class: keypad, lcd ,mcu, etc.. */
    U32          code;       /* msg type, user-defined */
    U32          lenght;     /* data lenght */
    U8          *payload;    /* dynamic ptr, user-defined */
    U8           data[64];   /* static buffer inside the msg, buffer'size=64 */

    /* 
    * 如果需要动态指定每个message的数据buffer大小，需要将data设置成指针类型
    * 在给queue分配内存时，需要根据element_size参数来决定数据buffer的大小。
    *
    * if need specify dynamic data size , data should be data ptr
    */
    //U8          *data;     /* static ptr inside the message buffer  */
}dnq_msg_t;

typedef struct dnq_queue
{
    U32         size;
    //U32         element_size; /* used for dynamic msg size */
    U32         used;
    U32         head;
    U32         tail;
    sem_t       sem;
    pthread_mutex_t mutex;
    dnq_msg_t   *msg;
}dnq_queue_t;


typedef struct dnq_task
{
    U8            name[32];
    U32           stacksize;
    U32           pri;
    
    pthread_t     tid;
    
}dnq_task_t;

typedef struct dnq_appinfo
{
    void         *func;
    dnq_task_t   *task;
    U32           msg_size;
    U32           queue_size;
    dnq_queue_t  *queue;

}dnq_appinfo_t;

dnq_queue_t *dnq_queue_create(U32 queue_size);
dnq_queue_t *dnq_queue_create1(U32 element_size, U32 queue_size);
void dnq_queue_delete(dnq_queue_t *queue);

S32 dnq_msg_send(dnq_queue_t *queue, dnq_msg_t *msg);
S32 dnq_msg_recv(dnq_queue_t *queue, dnq_msg_t *msg);
S32 dnq_msg_send_timeout(dnq_queue_t *queue, dnq_msg_t *msg, U32 timeout);
S32 dnq_msg_recv_timeout(dnq_queue_t *queue, dnq_msg_t *msg, U32 timeout);
    
dnq_task_t* dnq_task_create(U8 *name, U32 stack_size, void *func, void *param);
S32 dnq_task_delete(dnq_task_t *task);
S32 dnq_task_isExit(dnq_task_t *task);
S32 dnq_task_exit(dnq_task_t  *task);
dnq_appinfo_t * dnq_app_task_create(
    U8 *name, 
    U32 stack_size,
    U32 msg_size,
    U32 queue_size,
    void *func, 
    void *param);
S32 dnq_app_task_delete(dnq_appinfo_t *pAppinfo);
S32 dnq_app_task_exit(dnq_appinfo_t *pAppinfo);

#endif /* _DNQ_OS_H_ */

