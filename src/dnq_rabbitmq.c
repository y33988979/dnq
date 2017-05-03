/* dnq rabbitmq Program
 *
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 *
 *  this is a rabbitmq interface API, for app.
 * Note :
 */

#include "common.h"
#include "dnq_log.h"
#include "dnq_rabbitmq.h"

#include "cJSON.h"
#include "ngx_palloc.h"

#include <amqp.h>
#include <amqp_framing.h>
#include <amqp_tcp_socket.h>

typedef struct channel_t{
	int chid;
	char qname[64];
	char exchange[64];
	char rtkey[64];
}channel_t;

channel_t channels1[18] = {
	{1,  "_authorization",      "exchange_authorization",   ""},
	{2,  "_control",            "exchange_control",         ""},
	{3,  "_limit",              "exchange_limit",           ""},
	{4,  "_adjust",             "exchange_adjust",          ""},
	{5,  "_degreeerror",        "exchange_degreeerror",     ""},
	{6,  "_power",              "exchange_power",           ""},
	{7,  "_response",           "exchange_response",        ""},
	{8,  "_rectify",           "exchange_rectify",        ""},
	{9,  "_timecontrol",           "exchange_timecontrol",        ""},
	{10,  "queue_cloud_degree",       "exchange_cloud_degree",    "state"},
	{11,  "queue_cloud_heartbeat",    "exchange_cloud_heartbeat", "heartbeat"},
	{12, "queue_cloud_callback",     "exchange_cloud_callback",  "callback"},
	{13, "queue_cloud_warn",         "exchange_cloud_warn",      "warn"},
};

channel_t channels[18] = {
	{1,  "queue_host_",    "exchange_host",       ""},
	{2,  "state_",         "exchange_cloud",      "state"},
	{3,  "response_",      "exchange_cloud",      "callback"},
	{4,  "config_",        "exchange_cloud",      "config"},
	{5,  "warn_",          "exchange_cloud",      "warn"},
	{7,  "_power",         "exchange_power",       ""}
};

#if 1
static char *serverip = "112.74.43.136";
static int   serverport = 5672;
static char *username = "host001";
static char *password = "123456";
amqp_connection_state_t  g_conn;
#else
static char *serverip = "123.56.231.195";
static int   serverport = 5672;
static char *username = "control001";
static char *password = "123456";
#endif

#define SERVER_IPADDR   "112.74.43.136"
#define SERVER_PORT     5672

#if 0
#define MAC_ADDR   "080027000192"
#else
#define MAC_ADDR   "70b3d5cf4924"
#endif


#define YLOG  printf

/*
* message type define
* 消息类型字段  枚举
*/

typedef enum json_type
{
    MSG_TYPE_AUTHORRIZATION,
    MSG_TYPE_TEMP_POLICY,
    MSG_TYPE_TEMP_LIMIT,
    MSG_TYPE_DEGREE_ERROR,
    MSG_TYPE_POWER_CONFIG,
    MSG_TYPE_RESPONSE,
    MSG_TYPE_CORRECT,
    MSG_TYPE_INIT
}json_type_e;

/*
* message type string define
* 消息类型字段  字符串
*/
#define TYPE_STR_AUTHORRIZATION   "authorization"
#define TYPE_STR_TEMP_POLICY      "policy" //"temp_policy"
#define TYPE_STR_TEMP_LIMIT       "limit"
#define TYPE_STR_DEGREE_ERROR     "error"
#define TYPE_STR_POWER_CONFIG     "power"
#define TYPE_STR_RESPONSE         "response"
#define TYPE_STR_CORRECT          "rectify"
#define TYPE_STR_INIT             "init"

/*
* json item define
* json数据item  字符串
*/
#define JSON_ITEM_TYPE            "type"
#define JSON_ITEM_TIME            "time"
#define JSON_ITEM_CONTROLLERID    "id"
#define JSON_ITEM_AUTHORRIZATION  "authorization"
#define JSON_ITEM_STATUS          "status"
#define JSON_ITEM_MODE            "mode"
#define JSON_ITEM_ROOMS           "rooms"
#define JSON_ITEM_ROOMID          "id" //"roomid"
#define JSON_ITEM_DPID            "dpid"
#define JSON_ITEM_TIMESETTING_CNT "timesetting_cnt"
#define JSON_ITEM_TIMESETTINGS    "timesettings"
#define JSON_ITEM_START_TIME      "starttime"
#define JSON_ITEM_END_TIME        "endtime"
#define JSON_ITEM_DEGREES         "degrees"
#define JSON_ITEM_MAX             "max"
#define JSON_ITEM_MIN             "min"
#define JSON_ITEM_ERROR           "error"
#define JSON_ITEM_CONFIGS         "configs"
#define JSON_ITEM_POWER           "power"
#define JSON_ITEM_NUM             "num"
#define JSON_ITEM_RECTIFY         "rectify"
#define JSON_ITEM_CORRECT         "correct"
#define JSON_ITEM_ROOM_CNT        "room_cnt"

#define JSON_ITEM_PARTITION       "partition"
#define JSON_ITEM_NO              "no"
#define JSON_ITEM_MEMO            "memo"
#define JSON_ITEM_NAME            "name"
#define JSON_ITEM_IS_DELETE       "isDelete"
#define JSON_ITEM_PROJECT_ID      "projectId"
#define JSON_ITEM_BUILDING_ID     "buildingId"
#define JSON_ITEM_EQUIPMENT_ID    "equipmentId"
#define JSON_ITEM_PROJECT_NAME    "projectName"
#define JSON_ITEM_BUILDING_NAME   "buildingName"
#define JSON_ITEM_EQUIPMENT_MAC   "equipmentMac"

#define JSON_ITEM_ID              "id"
#define JSON_ITEM_ROOM_NAME       "roomName"
#define JSON_ITEM_ROOM_ID         "roomId"
#define JSON_ITEM_ROOM_ORDER      "roomOrder"
#define JSON_ITEM_ROOM_FLOOR      "roomFloor"
#define JSON_ITEM_ROOM_POSITION   "position"

/*
* json config define
* json配置文件 文件名称定义
*/
#define SERVER_CFG_FILE_AUTHORRIZATION  "server_authorization.json"
#define SERVER_CFG_FILE_POLICY          "server_policy.json"
#define SERVER_CFG_FILE_LIMIT           "server_limit.json"
#define SERVER_CFG_FILE_ERROR           "server_error.json"
#define SERVER_CFG_FILE_POWER           "server_power.json"
#define SERVER_CFG_FILE_RESPONSE        "server_response.json"
#define SERVER_CFG_FILE_CORRECT         "server_correct.json"

#define CLIENT_CFG_FILE_STATUS          "client_status.json"
#define CLIENT_CFG_FILE_LOSS            "client_loss.json"
#define CLIENT_CFG_FILE_RESPONSE        "client_response.json"
#define CLIENT_CFG_FILE_CONFIG          "client_config.json"
#define CLIENT_CFG_FILE_WARN            "client_warn.json"

#define copy_json_item_to_struct_item(obj, json, item_name, item_addr, item_type)  \
    do{\
        obj = cJSON_GetObjectItem(json, item_name);\
        if(!obj)\
        {\
            DNQ_WARN(DNQ_MOD_RABBITMQ, "item %s not found!", item_name);\
            break;\
        }\
        if(obj->type != item_type)\
        {\
            DNQ_WARN(DNQ_MOD_RABBITMQ, "item %s type[%d] error, should be [%d]!", \
            item_name, obj->type, item_type);\
            break;\
        }\
        if(item_type == cJSON_String)\
        {\
            strcpy((char*)item_addr, obj->valuestring); \
            break;\
        }\
        else if(item_type == cJSON_Number)\
        {\
            *(int*)item_addr = obj->valueint;\
            break;\
        }\
    }while(0);\
    obj = NULL;
    
/*
* server --> controller
* server/cloud send message to controller
*
* 云端向控制器发消息, 云端 --> 控制器
*
*/

/*
*  server --> controller
*  parse a "authorization manage" message (json data)
*
*  2.1 云端向控制器发送 <控制请求授权管理>
*  解析一个 "授权管理" 消息结构(json)
*
*/

int json_parse_authorization_manage(cJSON *pjson, server_authorization_t *pdst)
{
    cJSON  *obj = NULL;

    //type
    strcpy(pdst->type, "authorization");

    //time
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_TIME, pdst->time, cJSON_String);
    DNQ_INFO(DNQ_MOD_RABBITMQ, "time:\t%s", pdst->time);

    //authorization
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_AUTHORRIZATION, pdst->authorization, cJSON_String);
    DNQ_INFO(DNQ_MOD_RABBITMQ, "authorization:\t%s", pdst->authorization);

    return 0;
}

/*
*  server --> controller
*  parse a "temperature policy" message (json data)
*
*  2.2 云端向控制器发送 <温度策略>
*  解析一个 "温度策略" 消息结构(json)
*
*/

int json_parse_temp_policy(cJSON *pjson, server_temp_policy_t *pdst)
{
	int     i = 0;
    int     j = 0;
    cJSON  *obj = NULL;
    cJSON  *rooms = NULL;
    cJSON  *room_obj = NULL;
    cJSON  *timesettings = NULL;
    cJSON  *timesettings_obj = NULL;

    //type
    strcpy(pdst->type, "policy");

    //time
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_TIME, pdst->time, cJSON_String);
    DNQ_INFO(DNQ_MOD_RABBITMQ, "time:\t%s", pdst->time);

    //controllerid
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_CONTROLLERID, pdst->ctrl_id, cJSON_String);
    DNQ_INFO(DNQ_MOD_RABBITMQ, "id:\t%s", pdst->ctrl_id);

    //mode
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_MODE, &pdst->mode, cJSON_Number);
    DNQ_INFO(DNQ_MOD_RABBITMQ, "mode:\t%d", pdst->mode);

    //room count
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_ROOM_CNT, &pdst->rooms_cnt, cJSON_Number);
    DNQ_INFO(DNQ_MOD_RABBITMQ, "room_cnt:\t%d", pdst->rooms_cnt);

    //rooms
    rooms = cJSON_GetObjectItem(pjson, JSON_ITEM_ROOMS);
    if(!cJSON_IsArray(rooms))
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "item %s must be a array!", JSON_ITEM_TIMESETTINGS);
        return -1;
    }

    //rooms size
    pdst->rooms_cnt = cJSON_GetArraySize(rooms);
    DNQ_INFO(DNQ_MOD_RABBITMQ, "%s array's size=%d!", JSON_ITEM_ROOMS, pdst->rooms_cnt);

    //rooms array
    for(i=0; i<pdst->rooms_cnt; i++)
    {
        //item from rooms array
        room_obj = cJSON_GetArrayItem(rooms, i);

        //room ID
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ROOMID, &pdst->rooms[i].room_id, cJSON_Number);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "room_id=%d!", pdst->rooms[i].room_id);

        //dpid ID
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_DPID, &pdst->rooms[i].dpid, cJSON_Number);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "dpid=%d!", pdst->rooms[i].dpid);

        //timesetting_cnt
        //copy_json_item_to_struct_item(\
        //obj, room_obj, JSON_ITEM_TIMESETTING_CNT, &pdst->rooms[i].time_setting_cnt, cJSON_Number);
        //DNQ_INFO(DNQ_MOD_RABBITMQ, "time_setting_cnt=%d!", pdst->rooms[i].time_setting_cnt);

        //timesetting array
        timesettings= cJSON_GetObjectItem(room_obj, JSON_ITEM_TIMESETTINGS);
        if(!cJSON_IsArray(timesettings))
        {
            DNQ_ERROR(DNQ_MOD_RABBITMQ, "item %s must be a array!", JSON_ITEM_TIMESETTINGS);
            return -1;
        }

        //timesetting array size
        pdst->rooms[i].time_setting_cnt = cJSON_GetArraySize(timesettings);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "%s timesettings's size=%d!", JSON_ITEM_ROOMS, pdst->rooms[i].time_setting_cnt);

        //item from time_setting array
        for(j=0; j<pdst->rooms[i].time_setting_cnt; j++)
        {
            //item from array
            timesettings_obj = cJSON_GetArrayItem(timesettings, i);

            //start time
            copy_json_item_to_struct_item(\
                obj, timesettings_obj, JSON_ITEM_START_TIME, pdst->rooms[i].time_setting[j].starttime, cJSON_String);
            DNQ_INFO(DNQ_MOD_RABBITMQ, "starttime:\t%s!", pdst->rooms[i].time_setting[j].starttime);
            //end time
            copy_json_item_to_struct_item(\
                obj, timesettings_obj, JSON_ITEM_END_TIME, pdst->rooms[i].time_setting[j].endtime, cJSON_String);
            DNQ_INFO(DNQ_MOD_RABBITMQ, "endtime:\t%s!", pdst->rooms[i].time_setting[j].endtime);
            //degrees
            copy_json_item_to_struct_item(\
                obj, timesettings_obj, JSON_ITEM_DEGREES, pdst->rooms[i].time_setting[j].degrees, cJSON_String);
            DNQ_INFO(DNQ_MOD_RABBITMQ, "degrees:\t%s!", pdst->rooms[i].time_setting[j].degrees);
        }
    }

    return 0;
}

/*
*  server --> controller
*  parse a "temp_limit" message (json data)
*
*  2.3 云端向控制器发送 <高温限制和低温保护的温度>
*  解析一个 "高低温限制" 消息结构(json)
*
*/
int json_parse_temp_limit(cJSON *pjson, server_temp_limit_t *pdst)
{
    int     i = 0;
    cJSON  *obj = NULL;
    cJSON  *rooms = NULL;
    cJSON  *room_obj = NULL;

    //type
    strcpy(pdst->type, "limit");

    //time
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_TIME, pdst->time, cJSON_String);

    //mode
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_MODE, &pdst->mode, cJSON_Number);

    //rooms
    rooms = cJSON_GetObjectItem(pjson, JSON_ITEM_ROOMS);
    if(!cJSON_IsArray(rooms))
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "item %s must be a array!", JSON_ITEM_ROOMS);
        return -1;
    }

    //rooms size
    pdst->rooms_cnt = cJSON_GetArraySize(rooms);
    DNQ_INFO(DNQ_MOD_RABBITMQ, "%s array's size=%d!", JSON_ITEM_ROOMS, pdst->rooms_cnt);

    //rooms array
    for(i=0; i<pdst->rooms_cnt; i++)
    {
        //item from rooms array
        room_obj = cJSON_GetArrayItem(rooms, i);

        //room ID
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ROOMID, &pdst->rooms[i].room_id, cJSON_Number);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "id:\t%d!", pdst->rooms[i].room_id);

        //max
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_MAX, &pdst->rooms[i].max, cJSON_Number);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "max:\t%d!", pdst->rooms[i].max);

        //min
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_MIN, &pdst->rooms[i].min, cJSON_Number);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "min:\t%d!", pdst->rooms[i].min);
    }

    return 0;
}

/*
*  server --> controller
*  parse a "degree_error" message (json data)
*
*  2.4 云端向控制器发送 <设置温度回差>
*  解析一个 "温度回差" 消息结构(json)
*
*/
int json_parse_degree_error(cJSON * pjson, server_temp_error_t *pdst)
{
    int     i = 0;
    cJSON  *obj = NULL;
    cJSON  *rooms = NULL;
    cJSON  *room_obj = NULL;

    //type
    strcpy(pdst->type, "error");

    //time
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_TIME, pdst->time, cJSON_String);

    //mode
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_MODE, &pdst->mode, cJSON_Number);

    //rooms
    rooms = cJSON_GetObjectItem(pjson, JSON_ITEM_ROOMS);
    if(!cJSON_IsArray(rooms))
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "item %s must be a array!", JSON_ITEM_ROOMS);
        return -1;
    }

    //rooms size
    pdst->rooms_cnt = cJSON_GetArraySize(rooms);
    DNQ_INFO(DNQ_MOD_RABBITMQ, "%s array's size=%d!", JSON_ITEM_ROOMS, pdst->rooms_cnt);

    //rooms array
    for(i=0; i<pdst->rooms_cnt; i++)
    {
        //item from rooms array
        room_obj = cJSON_GetArrayItem(rooms, i);

        //room ID
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ROOMID, &pdst->rooms[i].room_id, cJSON_Number);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "id:\t%d!", pdst->rooms[i].room_id);

        //degree error
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ERROR, &pdst->rooms[i].error, cJSON_Number);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "error:\t%d!", pdst->rooms[i].error);

    }

    return 0;
}

/*
*  server --> controller
*  parse a "power_config" message (json data)
*
*  2.5 云端向控制器发送 <设置每个房间功率配置>
*  解析一个 "功率配置" 消息结构(json)
*
*/
int json_parse_power_config(cJSON *pjson, server_power_config_t *pdst)
{
    int     i = 0;
    int     j = 0;
    cJSON  *obj = NULL;
    cJSON  *rooms = NULL;
    cJSON  *room_obj = NULL;
    cJSON  *configs = NULL;
    cJSON  *config_obj = NULL;

    //type
    strcpy(pdst->type, "power");

    //time
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_TIME, pdst->time, cJSON_String);

    //mode
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_MODE, &pdst->mode, cJSON_Number);

    //rooms
    rooms = cJSON_GetObjectItem(pjson, JSON_ITEM_ROOMS);
    if(!cJSON_IsArray(rooms))
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "item %s must be a array!", JSON_ITEM_ROOMS);
        return -1;
    }

    //rooms size
    pdst->rooms_cnt = cJSON_GetArraySize(rooms);
    DNQ_INFO(DNQ_MOD_RABBITMQ, "%s array's size=%d!", JSON_ITEM_ROOMS, pdst->rooms_cnt);

    //rooms array
    for(i=0; i<pdst->rooms_cnt; i++)
    {
        //item from array
        room_obj = cJSON_GetArrayItem(rooms, i);

        //room ID
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ROOMID, &pdst->rooms[i].room_id, cJSON_Number);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "id:\t%d!", pdst->rooms[i].room_id);

        //configs
        configs = cJSON_GetObjectItem(room_obj, JSON_ITEM_ROOMS);
        if(!cJSON_IsArray(configs))
        {
            DNQ_ERROR(DNQ_MOD_RABBITMQ, "item %s must be a array!", JSON_ITEM_CONFIGS);
            return -1;
        }
        
        //configs size
        pdst->rooms[i].config_cnt = cJSON_GetArraySize(rooms);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "%s array's size=%d!", JSON_ITEM_ROOMS, pdst->rooms[i].config_cnt);

        //item from configs array
        for(j=0; j<pdst->rooms[i].config_cnt; j++)
        {
            //item from array
            config_obj = cJSON_GetArrayItem(configs, i);

            //power
            copy_json_item_to_struct_item(\
                obj, config_obj, JSON_ITEM_POWER, &pdst->rooms[i].power[j], cJSON_Number);
            DNQ_INFO(DNQ_MOD_RABBITMQ, "power:\t%s!", pdst->rooms[i].power[j]);
            //num
            copy_json_item_to_struct_item(\
                obj, config_obj, JSON_ITEM_NUM, &pdst->rooms[i].num[j], cJSON_Number);
            DNQ_INFO(DNQ_MOD_RABBITMQ, "num:\t%s!", pdst->rooms[i].num[j]);
        }
    }
    
    return 0;
}

/*
*  server --> controller
*  parse a "response" message (json data)
*
*  2.6 云端向控制器发送 <云端向控制器发送应答>
*  解析一个 "应答" 消息结构(json)
*
*/
int json_parse_response(cJSON *pjson, server_response_t *pdst)
{
    int     i = 0;
    cJSON  *obj = NULL;

    //type
    strcpy(pdst->type, "response");

    //time
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_TIME, pdst->time, cJSON_String);

    //mode
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_STATUS, &pdst->status, cJSON_String);
    
    return 0;
}

/*
*  server --> controller
*  parse a "rectify" message (json data)
*
*  2.7 云端向控制器发送 <设置房间矫正温度>
*  解析一个 "rectify" 消息结构(json)
*
*/
int json_parse_correct(cJSON *pjson, server_temp_correct_t *pdst)
{
    int     i = 0;
    cJSON  *obj = NULL;
    cJSON  *rooms = NULL;
    cJSON  *room_obj = NULL;

    //type
    strcpy(pdst->type, "correct");

    //time
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_TIME, pdst->time, cJSON_String);

    //mode
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_MODE, &pdst->mode, cJSON_Number);

    //rooms
    rooms = cJSON_GetObjectItem(pjson, JSON_ITEM_ROOMS);
    if(!cJSON_IsArray(rooms))
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "item %s must be a array!", JSON_ITEM_ROOMS);
        return -1;
    }

    //rooms size
    pdst->rooms_cnt = cJSON_GetArraySize(rooms);
    DNQ_INFO(DNQ_MOD_RABBITMQ, "%s array's size=%d!", JSON_ITEM_ROOMS, pdst->rooms_cnt);

    //rooms array
    for(i=0; i<pdst->rooms_cnt; i++)
    {
        //item from rooms array
        room_obj = cJSON_GetArrayItem(rooms, i);

        //room ID
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ROOMID, &pdst->rooms[i].room_id, cJSON_Number);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "id:\t%d!", pdst->rooms[i].room_id);

        //rectify
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_RECTIFY, &pdst->rooms[i].correct, cJSON_Number);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "error:\t%d!", pdst->rooms[i].correct);
    }

    return 0;
}

/*
*  server --> controller
*  parse a "init" message (json data)
*
*  2.8 初始化信息 <房间名称，小区信息，温度等>
*  解析一个 "init" 消息结构(json)
*
*/
int json_parse_init(cJSON *pjson, server_init_info_t *pdst)
{
    int     i = 0;
    cJSON  *obj = NULL;
    cJSON  *rooms = NULL;
    cJSON  *room_obj = NULL;
    cJSON  *partition_obj = NULL;

    //type
    strcpy(pdst->type, "init");

    //partition obj
    partition_obj = cJSON_GetObjectItem(pjson, JSON_ITEM_PARTITION);
    if(!cJSON_IsObject(partition_obj))
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "item %s must be a Object!", JSON_ITEM_PARTITION);
        return -1;
    }

    //partition item
    copy_json_item_to_struct_item(\
    obj, partition_obj, JSON_ITEM_NO, pdst->partition.no, cJSON_String);
    copy_json_item_to_struct_item(\
    obj, partition_obj, JSON_ITEM_ID, &pdst->partition.id, cJSON_Number);
    copy_json_item_to_struct_item(\
    obj, partition_obj, JSON_ITEM_MEMO, pdst->partition.memo, cJSON_String);
    copy_json_item_to_struct_item(\
    obj, partition_obj, JSON_ITEM_NAME, pdst->partition.name, cJSON_String);
    copy_json_item_to_struct_item(\
    obj, partition_obj, JSON_ITEM_IS_DELETE, &pdst->partition.isDelete, cJSON_Number);

    //projectId
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_PROJECT_ID, &pdst->project_id, cJSON_Number);
    
    return 0;
}

/*
*  server --> controller
*  this a parse entry for message (json data)
*  parse the message type, Branch processing
*
*  解析消息的总入口，不同的消息类型，通过分支处理
*
*/
int json_parse(char *json,void *output)
{
    cJSON *pjson = NULL;
    cJSON *item = NULL;
    char  *type = NULL;
    json_type_e  msg_type;

    pjson = cJSON_Parse(json);
    if(!pjson)
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "json parse failed!");
        return -1;
    }

    item = cJSON_GetObjectItem(pjson, "type");
    DNQ_INFO(DNQ_MOD_RABBITMQ, "type=%s", item->valuestring);

    if(cJSON_IsString(item))
    {
        type = item->valuestring;
        if(strcmp(type, TYPE_STR_AUTHORRIZATION) == 0)
        {
            json_parse_authorization_manage(pjson, output);
            msg_type = MSG_TYPE_AUTHORRIZATION;
        }
        else if(strcmp(type, TYPE_STR_TEMP_POLICY) == 0)
        {
            json_parse_temp_policy(pjson, output);
            msg_type = MSG_TYPE_TEMP_POLICY;
        }
        else if(strcmp(type, TYPE_STR_TEMP_LIMIT) == 0)
        {
            json_parse_temp_limit(pjson, output);
            msg_type = MSG_TYPE_TEMP_LIMIT;
        }
        else if(strcmp(type, TYPE_STR_DEGREE_ERROR) == 0)
        {
            json_parse_degree_error(pjson, output);
            msg_type = MSG_TYPE_DEGREE_ERROR;
        }
        else if(strcmp(type, TYPE_STR_POWER_CONFIG) == 0)
        {
            json_parse_power_config(pjson, output);
            msg_type = MSG_TYPE_POWER_CONFIG;
        }
        else if(strcmp(type, TYPE_STR_RESPONSE) == 0)
        {
            json_parse_response(pjson, output);
            msg_type = MSG_TYPE_RESPONSE;
        }
        else if(strcmp(type, TYPE_STR_CORRECT) == 0)
        {
            json_parse_correct(pjson, output);
            msg_type = MSG_TYPE_CORRECT;
        }
        else if(strcmp(type, TYPE_STR_INIT) == 0)
        {
            json_parse_init(pjson, output);
            msg_type = MSG_TYPE_INIT;
        }
        else
        {
            DNQ_ERROR(DNQ_MOD_RABBITMQ, "unkown msg! type=[%s]", item->valuestring);
        }
    }
    else
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "Error: type must be string!");
    }
    cJSON_Delete(pjson);
    
    return msg_type;
}

char *json_create_status(client_status_t *pdst)
{
    int    i = 0;
    cJSON *pjson = NULL;
    cJSON *room_array = NULL;
    cJSON *room_obj = NULL;
    char  *pstr = NULL;
    
    pjson = cJSON_CreateObject();
    if(!pjson)
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "cJSON_CreateObject error!");
        return NULL;
    }

    cJSON_AddStringToObject(pjson, "mac",  pdst->mac);
    for(i=0; i<pdst->rooms_cnt; i++)
    {
        room_array = cJSON_CreateArray();
        
        cJSON_AddItemToObject(pjson, "rooms", room_array);
        room_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(room_obj, "id",  pdst->rooms[i].room_id);
        cJSON_AddNumberToObject(room_obj, "degree",  pdst->rooms[i].value);
        cJSON_AddNumberToObject(room_obj, "loss",  pdst->rooms[i].loss);
        cJSON_AddItemToArray(room_array, room_obj);
    }
    
    pstr = cJSON_Print(pjson);
    if(!pstr)
    {
        cJSON_Delete(pjson);
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "cJSON_Print error!");
        return NULL;
    }

    cJSON_Delete(pjson);
    return pstr;
}

char *json_create_loss(client_loss_t *pdst)
{
    int    i = 0;
    cJSON *pjson = NULL;
    char  *pstr = NULL;
    
    pjson = cJSON_CreateObject();
    if(!pjson)
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "cJSON_CreateObject error!");
        return NULL;
    }

    cJSON_AddStringToObject(pjson, "mac",  pdst->mac);
    for(i=0; i<pdst->rooms_cnt; i++)
    {
        cJSON_AddNumberToObject(pjson, "id",  pdst->rooms[i].room_id);
        cJSON_AddNumberToObject(pjson, "value",  pdst->rooms[i].value);
    }
    
    pstr = cJSON_Print(pjson);
    if(!pstr)
    {
        cJSON_Delete(pjson);
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "cJSON_Print error!");
        return NULL;
    }

    cJSON_Delete(pjson);
    return pstr;
}

char *json_create_response(client_response_t *pdst)
{
    int    i = 0;
    cJSON *pjson = NULL;
    char  *pstr = NULL;
    
    pjson = cJSON_CreateObject();
    if(!pjson)
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "cJSON_CreateObject error!");
        return NULL;
    }

    cJSON_AddStringToObject(pjson, "mac",  pdst->mac);
    cJSON_AddStringToObject(pjson, "status",  pdst->status);

    pstr = cJSON_Print(pjson);
    if(!pstr)
    {
        cJSON_Delete(pjson);
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "cJSON_Print error!");
        return NULL;
    }

    cJSON_Delete(pjson);
    return pstr;
}

char *json_create_config(client_config_t *pdst)
{
    int    i = 0;
    cJSON *pjson = NULL;
    cJSON *room_array = NULL;
    cJSON *room_obj = NULL;
    char  *pstr = NULL;
    
    pjson = cJSON_CreateObject();
    if(!pjson)
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "cJSON_CreateObject error!");
        return NULL;
    }

    cJSON_AddStringToObject(pjson, "mac",  pdst->mac);
    for(i=0; i<pdst->rooms_cnt; i++)
    {
        room_array = cJSON_CreateArray();
        
        cJSON_AddItemToObject(pjson, "rooms", room_array);
        room_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(room_obj, "id",  pdst->rooms[i].room_id);
        cJSON_AddNumberToObject(room_obj, "degreePolicy",  pdst->rooms[i].degreePolicy);
        cJSON_AddNumberToObject(room_obj, "maxDegree",  pdst->rooms[i].maxdegree);
        cJSON_AddNumberToObject(room_obj, "minDegree",  pdst->rooms[i].mindegree);
        cJSON_AddNumberToObject(room_obj, "error",  pdst->rooms[i].error);
        cJSON_AddNumberToObject(room_obj, "correct",  pdst->rooms[i].corrent);
        cJSON_AddNumberToObject(room_obj, "power",  pdst->rooms[i].power);
        cJSON_AddItemToArray(room_array, room_obj);
    }

    pstr = cJSON_Print(pjson);
    if(!pstr)
    {
        cJSON_Delete(pjson);
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "cJSON_Print error!");
        return NULL;
    }

    cJSON_Delete(pjson);
    return pstr;
}

char *json_create_warn(client_warn_t *pdst)
{
    int    i = 0;
    cJSON *pjson = NULL;
    cJSON *room_array = NULL;
    cJSON *room_obj = NULL;
    char  *pstr = NULL;
    
    pjson = cJSON_CreateObject();
    if(!pjson)
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "cJSON_CreateObject error!");
        return NULL;
    }

    cJSON_AddStringToObject(pjson, "mac",  pdst->mac);
    for(i=0; i<pdst->rooms_cnt; i++)
    {
        room_array = cJSON_CreateArray();
        
        cJSON_AddItemToObject(pjson, "rooms", room_array);
        room_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(room_obj, "id",  pdst->rooms[i].room_id);
        cJSON_AddNumberToObject(room_obj, "value",  pdst->rooms[i].value);
        cJSON_AddItemToArray(room_array, room_obj);
    }
    pstr = cJSON_Print(pjson);
    if(!pstr)
    {
        cJSON_Delete(pjson);
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "cJSON_Print error!");
        return NULL;
    }

    cJSON_Delete(pjson);
    return pstr;
}

int send_response_to_server(
    amqp_connection_state_t conn,
    channel_t *pchnl,
    char *response)
{
    static char response_buffer[128];
	amqp_basic_properties_t props;
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	props.content_type = amqp_cstring_bytes("text/plain");
	props.delivery_mode = 2; /* persistent delivery mode */
	//snprintf(response_buffer,128, "50#%s#%s#%s",data);
	DNQ_INFO(DNQ_MOD_RABBITMQ, "send response to server:\n%s", response);
	die_on_error(amqp_basic_publish(conn,
	                                pchnl->chid,
	                                amqp_cstring_bytes(pchnl->exchange),
	                                amqp_cstring_bytes(pchnl->rtkey),
	                                0,
	                                0,
	                                &props,
	                                amqp_cstring_bytes(response)),
	             "Publishing");
    return 0;
}

int send_msg_to_server(
    amqp_connection_state_t conn,
    channel_t *pchnl,
    char *message)
{
    int ret = 0;
	amqp_basic_properties_t props;
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	props.content_type = amqp_cstring_bytes("text/plain");
	props.delivery_mode = 2; /* persistent delivery mode */
	//snprintf(response_buffer,128, "50#%s#%s#%s",data);
	
	ret = die_on_error(amqp_basic_publish(conn,
	                                pchnl->chid,
	                                amqp_cstring_bytes(pchnl->exchange),
	                                amqp_cstring_bytes(pchnl->rtkey),
	                                0,
	                                0,
	                                &props,
	                                amqp_cstring_bytes(message)),
	             "Publishing");
    if(ret == 0)
        DNQ_INFO(DNQ_MOD_RABBITMQ, "send success! chid=%d, ex=%s, key=%s",\
        pchnl->chid, pchnl->exchange, pchnl->rtkey);
    else
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "send failed! chid=%d, ex=%s, key=%s",\
        pchnl->chid, pchnl->exchange, pchnl->rtkey);
    return 0;
}

cJSON *json_data_prepare_warn(char *data)
{
    cJSON *pjson = NULL;
    cJSON *pjson_array = NULL;
    cJSON *proom = NULL;
    char  *pstr = NULL;

    pjson = cJSON_CreateObject();
    if(!pjson)
        return NULL;
    cJSON_AddStringToObject(pjson, "controllerid",  "AA:BB:CC");

    pjson_array = cJSON_CreateArray();
    cJSON_AddItemToObject(pjson, "rooms", pjson_array);
    proom = cJSON_CreateObject();
    cJSON_AddStringToObject(proom, "roomid", "22");
    cJSON_AddNumberToObject(proom, "degree", 55);
    cJSON_AddItemToArray(pjson_array, proom);

    pstr = cJSON_Print(pjson);
    if(!pstr)
    {
        cJSON_Delete(pjson);
        return NULL;
    }
    strcpy(data, pstr);

    dnq_free(pstr);
    cJSON_Delete(pjson);
}


int json_file_read(char *filename, char *buffer, int len)
{
    int    ret = 0;
    FILE  *fp = NULL;

    //memset(buffer, 0, len);
    fp = fopen(filename, "r");
    if(fp == NULL)
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "open file %s error! err=%s!", filename, strerror(errno));
        return -1;
    }
    ret = strlen(buffer);
    ret = fread(buffer, 1, len, fp);
    if(len < 0)
    {
        fclose(fp);
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "read error! err=%s!", filename, strerror(errno));
        return -1;
    }
    printf("read len=%d\n", ret);
    buffer[ret] = '\0';

    fclose(fp);
    return ret; 
}

int json_file_write(char *filename, char *buffer, int len)
{
    int    ret = 0;
    FILE  *fp = NULL;
    fp = fopen(filename, "w+");
    if(fp == NULL)
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "open file %s error! err=%s!", filename, strerror(errno));
        return -1;
    }
    ret = strlen(buffer);
    ret = fwrite(buffer, 1, len, fp);
    if(len < 0)
    {
        fclose(fp);
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "write error! err=%s!", filename, strerror(errno));
        return -1;
    }

    fclose(fp);
    return ret; 
}

int msg_process(amqp_envelope_t *penve, amqp_connection_state_t conn)
{
    char  *json_msg = NULL;
    char  *message[1024];
    char  *json_response = NULL;
    json_type_e    type = 0;
    channel_t   *pchnl = NULL;

    pchnl = &channels[2];
    json_msg = penve->message.body.bytes;

    type = json_parse(json_msg, message);
    if(type < 0)
        return -1;

    DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type=%d", type);
    if(type == MSG_TYPE_AUTHORRIZATION)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: authorization");
        client_response_t response;
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "authorization");
        
        json_response = json_create_response(&response);
        send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(type == MSG_TYPE_TEMP_POLICY)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: policy");
        client_response_t response;
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "policy");
        
        json_response = json_create_response(&response);
        send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(type == MSG_TYPE_TEMP_LIMIT)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: limit");
        client_response_t response;
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "limit");
        
        json_response = json_create_response(&response);
        send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(type == MSG_TYPE_DEGREE_ERROR)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: degree error");
        client_response_t response;
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "error");
        
        json_response = json_create_response(&response);
        send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(type == MSG_TYPE_POWER_CONFIG)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: power config");
        client_response_t response;
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "power");
        
        json_response = json_create_response(&response);
        send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(type == MSG_TYPE_RESPONSE)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: response");
    }
    else if(type == MSG_TYPE_CORRECT)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: correct");
        client_response_t response;
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "correct");
        
        json_response = json_create_response(&response);
        send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    
}

int run(amqp_connection_state_t conn)
{
    amqp_frame_t frame;
    amqp_rpc_reply_t ret;
    amqp_envelope_t envelope;
    //uint64_t start_time = now_microseconds();
    int received = 0;
    int error_count = 0;
    int connected = 0;

    while(1)
    {
    #if 0
        now = now_microseconds();
        if (now > next_summary_time) 
        {
            int countOverInterval = received - previous_received;
            double intervalRate = countOverInterval / ((now - previous_report_time) / 1000000.0);
            printf("%d ms: Received %d - %d since last report (%d Hz)\n",
            (int)(now - start_time) / 1000, received, countOverInterval, (int) intervalRate);

            previous_received = received;
            previous_report_time = now;
            next_summary_time += SUMMARY_EVERY_US;
        }
    #endif
        amqp_maybe_release_buffers(conn);

        struct timeval timeout;
        timeout.tv_sec  = 3;
        timeout.tv_usec = 0;
        ret = amqp_consume_message(conn, &envelope, &timeout, 0);
        if(ret.library_error != AMQP_STATUS_TIMEOUT)
        DNQ_DEBUG(DNQ_MOD_RABBITMQ, "recv msg! type=%d,%d, errno=%d", \
            ret.reply_type, frame.frame_type, ret.library_error);

        if (AMQP_RESPONSE_NORMAL != ret.reply_type)
        {
            if (AMQP_RESPONSE_LIBRARY_EXCEPTION == ret.reply_type &&
            AMQP_STATUS_UNEXPECTED_STATE == ret.library_error)
            {
                DNQ_ERROR(DNQ_MOD_RABBITMQ, "reply exception!");
                if (AMQP_STATUS_OK != amqp_simple_wait_frame(conn, &frame))
                {
                    DNQ_ERROR(DNQ_MOD_RABBITMQ, "test2!");
                    return;
                }
                if (AMQP_FRAME_METHOD == frame.frame_type)
                {
                    switch (frame.payload.method.id)
                    {
                        case AMQP_BASIC_ACK_METHOD:
                        /* if we've turned publisher confirms on, and we've published a message
                        * here is a message being confirmed
                        */
                        DNQ_ERROR(DNQ_MOD_RABBITMQ, "Received AMQP_BASIC_ACK_METHOD");
                        break;
                        case AMQP_BASIC_RETURN_METHOD:
                        /* if a published message couldn't be routed and the mandatory flag was set
                        * this is what would be returned. The message then needs to be read.
                        */
                        {
                            DNQ_ERROR(DNQ_MOD_RABBITMQ, "Received AMQP_BASIC_RETURN_METHOD");
                            amqp_message_t message;
                            ret = amqp_read_message(conn, frame.channel, &message, 0);
                            if (AMQP_RESPONSE_NORMAL != ret.reply_type)
                            {
                                return;
                            }

                            amqp_destroy_message(&message);
                        }

                        break;

                        case AMQP_CHANNEL_CLOSE_METHOD:
                        /* a channel.close method happens when a channel exception occurs, this
                        * can happen by publishing to an exchange that doesn't exist for example
                        *
                        * In this case you would need to open another channel redeclare any queues
                        * that were declared auto-delete, and restart any consumers that were attached
                        * to the previous channel
                        */

                        DNQ_ERROR(DNQ_MOD_RABBITMQ, "Received AMQP_CHANNEL_CLOSE_METHOD");
                        return;

                        case AMQP_CONNECTION_CLOSE_METHOD:
                        /* a connection.close method happens when a connection exception occurs,
                        * this can happen by trying to use a channel that isn't open for example.
                        *
                        * In this case the whole connection must be restarted.
                        */

                        DNQ_ERROR(DNQ_MOD_RABBITMQ, "Received  AMQP_CONNECTION_CLOSE_METHOD");
                        return;

                        default:
                        DNQ_ERROR(DNQ_MOD_RABBITMQ, "An unexpected method was received %d\n", frame.payload.method.id);
                        fprintf(stderr ,"An unexpected method was received %d\n", frame.payload.method.id);
                        return;
                    }
                }
            }     
            else
            {
                //DNQ_DEBUG(DNQ_MOD_RABBITMQ, "error %d %d",\
                //    ret.reply_type,ret.library_error);
                if(ret.library_error != AMQP_STATUS_TIMEOUT)
                {
                    connected = 0;
                    error_count++;
                }
                else
                {
                    connected = 1;
                    error_count = 0;
                }
                if(error_count == 50)
                    return -1;
            }
        }
        else
        {
            DNQ_INFO(DNQ_MOD_RABBITMQ, "recv content:");
            amqp_dump(envelope.message.body.bytes, envelope.message.body.len);
            msg_process(&envelope, conn);
            amqp_destroy_envelope(&envelope);
        }
        received++;
        usleep(100*1000);
    }
}

int msg_thread(char *serverip, int port, amqp_connection_state_t *pconn)
{
    int i = 0;
    int status;
    amqp_socket_t *socket = NULL;
    amqp_connection_state_t conn;
    amqp_queue_declare_ok_t *r = NULL;
    channel_t  *pchnl = NULL;
    
    conn = amqp_new_connection();
    *pconn = conn;
    if(!conn)
        dnq_error(-1, "amqp_new_connection error!");

    socket = amqp_tcp_socket_new(conn);
    if(!socket)
        dnq_error(-1, "amqp_tcp_socket_new error!");
   
    status = amqp_socket_open(socket, SERVER_IPADDR, SERVER_PORT);
    if(status < 0)
        dnq_error(status, "amqp_socket_open error!");

    DNQ_INFO(DNQ_MOD_RABBITMQ, "create new connecting! socket=%d, ip=%s, port=%d", \
        status, SERVER_IPADDR, SERVER_PORT);

    die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 30, AMQP_SASL_METHOD_PLAIN, \
    username, password), "Logging in");

    DNQ_INFO(DNQ_MOD_RABBITMQ, "Login success! user=%s, passwd=%s", username, password);


    pchnl = &channels[0];

    for(i=0; i<5; i++)
    {
        pchnl = &channels[i];
        //create channel
        amqp_channel_open(conn, pchnl->chid);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");
        DNQ_INFO(DNQ_MOD_RABBITMQ, "Opening channel %d success!", pchnl->chid);

        if(i == 0)
        {
        //exchange declare
        amqp_exchange_declare(conn, /* 连接 */
                            pchnl->chid, 
                            amqp_cstring_bytes(pchnl->exchange),
                            amqp_cstring_bytes("direct"),
                            0, /* passive */
                            1, /* durable 持久化 */
                            0, /* auto delete */
                            0, /* internal */
                            amqp_empty_table);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "exchange declare %s!", pchnl->exchange);

        //queue declare
        strcat(pchnl->qname, MAC_ADDR);
        r = amqp_queue_declare(conn, 
                            pchnl->chid,
                            amqp_cstring_bytes(pchnl->qname),
                            0, /* passive */
                            0, /* durable 持久化 */
                            0, /* exclusive */
                            1, /* auto delete */
                            amqp_empty_table);

        die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
        DNQ_INFO(DNQ_MOD_RABBITMQ, "queue declare %s!", pchnl->qname);

        
        //bind
        strcpy(pchnl->rtkey, MAC_ADDR);
        amqp_queue_bind(conn, 
                        pchnl->chid, 
                        amqp_cstring_bytes(pchnl->qname), 
                        amqp_cstring_bytes(pchnl->exchange),
                        amqp_cstring_bytes(pchnl->rtkey), 
                        amqp_empty_table);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");
        DNQ_INFO(DNQ_MOD_RABBITMQ, "queue %s bind on exchange=%s, rtkey=%s!", \
            pchnl->qname, pchnl->exchange, pchnl->rtkey);
        }
    }

    pchnl = &channels[0];
    //consume
    amqp_basic_consume(conn, 
                        pchnl->chid, 
                        amqp_cstring_bytes(pchnl->qname), 
                        amqp_empty_bytes, /* consumer_tag */
                        0,  /* no local */
                        1,  /* no ack */
                        0,  /* exclusive */
                        amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

    //purge
    amqp_queue_purge(conn, pchnl->chid, amqp_cstring_bytes(pchnl->qname));

    run(conn);

    /* error close the channel */
    for(i=0; i<5; i++)
    {
        pchnl = &channels[i];
        die_on_amqp_error(amqp_channel_close(conn, pchnl->chid, AMQP_REPLY_SUCCESS), "Closing channel");
    }
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
    die_on_error(amqp_destroy_connection(conn), "Ending connection");

    DNQ_ERROR(DNQ_MOD_RABBITMQ, "msg_thread exit...");
    return 0;
}

int rabbitmq_start()
{
    while(1)
    {
        msg_thread(serverip, serverport, &g_conn);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg_thread start!");
        sleep(3);
    }
}


void *rabbitmq_test()
{
    int i = 0;
    channel_t *pchnl = NULL;
    char   *msg;
    char    buffer[1024];
    client_status_t   status = {MAC_ADDR, 2, {{1, 23, 45}, {2, 45, 67}}};
    client_loss_t     loss = {MAC_ADDR, 1, {0, 55}};
    client_response_t response = {{MAC_ADDR}, NULL};
    client_config_t   config = {MAC_ADDR, 1, {1, 2, 3, 4, 5, 6, 7}};
    client_warn_t     warn = {MAC_ADDR, 3, {{1,7}, {2,8}, {3,9}}};
    int time = 3;

    sleep(5);
    while(1)
    {

    #if 0
        for(i=0; i<5; i++)
        {
            pchnl = channels[i];
            send_msg_to_server(g_conn, pchnl->chid, msg);
            sleep(1);
        }
    #else
        msg = json_create_status(&status);
        send_msg_to_server(g_conn, &channels[1], msg);
        DNQ_DEBUG(DNQ_MOD_RABBITMQ, "send status msg:\n%s", msg);
        dnq_free(msg);
        sleep(time);
        
        //msg = json_create_loss(&loss);
        //send_msg_to_server(g_conn, &channels[2], msg);
        //sleep(3);
        
        msg = json_create_response(&response);
        send_msg_to_server(g_conn, &channels[2], msg);
        DNQ_DEBUG(DNQ_MOD_RABBITMQ, "send response msg:\n%s", msg);
        dnq_free(msg);
        sleep(time);
        
        msg = json_create_config(&config);
        send_msg_to_server(g_conn, &channels[3], msg);
        DNQ_DEBUG(DNQ_MOD_RABBITMQ, "send config msg:\n%s", msg);
        dnq_free(msg);
        sleep(time);
        
        msg = json_create_warn(&warn);
        send_msg_to_server(g_conn, &channels[4], msg);
        DNQ_DEBUG(DNQ_MOD_RABBITMQ, "send warn msg:\n%s", msg);
        dnq_free(msg);
        sleep(time);
    #endif
        
        //send_response_to_server(g_conn, &channels[0], buffer);
        sleep(1);
    }
}

int main_()
{
    ngx_pool_t *pool = NULL;
    
    dnq_init();
    dnq_debug_setlever(1, 5);
    
    dnq_task_create("rabbitmq_test", 0, rabbitmq_test, NULL);
    rabbitmq_start();
}

int json_parse_test()
{
    int type;
    char buffer[2048] = {0};
    char cbuffer[2048] = {0};

    dnq_debug_setlever(1, 5);
    
    //server config file 
    printf("==================server json===================\n");
    json_file_read("configs/"SERVER_CFG_FILE_AUTHORRIZATION, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    json_file_read("configs/"SERVER_CFG_FILE_POLICY, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    json_file_read("configs/"SERVER_CFG_FILE_LIMIT, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    json_file_read("configs/"SERVER_CFG_FILE_ERROR, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    json_file_read("configs/"SERVER_CFG_FILE_POWER, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    json_file_read("configs/"SERVER_CFG_FILE_RESPONSE, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    json_file_read("configs/"SERVER_CFG_FILE_CORRECT, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);


    //client config file 
    printf("==================client json===================\n");
    json_file_read("configs/"CLIENT_CFG_FILE_STATUS, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);
    
    json_file_read("configs/"CLIENT_CFG_FILE_LOSS, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);
    
    json_file_read("configs/"CLIENT_CFG_FILE_RESPONSE, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    json_file_read("configs/"CLIENT_CFG_FILE_CONFIG, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    json_file_read("configs/"CLIENT_CFG_FILE_WARN, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);
    
    return 0;
}

#if 0
char json_data[] =
    "{\"type\": \"control\",\
    \"time\": \"2011200000000\",\
    \"mode\": \"0\",\
    \"rooms\": [\
        {\
\"id\": \"0\",\
\"dpid\": \"0\",\
            \"timesetting\": [\
                {\
                    \"starttime\": \"11:11\",\
                    \"endtime\": \"22:22\",\
                    \"degrees\": \"33\"\
                },\
                {\
                    \"starttime\": \"44:44\",\
                    \"endtime\": \"55:55\",\
                    \"degrees\": \"66\"\
                }\
            ]\
        },\
        {\
\"id\": \"2\",\
\"dpid\": \"0\",\
            \"timesetting\": [\
                {\
                    \"starttime\": \"77:77\",\
                    \"endtime\": \"88:88\",\
                    \"degrees\": \"99\"\
                },\
                {\
                    \"starttime\": \"01:23\",\
                    \"endtime\": \"01:23\",\
                    \"degrees\": \"123456789\"\
                }\
            ]\
        }\
    ]\
}";
#endif

int json_test()
{
    int ret = 0;
    ngx_pool_t *pool = NULL;
    
    pool = dnq_mempool_init(1024*1024);
    cJSON_Hooks hooks = {dnq_malloc, dnq_free};
    cJSON_InitHooks(&hooks);
    
    json_parse_test();
    
    dnq_mempool_deinit(pool);
    
    return ret;
}


int main0(int argc, char const *const *argv)
{
  char const *hostname;
  int port, status;
  char const *exchange;
  char const *bindingkey;
  amqp_socket_t *socket = NULL;
  amqp_connection_state_t conn;

  amqp_bytes_t queuename;
  channel_t *pchnl;
  int i;
  char real_queue[64];
  char *pcrtlid;

    dnq_mempool_init(1024*1024);
    cJSON_Hooks hooks = {dnq_malloc, dnq_free};
    cJSON_InitHooks(&hooks);

  if (argc < 5) {
    //fprintf(stderr, "Usage: amqp_listen host port exchange bindingkey\n");
    //return 1;
  }

  hostname = argv[1];
  port = atoi(argv[2]);
  exchange = argv[3];
  bindingkey = argv[4];

  hostname = serverip;
  port = serverport;
  pchnl = &channels[13];
  pcrtlid = "192.168.30.189";

    YLOG("serverip=%s, port=%d, chnl=%d\n", \
        serverip, serverport, (pchnl-channels)/sizeof(channel_t));
    YLOG("chnl=%d, exchange=%s, routekey=%s \n", \
        (pchnl-channels)/sizeof(channel_t), \
        pchnl->exchange, pchnl->rtkey);
  strcpy(real_queue, "192.168.30.186");
  sprintf(real_queue,"%s%s",pchnl->qname, "aa:bb:cc");

  conn = amqp_new_connection();

    YLOG("[ychen]: amqp_new_connection!\n");
  socket = amqp_tcp_socket_new(conn);
  if (!socket) {
    die("creating TCP socket");
  }

YLOG("[ychen]: amqp_tcp_socket_new!\n");
  status = amqp_socket_open(socket, hostname, port);
  if (status) {
    die("opening TCP socket");
  }
YLOG("[ychen]: amqp_socket_open!\n");
  die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 30, AMQP_SASL_METHOD_PLAIN, \
    username, password),
                    "Logging in");
  YLOG("[ychen]: amqp_login!\n");

  for(i=0; i<13; i++);;

  amqp_channel_open(conn, pchnl->chid);
  amqp_channel_open(conn, 10);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");
YLOG("[ychen]: amqp_channel_open! chnl=%d\n", pchnl->chid);
  {
    /* */
    amqp_exchange_declare(conn, pchnl->chid, amqp_cstring_bytes(pchnl->exchange),\
        amqp_cstring_bytes("direct"),  0, 1 /* durable 持久化 */, 0, 0, amqp_empty_table);
    YLOG("[ychen]: amqp_exchange_declare! exchange=%s\n", pchnl->exchange);
    amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, pchnl->chid,\
        amqp_cstring_bytes(real_queue), 0, 0, 0, 1, amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
    YLOG("[ychen]: amqp_queue_declare!\n");
    queuename = amqp_bytes_malloc_dup(r->queue);
    if (queuename.bytes == NULL) {
      fprintf(stderr, "Out of memory while copying queue name");
      return 1;
    }
  }

    YLOG("[ychen]: queuename=%s, len=%d!\n", queuename.bytes, queuename.len);
    YLOG("[ychen]: real_queue=%s, len=%d!\n", real_queue, strlen(real_queue));
  amqp_queue_bind(conn, pchnl->chid, amqp_cstring_bytes(real_queue), amqp_cstring_bytes(pchnl->exchange), \
    amqp_cstring_bytes(pchnl->rtkey), amqp_empty_table);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");
  YLOG("[ychen]: amqp_queue_bind! qname=%s, exchange=%s, key=%s\n", \
    pchnl->qname, pchnl->exchange, pchnl->rtkey);

  amqp_basic_consume(conn, pchnl->chid, amqp_cstring_bytes(real_queue), amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");
    YLOG("[ychen]: amqp_basic_consume!\n");
    amqp_queue_purge(conn, pchnl->chid, amqp_cstring_bytes(real_queue));

  {
    for (;;) {
      amqp_rpc_reply_t res;
      amqp_envelope_t envelope;

        /* send msg */
      {
        char messagebody[256] = "sichuan";
        amqp_basic_properties_t props;
        pchnl = &channels[9];
        //cjson_data_prepare(messagebody);
        json_data_prepare_warn(messagebody);
        YLOG("messagebody=%s\n", messagebody);
        props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
        props.content_type = amqp_cstring_bytes("text/plain");
        props.delivery_mode = 2; /* persistent delivery mode */
        die_on_error(amqp_basic_publish(conn,
                            pchnl->chid,
                            amqp_cstring_bytes(pchnl->exchange),
                            amqp_cstring_bytes(pchnl->rtkey), //"door_response"
                            0,
                            0,
                            &props,
                            amqp_cstring_bytes(messagebody)),
                     "Publishing");
      }
        YLOG("[ychen]: amqp_basic_publish!\n");
        YLOG("[ychen]: chnl=%d, pchnl->exchange=%s, pchnl->rtkey=%s\n",\
            pchnl->chid, pchnl->exchange, pchnl->rtkey);

      amqp_maybe_release_buffers(conn);

        struct timeval timeout;
        timeout.tv_sec  = 3;
        timeout.tv_usec = 0;
      res = amqp_consume_message(conn, &envelope, &timeout, 0);

      if (AMQP_RESPONSE_NORMAL != res.reply_type) {
        YLOG("reply: res.reply_type=%d, library_error=%d\n", \
            res.reply_type, res.library_error);
        sleep(3);
        continue;
        break;
      }

      printf("Delivery %u, exchange %.*s routingkey %.*s\n",
             (unsigned) envelope.delivery_tag,
             (int) envelope.exchange.len, (char *) envelope.exchange.bytes,
             (int) envelope.routing_key.len, (char *) envelope.routing_key.bytes);

      if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
        printf("Content-type: %.*s\n",
               (int) envelope.message.properties.content_type.len,
               (char *) envelope.message.properties.content_type.bytes);
      }
      printf("----\n");

      amqp_dump(envelope.message.body.bytes, envelope.message.body.len);

      amqp_destroy_envelope(&envelope);
      sleep(5);
    }
  }

  die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
  die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
  die_on_error(amqp_destroy_connection(conn), "Ending connection");

  return 0;
}

int main1(int argc, char const *const *argv)
{
  char const *hostname;
  int port, status;
  char const *exchange;
  char const *bindingkey;
  amqp_socket_t *socket = NULL;
  amqp_connection_state_t conn;

  amqp_bytes_t queuename;
  channel_t *pchnl;
  char real_queue[64];
  char *pcrtlid;

  if (argc < 5) {
    //fprintf(stderr, "Usage: amqp_listen host port exchange bindingkey\n");
    //return 1;
  }

  hostname = argv[1];
  port = atoi(argv[2]);
  exchange = argv[3];
  bindingkey = argv[4];

  hostname = serverip;
  port = serverport;
  pchnl = &channels[13];
  pcrtlid = "192.168.30.189";

  strcpy(real_queue, "192.168.30.186");
  sprintf(real_queue,"%s%s",pcrtlid,pchnl->qname);

  conn = amqp_new_connection();

    YLOG("[ychen]: amqp_new_connection!\n");
  socket = amqp_tcp_socket_new(conn);
  if (!socket) {
    die("creating TCP socket");
  }

YLOG("[ychen]: amqp_tcp_socket_new!\n");
  status = amqp_socket_open(socket, hostname, port);
  if (status) {
    die("opening TCP socket");
  }
YLOG("[ychen]: amqp_socket_open!\n");
  die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 30, AMQP_SASL_METHOD_PLAIN, \
    username, password),
                    "Logging in");
  YLOG("[ychen]: amqp_login!\n");
  amqp_channel_open(conn, pchnl->chid);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");
YLOG("[ychen]: amqp_channel_open!\n");
  {
    /* */
    amqp_exchange_declare(conn, pchnl->chid, amqp_cstring_bytes(pchnl->exchange),\
        amqp_cstring_bytes("direct"),  0, 1 /* durable 持久化 */, 0, 0, amqp_empty_table);
    YLOG("[ychen]: amqp_exchange_declare!\n");
    amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, pchnl->chid,\
        amqp_cstring_bytes(real_queue), 0, 0, 0, 1, amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
    YLOG("[ychen]: amqp_queue_declare!\n");
    queuename = amqp_bytes_malloc_dup(r->queue);
    if (queuename.bytes == NULL) {
      fprintf(stderr, "Out of memory while copying queue name");
      return 1;
    }
  }

    YLOG("[ychen]: queuename=%s, len=%d!\n", queuename.bytes, queuename.len);
  amqp_queue_bind(conn, pchnl->chid, queuename, amqp_cstring_bytes(pchnl->exchange), \
    amqp_cstring_bytes(pchnl->rtkey), amqp_empty_table);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");
  YLOG("[ychen]: amqp_queue_bind!\n");

  amqp_basic_consume(conn, pchnl->chid, queuename, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");
    YLOG("[ychen]: amqp_basic_consume!\n");
  {
    for (;;) {
      amqp_rpc_reply_t res;
      amqp_envelope_t envelope;

        /* send msg */
      {
        char *messagebody = "sichuan";
        amqp_basic_properties_t props;
        props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
        props.content_type = amqp_cstring_bytes("text/plain");
        props.delivery_mode = 2; /* persistent delivery mode */
        die_on_error(amqp_basic_publish(conn,
                                        1,
                                        amqp_cstring_bytes(pchnl->exchange),
                                        amqp_cstring_bytes(pchnl->rtkey),
                                        0,
                                        0,
                                        &props,
                                        amqp_cstring_bytes(messagebody)),
                     "Publishing");
      }
        YLOG("[ychen]: amqp_basic_publish!\n");

      amqp_maybe_release_buffers(conn);

      res = amqp_consume_message(conn, &envelope, NULL, 0);

      if (AMQP_RESPONSE_NORMAL != res.reply_type) {
        break;
      }

      printf("Delivery %u, exchange %.*s routingkey %.*s\n",
             (unsigned) envelope.delivery_tag,
             (int) envelope.exchange.len, (char *) envelope.exchange.bytes,
             (int) envelope.routing_key.len, (char *) envelope.routing_key.bytes);

      if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
        printf("Content-type: %.*s\n",
               (int) envelope.message.properties.content_type.len,
               (char *) envelope.message.properties.content_type.bytes);
      }
      printf("----\n");

      amqp_dump(envelope.message.body.bytes, envelope.message.body.len);

      amqp_destroy_envelope(&envelope);
    }
  }

  die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
  die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
  die_on_error(amqp_destroy_connection(conn), "Ending connection");

  return 0;
}





