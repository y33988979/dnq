#ifndef _DNQ_RABBITMQ_H_
#define _DNQ_RABBITMQ_H_

#include "common.h"

#define SIZE_8       8
#define SIZE_16      16
#define SIZE_32      32

#define CTRL_WHOLE_ROOM    0
#define CTRL_SINGLE_ROOM   1

#define SERVER_IPADDR   "112.74.43.136"
#define SERVER_PORT     5672

#define MAC_ADDR   "70b3d5cf4924"


typedef struct channel_t{
	int chid;
	char qname[32];
	char exchange[32];
	char rtkey[32];
}channel_t;

/*
* message type define
* 消息类型字段  枚举
*/

typedef enum json_type
{
    JSON_TYPE_AUTHORRIZATION,
    JSON_TYPE_TEMP_POLICY,
    JSON_TYPE_TEMP_LIMIT,
    JSON_TYPE_TEMP_ERROR,
    JSON_TYPE_POWER_CONFIG,
    JSON_TYPE_RESPONSE,
    JSON_TYPE_CORRECT,
    JSON_TYPE_INIT
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

#define JSON_FILE_AUTHORRIZATION  "authorization.json"
#define JSON_FILE_POLICY          "policy.json"
#define JSON_FILE_LIMIT           "limit.json"
#define JSON_FILE_ERROR           "error.json"
#define JSON_FILE_POWER           "power.json"
#define JSON_FILE_RESPONSE        "response.json"
#define JSON_FILE_CORRECT         "correct.json"
#define JSON_FILE_INIT            "init.json"

/* json file for test */
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

/*
* cjson convert to struct, authorization manage
*
* cjson数据的C结构体，云端向控制器 发送授权控制
*
*/
typedef struct server_authorization
{
    U8      type[SIZE_32];
    U8      time[SIZE_32];
    U8      authorization[SIZE_32];
}server_authorization_t;

/*
* cjson convert to struct, temperature policy
*
* cjson数据的C结构体，云端向控制器 下发温度策略
*
*/
typedef struct timesetting
{
    U8   starttime[SIZE_16];
    U8   endtime[SIZE_16];
    U32  start;  /* starttime second */
    U32  end;    /* endtime second */
    U16  degrees;
}timesetting_t;

typedef struct room_temp_policy
{
    U16            room_id;
    U16            dpid;
    U16            time_setting_cnt;
    timesetting_t  time_setting[4];
}room_temp_policy_t;

typedef struct server_temp_policy
{
    U8        type[SIZE_32];
    U8        time[SIZE_32];
    U8        ctrl_id[SIZE_32];
    U16       mode;
    U16       rooms_cnt;
    room_temp_policy_t    rooms[DNQ_ROOM_MAX];
}server_temp_policy_t;

/*
* cjson convert to struct, temperature limit
*
* cjson数据的C结构体，云端向控制器 发送高低温限制
*
*/
typedef struct room_temp_limit
{
    U16       room_id;  
    U16       max;  
    U16       min;  
}room_temp_limit_t;

typedef struct server_temp_limit
{
    U8        type[SIZE_32];
    U8        time[SIZE_32];
    U8        ctrl_id[SIZE_32];
    U16       mode;
    U16       rooms_cnt;
    room_temp_limit_t      rooms[DNQ_ROOM_MAX];
}server_temp_limit_t;

/*
* cjson convert to struct, temperature error
*
* cjson数据的C结构体，云端向控制器 设置温度回差
*
*/

typedef struct room_temp_error
{
    U16       room_id;
    U16       error;
}room_temp_error_t;

typedef struct server_temp_error
{
    U8        type[SIZE_32];
    U8        time[SIZE_32];
    U8        ctrl_id[SIZE_32];
    U16       mode;
    U16       rooms_cnt;
    room_temp_error_t      rooms[DNQ_ROOM_MAX];
}server_temp_error_t;

/*
* cjson convert to struct, power config
*
* cjson数据的C结构体，云端向控制器 发送功率配置
*
*/

typedef struct room_power_config
{
    U16       room_id;
    U16       config_cnt;
    U16       power[6];   /* need fixed?  */
    U16       num[6];     /* need fixed?  */
}room_power_config_t;

typedef struct server_power_config
{
    U8        type[SIZE_32];
    U8        time[SIZE_32];
    U8        ctrl_id[SIZE_32];
    U16       mode;
    U16       rooms_cnt;
    room_power_config_t      rooms[DNQ_ROOM_MAX];
}server_power_config_t;

/*
* cjson convert to struct, a response from server to controller
*
* cjson数据的C结构体，云端向控制器 发送应答消息
*
*/
typedef struct server_response
{
    U8        type[SIZE_32];
    U8        time[SIZE_32];
    U8        status[SIZE_32];
}server_response_t;


/*
* cjson convert to struct, temperature rectify 
*
* cjson数据的C结构体，云端向控制器 设置房间矫正温度
*
*/

typedef struct room_temp_correct
{
    U16       room_id;
    S16       correct;
}room_temp_correct_t;

typedef struct server_temp_correct
{
    U8        type[SIZE_32];
    U8        time[SIZE_32];
    U8        ctrl_id[SIZE_32];
    U16       mode;
    U16       rooms_cnt;
    room_temp_correct_t  rooms[DNQ_ROOM_MAX];
}server_temp_correct_t;

/*
* cjson convert to struct, initialize device install info 
*
* cjson数据的C结构体，云端向控制器 初始化房间信息，设备安装信息。
*
*/
typedef struct partition_info
{
    U8        no[SIZE_32];
    U16       id;
    U8        memo[SIZE_32];
    U8        name[SIZE_32];
    U16       isDelete;
    
}partition_info_t;

typedef struct room_info
{
    U16       error;
    U16       max;
    U16       min;
    U16       correct;
    U16       room_id;
    U8        room_name[SIZE_16];
    U16       room_order;
    U16       room_floor;
    U16       position;
}room_info_t;

typedef struct server_init_info
{
    U8        type[SIZE_32];
    partition_info_t partition;
    U32       project_id;
    U32       building_id;
    U32       equipment_id;
    U8        project_name[SIZE_32];
    U8        building_name[SIZE_32];
    U8        equipment_mac[SIZE_32];
    U16       rooms_cnt;
    room_info_t    rooms[DNQ_ROOM_MAX];
    
}server_init_info_t;

/*
* cjson convert to struct, All config info 
*
* 所有配置的结构体，存放云端向控制器发送的所有配置
*
*/
typedef struct _dnq_config
{
    U32  inited;
    server_authorization_t authorization;
    server_temp_policy_t   temp_policy;
    server_temp_limit_t    temp_limit;
    server_temp_error_t    temp_error;
    server_power_config_t  power_config;
    server_response_t      response;
    server_temp_correct_t  temp_correct;
    server_init_info_t     init;
}dnq_config_t;


/*
* controller to server 
* 控制器-->服务器 发送消息
*/
typedef struct client_room
{
    U32       room_id;
    U32       value;
    U32       loss;
}client_room_t;

typedef struct client_config_room
{
    U32       room_id;
    U32       degreePolicy;
    U32       maxdegree;
    U32       mindegree;
    U32       error;
    S32       correct;
    U32       power;
    
}client_room_config_t;

typedef struct client_status
{
    U8        mac[SIZE_32];
    U32       rooms_cnt;
    client_room_t  rooms[DNQ_ROOM_MAX];
}client_status_t;

typedef struct client_loss
{
    U8        mac[SIZE_32];
    U32       rooms_cnt;
    client_room_t  rooms[DNQ_ROOM_MAX];
}client_loss_t;

typedef struct client_response
{
    U8        mac[SIZE_32];
    U8        status[SIZE_32];
}client_response_t;

typedef struct client_config
{
    U8        mac[SIZE_32];
    U32       rooms_cnt;
    client_room_config_t  rooms[DNQ_ROOM_MAX];
}client_config_t;

typedef struct client_warn
{
    U8        mac[SIZE_32];
    U32       rooms_cnt;
    client_room_t  rooms[DNQ_ROOM_MAX];
}client_warn_t;


extern dnq_config_t dnq_config;
#define dnq_get_authorization_config()  &dnq_config.authorization
#define dnq_get_temp_policy_config()  &dnq_config.temp_policy
#define dnq_get_temp_limit_config()  &dnq_config.temp_limit
#define dnq_get_temp_error_config()  &dnq_config.temp_error
#define dnq_get_power_config_config()  &dnq_config.power_config
#define dnq_get_response_config()  &dnq_config.response
#define dnq_get_temp_correct_config()  &dnq_config.temp_correct
#define dnq_get_init_config()  &dnq_config.init

S32 dnq_rabbitmq_init();
S32 dnq_rabbitmq_deinit();

#endif /* _DNQ_RABBITMQ_H_ */

