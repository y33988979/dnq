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
#include "dnq_lcd.h"
#include "dnq_mcu.h"

typedef enum heater_status
{
    WAIT_LOW_LIMIT,
    WAIT_HIGH_LIMIT,
    CLOSE_STATUS,
    
}heater_status_e;

dnq_appinfo_t *manage_appinfo = NULL;

S32 send_msg_to_manage(dnq_msg_t *msg)
{
    S32 ret;
    dnq_queue_t *queue = NULL;
        
    queue = manage_appinfo->queue;
    ret = dnq_msg_send(queue, msg);

    return ret;
}

timesetting_t* dnq_get_room_setting_by_time(U32 room_id, U32 current_time)
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
            return &room_time_setting[i];
    }
    
    return NULL;
}

U16 dnq_get_room_temp_error(U32 room_id)
{
    S32 ret;
    U16 temp_error;
    server_temp_error_t *temp_error_config;

    temp_error_config = dnq_get_temp_error_config();
    temp_error = temp_error_config->rooms[room_id].error;

    return temp_error;
}


S32 dnq_proc()
{
    S32 ret;
    S32 room_id;
    datetime_t  datetime  = {0};
    U32 current_second;
    U32 current_temp;
    U32 setting_temp;
    U16 temp_error;
    server_temp_policy_t *temp_policy_config;
    room_temp_policy_t   *rooms_policy;
    timesetting_t     *current_setting;
    
    temp_policy_config = dnq_get_temp_policy_config();
    rooms_policy = temp_policy_config->rooms;

    temp_error = dnq_get_room_temp_error(room_id);
    
    current_second = dnq_current_time();
    //current_second = datetime.hour*3600+datetime.minute*60+datetime.second;

    heater_status_e status = CLOSE_STATUS;
    
    /* Traversal all rooms */
    for(room_id=0; room_id<DNQ_ROOM_CNT; room_id++)
    {
        current_temp = dnq_get_room_temperature(room_id);
        current_setting = dnq_get_room_setting_by_time(room_id, current_second);
        setting_temp = current_setting->degrees;

        /*
        * if current temp is 0, or cannot find configure in current time, heater should close! 
        * 如果没有找到对应配置，或对应配置温度等于0，则关闭heater，进入close status 
        */
        if(current_setting == NULL || setting_temp == 0)
        {
            dnq_heater_close(room_id);
            status = CLOSE_STATUS;
            continue;
        }
    
        switch(status)
        {
            case WAIT_LOW_LIMIT:
                /* 
                * check until the temprature falls limit 
                * 等待温度下降到设定的温度回差处
                */
                if(current_temp <= setting_temp - temp_error)
                {
                    dnq_heater_ctrl_single(room_id, HEATER_MODE_SWITCH, HEATER_OPEN);
                    dnq_heater_open(room_id);
                    status = WAIT_HIGH_LIMIT;
                }
                
                break;
            case WAIT_HIGH_LIMIT:
                /* 
                * check until the temprature rise limit 
                * 等待温度上升到设定的温度
                */
                if(current_temp >= setting_temp - temp_error)
                {
                    dnq_heater_ctrl_single(room_id, HEATER_MODE_SWITCH, HEATER_CLOSE);
                    dnq_heater_close(room_id);
                    status = WAIT_LOW_LIMIT;
                }

                break;
            case CLOSE_STATUS:
                /* 
                * check until current time is within the configuration range
                * 检查当前时刻是否在用户配置范围内
                */
                
                /* 需要开启heater */
                if(setting_temp >= current_temp)
                {
                    dnq_heater_open(room_id);
                    status = WAIT_HIGH_LIMIT;
                }

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
    dnq_appinfo_t *appinfo = *(dnq_appinfo_t**)args;
    
    manage_queue = appinfo->queue;
    
    while(1)
    {

        ret = dnq_msg_recv_timeout(manage_queue, pRecvMsg, 1000);
        if(ret < 0)
        {
            //dnq_proc();
            continue;
        }

        switch(pRecvMsg->Class)
        {
            case DNQ_CONFIG_UPDATE:

                send_msg_to_lcd();
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
    
    *appinfo = dnq_app_task_create("dnq_manage", 64*2048,\
        QUEUE_MSG_SIZE, QUEUE_SIZE_MAX, manage_task, appinfo);
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


