/* dnq operation manage Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  this is a operation manage source , for dnq.
 * Note : 
 */


#include "dnq_common.h"
#include "ngx_palloc.h"
#include "dnq_manage.h"
#include "dnq_rabbitmq.h"
#include "dnq_log.h"

dnq_appinfo_t *manage_appinfo = NULL;

S32 send_msg_to_manage(dnq_msg_t *msg)
{
    S32 ret;
    dnq_queue_t *queue = NULL;
        
    queue = manage_appinfo->queue;
    ret = dnq_msg_send(queue, msg);

    return ret;
}

S32 dnq_proc()
{
    S32 i,j;
    server_temp_policy_t *temp_policy;
    room_temp_policy_t   *rooms_policy;
    
    temp_policy = dnq_get_temp_policy_config();
    rooms_policy = temp_policy->rooms;

    /* Traversal all rooms */
    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        
    }
}

void *manage_task(void *args)
{
    S32  ret;
    S32  status;
    dnq_queue_t *manage_queue;
    dnq_msg_t  sendMsg;
    dnq_msg_t  recvMsg;
    dnq_msg_t *pSendMsg = &sendMsg;
    dnq_msg_t *pRecvMsg = &recvMsg;
    server_temp_policy_t *temp_policy;
    
    manage_queue = (dnq_queue_t*)manage_appinfo->queue;
    
    while(1)
    {
        ret = dnq_msg_recv_timeout(manage_queue, pRecvMsg, 1000);
        if(ret < 0)
        {
            continue;
        }

        switch(pRecvMsg->code)
        {
            case DNQ_CONFIG_UPDATE:

                
                
                break;

            default:
            break;
        }

        

        /*  */
        
    }
}

S32 dnq_manage_init()
{
    dnq_appinfo_t **appinfo = &manage_appinfo;
    *appinfo = dnq_app_task_create("dnq_manage", 2048*32,\
        QUEUE_MSG_SIZE, QUEUE_SIZE_MAX, manage_task, NULL);
    if(!*appinfo)
    {
        DNQ_ERROR(DNQ_MOD_LCD, "manage_task create error!");
        return -1;
    }
    
    DNQ_INFO(DNQ_MOD_LCD, "dnq_manage_init ok!");
    return 0;
}

S32 dnq_manage_deinit()
{
    S32 ret;

    if(!manage_appinfo)
        return -1;
    ret = dnq_app_task_exit(manage_appinfo);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_MANAGE, "manage_task exit error!");
        return -1;
    }
    
    DNQ_INFO(DNQ_MOD_MANAGE, "manage_deinit ok!");
    return ret;
}


