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
#include "dnq_manage.h"
#include "ngx_palloc.h"

dnq_appinfo_t *manage_appinfo = NULL;

S32 send_msg_to_manage(dnq_msg_t *msg)
{
    S32 ret;
    dnq_queue_t *queue = NULL;
        
    queue = manage_appinfo->queue;
    ret = dnq_msg_send(queue, msg);

    return ret;
}

void *dnq_manage_task(void *args)
{
    S32  ret;
    S32  status;
    dnq_queue_t *manage_queue;
    dnq_msg_t  sendMsg;
    dnq_msg_t  recvMsg;
    dnq_msg_t *pSendMsg = &sendMsg;
    dnq_msg_t *pRecvMsg = &recvMsg;
    
    manage_queue = (dnq_queue_t*)manage_appinfo->queue;
    
    while(1)
    {
        ret = dnq_msg_recv_timeout(manage_queue, pRecvMsg, 1000);
        if(ret < 0)
        {
            continue;
        }

        /*  */
        
    }
}

S32 dnq_manage_init()
{
    dnq_appinfo_t **appinfo = &manage_appinfo;
    *appinfo = dnq_app_task_create("dnq_manage", 2048*32,\
        QUEUE_MSG_SIZE, QUEUE_SIZE_MAX, dnq_manage_task, NULL);
    if(!*appinfo)
    {
        DNQ_ERROR(DNQ_MOD_LCD, "dnq_manage_task create error!");
        return -1;
    }
    
    DNQ_INFO(DNQ_MOD_LCD, "dnq_mgr_init ok!");
    return 0;
}

S32 dnq_manage_deinit()
{

}


