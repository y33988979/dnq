/* Common os Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  this is a os interface API, for app.
 * Note : 
 */

#include <common.h>
#include "ngx_palloc.h"
#include "dnq_os.h"
#include "dnq_log.h"

dnq_queue_t *dnq_queue_create(U32 queue_size)
{
    S32  ret;
    U32  malloc_size;
    dnq_queue_t *queue = NULL;
    pthread_mutex_t attr;
    
    malloc_size = \
        sizeof(dnq_queue_t) + sizeof(dnq_msg_t)*queue_size;
    queue = dnq_malloc(malloc_size);
    if(queue == NULL)
    {
        DNQ_ERROR(DNQ_MOD_OS, "malloc error: %s\n", strerror(errno));
        return NULL;
    }

    memset(queue, 0, malloc_size);
    
    queue->size = queue_size;
    queue->used = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->msg = (dnq_msg_t*)(queue + sizeof(dnq_queue_t));

    ret = pthread_mutex_init(&queue->mutex, NULL);
    if(ret != 0)
    {
        dnq_free(queue);
        DNQ_ERROR(DNQ_MOD_OS, "pthread_mutex_init error: %s\n", strerror(errno));
        return NULL;
    }

    ret = sem_init(&queue->sem, 0, 0);
    if(ret != 0)
    {
        dnq_free(queue);
        pthread_mutex_destroy(&queue ->mutex);
        DNQ_ERROR(DNQ_MOD_OS, "sem_init error: %s\n", strerror(errno));
        return NULL;
    }
    return queue;
}

#if 0
dnq_queue_t *dnq_queue_create1(U32 element_size, U32 queue_size)
{
    S32  i, ret;
    U32  malloc_size;
    dnq_queue_t *queue = NULL;
    pthread_mutex_t attr;
    
    malloc_size = \
        sizeof(dnq_queue_t) + (sizeof(dnq_msg_t)+element_size)*queue_size;
    queue = dnq_malloc(malloc_size);
    if(queue == NULL)
    {
        DNQ_ERROR(DNQ_MOD_OS, "malloc error: %s\n", strerror(errno));
        return NULL;
    }

    memset(queue, 0, malloc_size);
    
    queue->size = queue_size;
    queue->used = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->msg = (dnq_msg_t*)(queue + sizeof(dnq_msg_t));

    for(i=0; i<queue_size; i++)
    {
        /* static ptr inside the message buffer, Not allowed to be changed!! */
        queue->msg[i].data = (U8*)((S32)&queue->msg[i] + sizeof(dnq_msg_t));
        /* dynamic ptr , allowed to be changed */
        queue->msg[i].payload = NULL;
    }

    ret = pthread_mutex_init(&queue->mutex, NULL);
    if(ret != 0)
    {
        dnq_free(queue);
        DNQ_ERROR(DNQ_MOD_OS, "pthread_mutex_init error: %s\n", strerror(errno));
        return NULL;
    }

    ret = sem_init(&queue->sem, 0, 0);
    if(ret != 0)
    {
        dnq_free(queue);
        pthread_mutex_destroy(&queue ->mutex);
        DNQ_ERROR(DNQ_MOD_OS, "sem_init error: %s\n", strerror(errno));
        return NULL;
    }
    return queue;
}
#endif

void dnq_queue_delete(dnq_queue_t *queue)
{
    if(sem_destroy(&queue->sem) != 0)
    {
        DNQ_ERROR(DNQ_MOD_OS, "sem_destroy error: %s\n", strerror(errno));
    }
    if(pthread_mutex_destroy(&queue ->mutex) != 0) 
    {
        DNQ_ERROR(DNQ_MOD_OS, "pthread_mutex_destroy error: %s\n", strerror(errno));
    }
    dnq_free(queue);
}

S32 dnq_msg_send(dnq_queue_t *queue, dnq_msg_t *msg)
{
    pthread_mutex_lock(&queue->mutex);

    if(queue->used >= queue->size-1)
    {
        pthread_mutex_unlock(&queue->mutex);
        DNQ_ERROR(DNQ_MOD_OS, "queue is full!");
        return -1;
    }

    memcpy(&queue->msg[queue->tail], msg, sizeof(dnq_msg_t));
    //memcpy(&queue->msg[queue->tail].data, msg->data, msg->lenght);
    queue->tail = (queue->tail+1) % queue->size;
    queue->used++;

    pthread_mutex_unlock(&queue->mutex);
    if(sem_post(&queue->sem) < 0)
    {
        DNQ_ERROR(DNQ_MOD_OS, "sem_post error: %s\n", strerror(errno));
        return -1;
    }
    return 0;

}

S32 dnq_msg_send_timeout(dnq_queue_t *queue, dnq_msg_t *msg, U32 timeout)
{
    pthread_mutex_lock(&queue->mutex);

    if(queue->used >= queue->size-1)
    {
        pthread_mutex_unlock(&queue->mutex);
        DNQ_ERROR(DNQ_MOD_OS, "queue is full!");
        return -1;
    }

    memcpy(&queue->msg[queue->tail], msg, sizeof(dnq_msg_t));
    //memcpy(&queue->msg[queue->tail].data, msg->data, msg->lenght);
    queue->tail = (queue->tail+1) % queue->size;
    queue->used++;
    
    pthread_mutex_unlock(&queue->mutex);
    if(sem_post(&queue->sem) < 0)
    {
        DNQ_ERROR(DNQ_MOD_OS, "sem_post error: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

S32 dnq_msg_recv(dnq_queue_t *queue, dnq_msg_t *msg)
{

    if(sem_wait(&queue->sem) < 0)
    {
        if(errno != ETIMEDOUT)
            DNQ_ERROR(DNQ_MOD_OS, "sem_wait error: %s\n", strerror(errno));
        return -1;
    }

    pthread_mutex_lock(&queue->mutex);
    if(queue->used == 0)
    {
        pthread_mutex_unlock(&queue->mutex);
        DNQ_ERROR(DNQ_MOD_OS, "queue is empty!");
        return -1;
    }

    memcpy(msg, &queue->msg[queue->head], sizeof(dnq_msg_t));
    //memcpy(msg.data, &queue->msg[queue->head].data, msg->lenght);
    queue->head = (queue->head+1) % queue->size;
    queue->used--;
    
    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

S32 dnq_msg_recv_timeout(dnq_queue_t *queue, dnq_msg_t *msg, U32 timeout)
{
    struct timespec ts;
    struct timeval  tv;
	U32 sec, msec, time;
    U32 ret;
    
    time = timeout;
    gettimeofday(&tv, NULL);
    sec = time / 1000;
    msec = time - (sec * 1000);
    tv.tv_sec += sec;
    tv.tv_usec += msec * 1000;
    sec = tv.tv_usec / 1000000;
    tv.tv_usec = tv.tv_usec - (sec * 1000000);
    tv.tv_sec += sec;
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = ((long)tv.tv_usec*1000);
    if((ret=sem_timedwait(&queue->sem, &ts)) != 0)
    {
        if(errno != ETIMEDOUT)
            DNQ_ERROR(DNQ_MOD_OS, "sem_timedwait error: %s\n", strerror(errno));
        return -1;
    }

    pthread_mutex_lock(&queue->mutex);
    if(queue->used == 0)
    {
        pthread_mutex_unlock(&queue->mutex);
        DNQ_ERROR(DNQ_MOD_OS, "queue is empty!");
        return -1;
    }

    memcpy(msg, &queue->msg[queue->head], sizeof(dnq_msg_t));
    //memcpy(msg.data, &queue->msg[queue->head].data, msg->lenght);
    queue->head = (queue->head+1) % queue->size;
    queue->used--;
    
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

void *test1(void *args)
{
    while(1) 
        {
        printf("test thread!\n");
        sleep(2);
        }
}

dnq_task_t* dnq_task_create(U8 *name, U32 stack_size, void *func, void *param)
{
    S32          ret;
    dnq_task_t  *task;
    pthread_attr_t attr;

    task = (dnq_task_t *)dnq_malloc(sizeof(dnq_task_t));
    if(!task)
    {
        DNQ_ERROR(DNQ_MOD_OS, "malloc error!");
        return NULL;
    }

    strcpy(task->name, name);
    ret = pthread_attr_init(&attr);
    if (ret != 0)
    {
        dnq_free(task);
        DNQ_ERROR(DNQ_MOD_OS, "pthread_attr_init error: %s\n", strerror(errno));
        return NULL;
    }

    if (stack_size > 0)
    {
        ret = pthread_attr_setstacksize(&attr, stack_size);
        if(ret != 0)
        {
            dnq_free(task);
            DNQ_ERROR(DNQ_MOD_OS, "pthread_attr_setstacksize error: %s\n", strerror(errno));
            return NULL;
        }
    }

    task->stacksize = stack_size;
    tt;
    pthread_t tid;
    ret = pthread_create(&task->tid, &attr, func, param);
    //ret = pthread_create(&tid, NULL, test1, NULL);//&attr
    if(ret < 0)
    {
        dnq_free(task);
        DNQ_ERROR(DNQ_MOD_OS, "pthread_create error: %s\n", strerror(errno));
        return NULL;
    }
tt;
    ret = pthread_attr_destroy(&attr);
    if(ret != 0)
    {
        DNQ_ERROR(DNQ_MOD_OS, "pthread_attr_destroy error: %s\n", strerror(errno));
        //return NULL;
    }

    return task;
}

U32 dnq_task_isExit(dnq_task_t *task)
{
    S32 ret;
    ret = pthread_kill(task, 0);
    if(ret == ESRCH)
        return 1;
    DNQ_ERROR(DNQ_MOD_OS, "the %s task is not exit!!", task->name);
    return 0;
}


S32 dnq_task_exit(dnq_task_t *task)
{
    S32            ret;

    ret = pthread_cancel(task->tid);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_OS, "name %s: pthread_cancel error: %s\n",\
            task->name, strerror(errno));
        return -1;
    }
    ret = pthread_join(task->tid, NULL);
    //ret = pthread_timedjoin_np(task->tid, NULL ,ts);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_OS, "pthread_join error: %s\n", strerror(errno));
        return -1;
    }

    free(task);
    return ret;
}

dnq_appinfo_t * dnq_app_task_create(
    U8 *name, 
    U32 stack_size,
    U32 msg_size,
    U32 queue_size,
    void *func, 
    void *param)
{
    dnq_appinfo_t *appinfo = NULL;

    appinfo = (dnq_appinfo_t *)dnq_malloc(sizeof(dnq_appinfo_t));
    if(appinfo == NULL)
    {
        DNQ_ERROR(DNQ_MOD_OS, "name %s: dnq_malloc appinfo error!", name);
        return NULL;
    }
    
    appinfo->msg_size = msg_size;
    appinfo->queue_size = queue_size;
    appinfo->queue = dnq_queue_create(queue_size);
    if(appinfo->queue == NULL)
    {
        dnq_free(appinfo);
        DNQ_ERROR(DNQ_MOD_OS, "name %s: dnq_queue_create error!", name);
        return NULL;
    }
    tt;
    appinfo->task = dnq_task_create(name, stack_size, func, (void*)appinfo);
    if(appinfo->task == NULL)
    {
        dnq_free(appinfo->queue);
        dnq_free(appinfo);
        DNQ_ERROR(DNQ_MOD_OS, "name %s: dnq_task_create error!", name);
        return NULL;
    }
tt;
    return appinfo;
}

S32 dnq_app_task_exit(dnq_appinfo_t *pAppinfo)
{
    S32 ret;
    dnq_queue_delete(pAppinfo->queue);
    ret = dnq_task_exit(pAppinfo->task);
    return ret;
}

