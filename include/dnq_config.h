
#ifndef _DNQ_CONFIG_H_
#define _DNQ_CONFIG_H_

#include "common.h"

#define CTRL_WHOLE_ROOM    0
#define CTRL_SINGLE_ROOM   1

#define DNQ_CONFIG_PATH           "/root/dnq/configs"
#define DNQ_DATA_FILE             "/root/dnq/dnq.dat"
#define DNQ_CONFIG_FILE           "/root/dnq/dnq.conf"
#define DNQ_SN_CONF_FILE          "/root/dnq/sn.conf"

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
#define DEGREES_NULL    0xFF

/*
* cjson convert to struct, authorization manage
*
* cjson数据的C结构体，云端向控制器 发送授权控制
*
*/
typedef struct authorization
{
    U8      type[SIZE_32];
    U8      time[SIZE_32];
    U8      authorization[SIZE_32];
}authorization_t;

/*
* cjson convert to struct, temperature policy
*
* cjson数据的C结构体，云端向控制器 下发温度策略
*
*/
typedef struct timesetting
{
    U32  start;  /* starttime second */
    U32  end;    /* endtime second */
    U16  degrees;
    U8   starttime[SIZE_16];
    U8   endtime[SIZE_16];
}timesetting_t;

typedef struct room_temp_policy
{
    U16            room_id;
    U16            dpid;
    U16            time_setting_cnt;
    timesetting_t  time_setting[4];
}room_temp_policy_t;

typedef struct _policy_config
{
    U8        type[SIZE_32];
    U8        time[SIZE_32];
    U8        ctrl_id[SIZE_32];
    U16       mode;
    U16       rooms_cnt;
    room_temp_policy_t    rooms[DNQ_ROOM_MAX];
}policy_config_t;

/*
* cjson convert to struct, temperature limit
*
* cjson数据的C结构体，云端向控制器 发送高低温限制
*
*/
typedef struct room_temp_limit
{
    U16       room_id;  
    U16       min;  
    U16       max;  
    
}room_temp_limit_t;

typedef struct _limit_config
{
    U8        type[SIZE_32];
    U8        time[SIZE_32];
    U8        ctrl_id[SIZE_32];
    U16       mode;
    U16       rooms_cnt;
    room_temp_limit_t      rooms[DNQ_ROOM_MAX];
}limit_config_t;

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

typedef struct _error_config
{
    U8        type[SIZE_32];
    U8        time[SIZE_32];
    U8        ctrl_id[SIZE_32];
    U16       mode;
    U16       rooms_cnt;
    room_temp_error_t      rooms[DNQ_ROOM_MAX];
}error_config_t;

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

typedef struct _power_config
{
    U8        type[SIZE_32];
    U8        time[SIZE_32];
    U8        ctrl_id[SIZE_32];
    U16       mode;
    U16       rooms_cnt;
    room_power_config_t      rooms[DNQ_ROOM_MAX];
}power_config_t;

/*
* cjson convert to struct, a response from server to controller
*
* cjson数据的C结构体，云端向控制器 发送应答消息
*
*/
typedef struct _response
{
    U8        type[SIZE_32];
    U8        time[SIZE_32];
    U8        status[SIZE_32];
}response_t;


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

typedef struct _correct_config
{
    U8        type[SIZE_32];
    U8        time[SIZE_32];
    U8        ctrl_id[SIZE_32];
    U16       mode;
    U16       rooms_cnt;
    room_temp_correct_t  rooms[DNQ_ROOM_MAX];
}correct_config_t;

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
    S16       correct;
    U16       room_id;
    U8        room_name[SIZE_16];
    U16       room_order;
    U16       room_floor;
    U8        position[SIZE_16];
}room_info_t;

typedef struct _init_info
{
    U8        type[SIZE_32];
    U8        time[SIZE_32];
    partition_info_t partition;
    U32       project_id;
    U32       building_id;
    U32       equipment_id;
    U32       heater_work_mode;
    U8        project_name[SIZE_32];
    U8        building_name[SIZE_32];
    U8        buildPosition[SIZE_32];
    U8        hostName[SIZE_32];
    U8        equipment_mac[SIZE_32];
    U16       rooms_cnt;
    room_info_t    rooms[DNQ_ROOM_MAX];
    U32       inited;
    
}init_info_t;

/*
* cjson convert to struct, All config info 
*
* 所有配置的结构体，存放云端向控制器发送的所有配置
*
*/
typedef struct _dnq_config
{
    U32  inited;
    authorization_t  authorization;
    policy_config_t  policy_config;
    limit_config_t   limit_config;
    error_config_t   error_config;
    power_config_t   power_config;
    response_t       response;
    correct_config_t correct_config;
    init_info_t      init;
    U32              sensor_generation;
    U32              reserved;
}dnq_config_t;


/*
* controller to server 
* 控制器-->服务器 发送消息
*/
typedef struct client_room
{
    U16       room_id;
    U16       degree;
    U16       loss;
}client_room_t;

typedef struct client_config_room
{
    U16       room_id;
    U16       degreePolicy;
    U16       maxdegree;
    U16       mindegree;
    U16       error;
    S16       correct;
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
    U8        type[SIZE_32];
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

S32 dnq_config_init();
S32 dnq_config_deinit();
S32 dnq_file_read(U8 *filepath, U8 *buffer, U32 len);
S32 dnq_file_write(U8 *filepath, U8 *buffer, U32 len);
void dnq_config_print();
S32 dnq_config_load();
S32 dnq_data_file_save();
S32 dnq_config_check_and_sync(json_type_e json_type, U8 *json_data, U32 len, void *cjson_struct);
S32 dnq_config_sync_to_lcd(json_type_e json_type, void *cjson_struct, U32 room_id);

extern dnq_config_t g_dnq_config;

#define init_info_is_ok()  (g_dnq_config.init.inited)

authorization_t*
    dnq_get_authorization_config(authorization_t *config);
policy_config_t*
    dnq_get_temp_policy_config(policy_config_t *config);
limit_config_t*
    dnq_get_temp_limit_config(limit_config_t *config);
error_config_t*
    dnq_get_temp_error_config(error_config_t *config);
power_config_t*
    dnq_get_power_config_config(power_config_t *config);
response_t*
    dnq_get_response_config(response_t *config);
correct_config_t*
    dnq_get_temp_correct_config(correct_config_t *config);
init_info_t*
    dnq_get_init_config(init_info_t *config);

timesetting_t* dnq_get_room_setting_by_time(U32 room_id, U32 current_time);
S32 dnq_get_room_current_setting_temp(U32 room_id);

#endif /* _DNQ_CONFIG_H_ */

