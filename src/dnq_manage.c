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
#include "dnq_mcu.h"

dnq_appinfo_t *manage_appinfo = NULL;

S32 send_msg_to_manage(dnq_msg_t *msg)
{
    S32 ret;
    dnq_queue_t *queue = NULL;
        
    queue = manage_appinfo->queue;
    ret = dnq_msg_send(queue, msg);

    return ret;
}

S32 dnq_room_policy_check(U32 room_id, U32 current_time)
{
    S32 ret;
    U32 i;
    U32 start_time, end_time;
    room_temp_policy_t *room_policy;
    server_temp_policy_t *temp_policy;
    timesetting_t  *room_time_setting;

    temp_policy = dnq_get_temp_policy_config();
    room_policy = &temp_policy->rooms[room_id];
    room_time_setting = room_policy->time_setting;

    for(i=0; i<room_policy->time_setting_cnt; i++)
    {
        if(current_time >= room_time_setting[i].start
        && current_time <= room_time_setting[i].end)
            return i;
    }
    
    return -1;
}

S32 dnq_proc()
{
    S32 i,j, ret;
    U8  datetime[8] = {0};
    U32 current_time;
    U32 current_temp;
    server_temp_policy_t *temp_policy;
    room_temp_policy_t   *rooms_policy;
    timesetting_t     *time_setting;
    
    temp_policy = dnq_get_temp_policy_config();
    rooms_policy = temp_policy->rooms;

    dnq_rtc_get_time(datetime);
    

    current_time;

    /* Traversal all rooms */
    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        dnq_room_get_temperature(i);
        ret = dnq_room_policy_check(i, current_time);
        if(ret >= 0)
        {
            rooms_policy[i].time_setting[ret].degrees;
            dnq_heater_ctrl_single(i, HEATER_MODE_SWITCH, HEATER_OPEN);
        }
        
        for(j=0; j<rooms_policy[i].time_setting_cnt; j++)
        {

        }

typedef enum heater_status
{
    WAIT_LOW_LIMIT,
    WAIT_HIGH_LIMIT,
    
}heater_status_e;

        heater_status_e status = WAIT_LOW_LIMIT;
        current_temp = dnq_room_get_temperature(i);
        switch(status)
        {
            case WAIT_LOW_LIMIT:
                /* 
                * check until the temprature falls limit 
                * 等待温度下降到设定的温度回差处
                */

                if(current_temp)

                dnq_heater_ctrl_single(i, HEATER_MODE_SWITCH, HEATER_OPEN);
                dnq_heater_open(i);
                status = WAIT_HIGH_LIMIT;
                break;
            case WAIT_HIGH_LIMIT:
                /* 
                * check until the temprature rise limit 
                * 等待温度上升到设定的温度
                */

                dnq_heater_ctrl_single(i, HEATER_MODE_SWITCH, HEATER_CLOSE);
                dnq_heater_close(i);
                status = WAIT_LOW_LIMIT;
                break;
            default:
            break;
        }
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


