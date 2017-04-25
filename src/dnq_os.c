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
#include "dnq_os.h"
#include "dnq_log.h"

S32 dnq_os_mem_init()
{

}

S32 dnq_os_mem_deinit()
{

}

dnq_task_t* dnq_os_task_create(U8 *name, U32 stack_size, void *func, void *param)
{
    S32          ret;
    dnq_task_t  *task;
    pthread_attr_t attr;
    
    task = (dnq_task_t *)malloc(sizeof(dnq_task_t));
    if(!task)
        return NULL;

    strcpy(task->name, name);
    ret = pthread_attr_init(&attr);
    if (ret != 0)
    {
        fprintf(stderr, "pthread_attr_init error: %s\n", strerror(errno));
        dnq_error(ret, "pthread_attr_init");
    }

    if (stack_size > 0)
    {
        ret = pthread_attr_setstacksize(&attr, stack_size);
        if(ret != 0)
        {
            dnq_error(ret, "pthread_attr_setstacksize error");
        }
    }
    
    ret = pthread_create(&task->tid, NULL, func, param);
    if(ret < 0)
    {
        perror("pthread_create error");
        dnq_error(ret, "pthread_create error");
        return NULL;
    }

    ret = pthread_attr_destroy(&attr);
    if(ret != 0)
        dnq_error(ret, "pthread_attr_destroy error");

    return task;
}

S32 dnq_os_task_exit(dnq_task_t  *task)
{
    S32            ret;
    
    ret = pthread_kill(&task->tid, NULL);
    if(ret < 0)
    {
        perror("pthread_kill error");
        dnq_error(ret, "pthread_kill error");
        return NULL;
    }

    free(task);
    return ret;
}

