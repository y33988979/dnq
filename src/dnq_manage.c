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
#include "dnq_config.h"
#include "ngx_palloc.h"
#include "dnq_manage.h"
#include "dnq_rabbitmq.h"
#include "dnq_log.h"
#include "dnq_lcd.h"
#include "dnq_mcu.h"
#include <iconv.h>

typedef enum heater_status
{
    CLOSE_STATUS,
    WAIT_LOW_LIMIT,
    WAIT_HIGH_LIMIT
    
}heater_status_e;

dnq_appinfo_t *manage_appinfo = NULL;


#define OUTLEN 255 
 
//代码转换:从一种编码转为另一种编码 
int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen) 
{ 
    iconv_t cd; 
    int rc; 
    char **pin = &inbuf; 
    char **pout = &outbuf; 
 
    cd = iconv_open(to_charset, from_charset); 
    if (cd==0) return -1; 
    memset(outbuf, 0, outlen); 
    if (iconv(cd, pin, &inlen, pout, &outlen)==-1) return -1; 
    iconv_close(cd); 
    return 0; 
} 
 
//UNICODE码转为GB2312码 
int u2g(char *inbuf,int inlen,char *outbuf,int outlen) 
{ 
    return code_convert("utf-8", "gb2312", inbuf, inlen, outbuf, outlen); 
} 
 
//GB2312码转为UNICODE码 
int g2u(char *inbuf,size_t inlen,char *outbuf,size_t outlen) 
{ 
    return code_convert("gb2312", "utf-8", inbuf, inlen, outbuf, outlen); 
} 
 
S32 iconv_test() 
{ 
    char *in_utf8 = "中文, 汉字"; 
    char in_gb2312[64] = {0x43, 0x43, 0x54, 0x56, 0x2d, 0x31, 0x20, 0xd7, 0xdb, 0xba, 0xcf}; // CCTV-1 综合
    char out[OUTLEN]; 
    int rc;
 
    //unicode码转为gb2312码 
    rc = u2g(in_utf8, strlen(in_utf8),out,OUTLEN); 
    printf("unicode-->gb2312 out=%s\n",out); 
    //gb2312码转为unicode码 
    rc = g2u(in_gb2312, strlen(in_gb2312),out,OUTLEN); 
    printf("gb2312-->unicode out=%s\n",out); 
    return 0;
} 

S32 send_msg_to_manage(dnq_msg_t *msg)
{
    S32 ret;
    dnq_queue_t *queue = NULL;
        
    queue = manage_appinfo->queue;
    ret = dnq_msg_send(queue, msg);

    return ret;
}

U16 dnq_get_room_temp_error(U32 room_id)
{
    S32 ret;
    U16 temp_error;
    error_config_t *temp_error_config;

    temp_error_config = dnq_get_temp_error_config(NULL);
    temp_error = temp_error_config->rooms[room_id].error;

    return temp_error;
}

static S32 heater_work_status_update(U32 room_id, U32 status)
{
    dnq_msg_t msg = {0};
    room_item_t *room = dnq_get_room_item(room_id);
    
    if(room->work_status == status)
        return 0;
    
    msg.Class = MSG_CLASS_MANAGE;
    msg.code = room_id;               /* room's id */
    msg.lenght = ROOM_ITEM_WORK_STATUS; /* room's item id */
    msg.payload = (void*)status;     /* work status */

    DNQ_DEBUG(DNQ_MOD_MANAGE, "room_id=%d, work_status=%d!\n", room_id, status);
    /* update room's current temperature */
    return send_msg_to_lcd(&msg);
}

S32 dnq_proc()
{
    S32 ret;
    S32 room_id;
    datetime_t  datetime  = {0};
    U32 current_second;
    S32 current_temp;
    S32 setting_temp;
    S16 temp_error;
    S32 reach_temp_limit = 0;
    error_config_t  *error_config;
    policy_config_t *policy_config;
    room_temp_policy_t   *rooms_policy;
    limit_config_t    *limit_config;
    timesetting_t     *current_setting;
    room_item_t *rooms = dnq_get_rooms();
    static U32 status[DNQ_ROOM_MAX] = {0};

    error_config = dnq_get_temp_error_config(NULL);
    policy_config = dnq_get_temp_policy_config(NULL);
    limit_config = dnq_get_temp_limit_config(NULL);
    rooms_policy = policy_config->rooms;

    /* 获取当前时间，单位是秒 */
    current_second = dnq_get_current_second();
    //current_second = datetime.hour*3600+datetime.minute*60+datetime.second;

    /* Traversal all rooms */
    for(room_id=0; room_id<DNQ_ROOM_MAX; room_id++)
    {
        /* check sensor */
        if(rooms[room_id].sn_status == STOP_STATUS)
            continue;
        
        temp_error = error_config->rooms[room_id].error*100;
        current_temp = rooms[room_id].curr_temp + rooms[room_id].correct*100;
        //printf("room_id=%d, current_temp====%d\n",room_id, current_temp);
        current_setting = dnq_get_room_setting_by_time(room_id, current_second);

        /*
        * if current temp is 0, or cannot find configure in current time, heater should close! 
        * 如果没有找到对应配置，或对应配置温度等于0，则关闭heater，进入close status 
        */
        if(current_setting == NULL)
        {
            DNQ_DEBUG(DNQ_MOD_MANAGE, \
                "room[%d] found not temp policy! close the hearter", room_id);
            dnq_heater_close(room_id);
            heater_work_status_update(room_id, STOP_STATUS);
            status[room_id] = CLOSE_STATUS;
            continue;
        }

        setting_temp = current_setting->degrees*100;
        setting_temp = rooms[room_id].set_temp;
        DNQ_DEBUG(DNQ_MOD_MANAGE, "found temp policy!! id=%d, current=%d'C, set=%d'C, error=%d",
            room_id, current_temp, setting_temp, temp_error);

        switch(status[room_id])
        {
            case WAIT_LOW_LIMIT:
                
                /* 
                * check if the temprature falls limit 
                * 检查温度是否已经下降到设定的低温极限
                */
                reach_temp_limit = 0;
                if(current_temp <= DNQ_TEMP_MIN \
                    || current_temp <= limit_config->rooms[room_id].min*100)
                {
                    reach_temp_limit = 1;
                    DNQ_WARN(DNQ_MOD_MANAGE, \
                    "room[%d]'s temprature[%d] is too low, lowest is %d|%d. force open heater!", \
                    room_id, current_temp, limit_config->rooms[room_id].min*100, DNQ_TEMP_MIN);
                }
                /* 
                * check until the temprature falls error 
                * 等待温度下降到设定的温度回差处
                */
                else if(current_temp <= setting_temp - temp_error)
                {   
                    reach_temp_limit = 1;
                    
                    DNQ_DEBUG(DNQ_MOD_MANAGE, \
                    "low_limit check:room[%d]'s temprature[%d] setting_temp=%d, temp_error=%d", \
                    room_id, current_temp, setting_temp, temp_error);
                }

                /* 到达了温度临界点，需要改变电暖气工作状态 */
                if(reach_temp_limit)
                {
                    dnq_heater_open(room_id);
                    heater_work_status_update(room_id, WORK_STATUS);
                    status[room_id] = WAIT_HIGH_LIMIT;
                }
                
                break;
            case WAIT_HIGH_LIMIT:
                
                /* 
                * check if the temprature falls limit 
                * 检查温度是否已经下降到设定的低温极限
                */
                reach_temp_limit = 0;
                if(current_temp >= DNQ_TEMP_MAX \
                    || current_temp >= limit_config->rooms[room_id].max*100)
                {
                    reach_temp_limit = 1;
                    
                    DNQ_WARN(DNQ_MOD_MANAGE, \
                    "room[%d]'s temprature[%d] is too high, highest is %d|%d. force close heater!", \
                    room_id, current_temp, limit_config->rooms[room_id].max*100, DNQ_TEMP_MAX);
                }
                /* 
                * check until the temprature rise limit 
                * 等待温度上升到设定的温度
                */
                else if(current_temp >= setting_temp \
                    || current_temp >= DNQ_TEMP_MAX)
                {
                    reach_temp_limit = 1;  
                    
                    DNQ_DEBUG(DNQ_MOD_MANAGE, \
                    "high_limit check:room[%d]'s temprature[%d] setting_temp=%d, temp_error=%d", \
                    room_id, current_temp, setting_temp, temp_error);
                }

                /* 到达了温度临界点，需要改变电暖气工作状态 */
                if(reach_temp_limit)
                {
                    dnq_heater_close(room_id);
                    heater_work_status_update(room_id, STOP_STATUS);
                    status[room_id] = WAIT_LOW_LIMIT;
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
                    heater_work_status_update(room_id, WORK_STATUS);
                    status[room_id] = WAIT_HIGH_LIMIT;
                }

            default:
            break;
        }
        //dnq_msleep(100);
    }
}

S32 dnq_lcd_init_info_sync()
{
    S32 i = 0;
    S32 j;
    U32 room_id = 0;
    U8  gb2312_out[32] = {0};
    dnq_msg_t msg = {0};
    init_info_t *init_config;
    room_item_t *rooms = dnq_get_rooms();

    init_config = dnq_get_init_config(NULL);
        
    for(i=0; i<init_config->rooms_cnt; i++)
    {
        room_id = init_config->rooms[i].room_order;        
        rooms[i].id = init_config->rooms[i].room_order;   
        strncpy(rooms[i].name, init_config->rooms[i].room_name, SIZE_16);
        
        //utf8 -> gb2312
        u2g(init_config->rooms[i].room_name, SIZE_16, gb2312_out,sizeof(gb2312_out));   
        strncpy(rooms[i].name, gb2312_out, SIZE_16);
        DNQ_INFO(DNQ_MOD_MANAGE, "[%d]:room_id=%d,room_name='%s'", \
            i,room_id, init_config->rooms[i].room_name);
        
        //rooms[i].correct = init_config->rooms[i].correct;
    }

    msg.Class = MSG_CLASS_MANAGE;
    msg.code = 0x100; /* update init info !*/
    send_msg_to_lcd(&msg);
    
}

/* 每次开机要和rabbitmq服务器通信，索要主机初始化的信息，如:
各房间名称，主机位置，当前时间等，主要用来初始化lcd屏幕，展示信息 */
S32 dnq_init_info_update()
{ 
    S32 ret;
    U32 timeout = 4;
    U8 *ptr = NULL;
    U8  time_string[32] = {0};
    datetime_t datetime;
    init_info_t *init_info;
    
    init_info = dnq_get_init_config(NULL);
    
    /* 向服务器发送初始化请求 */
    ret = send_init_request_to_server();

    
    /* 等待服务器回传完成 */
    timeout = 3;
    while(!init_info_is_ok() && timeout--)
    {
        DNQ_INFO(DNQ_MOD_MANAGE, "waiting server reply...");
        dnq_msleep(500);
    }
        
    /* 未能获取到初始化信息，则使用之前本地存储的信息 */
    if(!init_info_is_ok())
    {
        DNQ_ERROR(DNQ_MOD_MANAGE, "can't get host init_info! use local config.");
    }

    /* 云端回传的时间为字符串格式  "2017-06-04 18:14:18" */    
    memcpy(time_string, init_info->time, sizeof(time_string));

    /* 时间字符串转换成datetime_t结构 */
    dnq_timestr_to_datetime(time_string, &datetime);
    /* 同步网络时间到rtc芯片 */
    dnq_rtc_datetime_sync(&datetime);
    /* 通知lcd模块，初始化屏幕信息 */
    dnq_lcd_init_info_sync();
}

void *manage_task(void *args)
{
    S32  ret;
    S32  status;
    U32  second = 0;
    dnq_queue_t *manage_queue;
    dnq_msg_t  sendMsg;
    dnq_msg_t  recvMsg;
    dnq_msg_t *pSendMsg = &sendMsg;
    dnq_msg_t *pRecvMsg = &recvMsg;
    policy_config_t *policy_config;
    dnq_appinfo_t *appinfo;
    
    appinfo = (dnq_appinfo_t*)args;
    manage_queue = appinfo->queue;

    while(1)
    {
        ret = dnq_msg_recv_timeout(manage_queue, pRecvMsg, 1000);
        if(ret < 0)
        {
            dnq_proc();
            second++;
            if(second%300 == 0) /* five minutes */
            {
                send_room_status_to_server();
            }
            continue;
        }

        switch(pRecvMsg->Class)
        {
            case MSG_CLASS_LCD:
                DNQ_INFO(DNQ_MOD_MANAGE, "recv lcd msg!"); 
                break;
            case MSG_CLASS_RABBITMQ:
                DNQ_INFO(DNQ_MOD_MANAGE, "recv rabbitmq msg!");
                dnq_init_info_update();
                break;
            default:
                DNQ_ERROR(DNQ_MOD_MANAGE, "unknow msg type=%d!", pRecvMsg->Class);
            break;
        }

        /*  */
        
    }
}

S32 dnq_manage_init()
{
    dnq_appinfo_t **appinfo = &manage_appinfo;

    dnq_debug_setlever(DNQ_MOD_RABBITMQ, 5);

    *appinfo = dnq_app_task_create("dnq_manage", 64*2048,\
        QUEUE_MSG_SIZE, QUEUE_SIZE_MAX, manage_task, NULL);
    if(!*appinfo)
    {
        DNQ_ERROR(DNQ_MOD_MANAGE, "manage_task create error!");
        return -1;
    }
        
    DNQ_INFO(DNQ_MOD_MANAGE, "dnq_manage_init ok!");
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


