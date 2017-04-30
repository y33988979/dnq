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
    
    malloc_size = sizeof(dnq_queue_t) + sizeof(dnq_msg_t)*queue_size;
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
    queue->head = (queue->head+1) % queue->size;
    queue->used--;
    
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

dnq_task_t* dnq_os_task_create(U8 *name, U32 stack_size, void *func, void *param)
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
    ret = pthread_create(&task->tid, NULL, func, param);
    if(ret < 0)
    {
        dnq_free(task);
        DNQ_ERROR(DNQ_MOD_OS, "pthread_create error: %s\n", strerror(errno));
        return NULL;
    }

    ret = pthread_attr_destroy(&attr);
    if(ret != 0)
    {
        DNQ_ERROR(DNQ_MOD_OS, "pthread_attr_destroy error: %s\n", strerror(errno));
        //return NULL;
    }

    return task;
}

S32 dnq_os_task_exit(dnq_task_t  *task)
{
    S32            ret;
    
    ret = pthread_kill(&task->tid, NULL);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_OS, "pthread_kill error: %s\n", strerror(errno));
        return -1;
    }

    free(task);
    return ret;
}

