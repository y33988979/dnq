#ifndef _DNQ_RABBITMQ_H_
#define _DNQ_RABBITMQ_H_

#include "common.h"

#define SIZE      32

/*
* cjson convert to struct, authorization manage
*
* cjson数据的C结构体，云端向控制器 发送授权控制
*
*/
typedef struct server_authorization
{
    U8      type[SIZE];
    U8      time[SIZE];
    U8      authorization[SIZE];
}server_authorization_t;

/*
* cjson convert to struct, temperature policy
*
* cjson数据的C结构体，云端向控制器 下发温度策略
*
*/
typedef struct timesetting
{
    U8   starttime[SIZE];
    U8   endtime[SIZE];
    U8   degrees[SIZE];
}timesetting_t;

typedef struct room_temp_policy
{
    U16            room_id;
    U16            dpid;
    U16            time_setting_cnt;
    timesetting_t  time_setting[5];
}room_temp_policy_t;

typedef struct server_temp_policy
{
    U8        type[SIZE];
    U8        time[SIZE];
    U8        ctrl_id[SIZE];
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
    U8        type[SIZE];
    U8        time[SIZE];
    U8        ctrl_id[SIZE];
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
    U8        type[SIZE];
    U8        time[SIZE];
    U8        ctrl_id[SIZE];
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
    U16       power[5];   /* need fixed?  */
    U16       num[5];     /* need fixed?  */
}room_power_config_t;

typedef struct server_power_config
{
    U8        type[SIZE];
    U8        time[SIZE];
    U8        ctrl_id[SIZE];
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
    U8        type[SIZE];
    U8        time[SIZE];
    U8        status[SIZE];
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
    U8        type[SIZE];
    U8        time[SIZE];
    U8        ctrl_id[SIZE];
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
    U8        no[SIZE];
    U32       id;
    U8        memo[SIZE];
    U8        name[SIZE];
    U32       isDelete;
    
}partition_info_t;

typedef struct room_info
{
    U16       error;
    U16       max;
    U16       min;
    U16       correct;
    U16       room_id;
    U8        room_name[16];
    U16       room_order;
    U16       room_floor;
    U16       position;
}room_info_t;

typedef struct server_init_info
{
    U8        type[SIZE];
    partition_info_t partition;
    U32       project_id;
    U32       building_id;
    U32       equipment_id;
    U8        project_name[SIZE];
    U8        building_name[SIZE];
    U8        equipment_mac[SIZE];
    U16       rooms_cnt;
    room_info_t    rooms[DNQ_ROOM_MAX];
    
}server_init_info_t;


/*
* controller to server 
* 控制器-->服务器 发送消息
*/
typedef struct client_room
{
    int       room_id;
    int       value;
    int       loss;
}client_room_t;

typedef struct client_config_room
{
    int       room_id;
    int       degreePolicy;
    int       maxdegree;
    int       mindegree;
    int       error;
    int       corrent;
    int       power;
    
}client_room_config_t;

typedef struct client_status
{
    char      mac[SIZE];
    int       rooms_cnt;
    client_room_t  rooms[DNQ_ROOM_MAX];
}client_status_t;

typedef struct client_loss
{
    char      mac[SIZE];
    int       rooms_cnt;
    client_room_t  rooms[DNQ_ROOM_MAX];
}client_loss_t;

typedef struct client_response
{
    char      mac[SIZE];
    char      status[SIZE];
}client_response_t;

typedef struct client_config
{
    char      mac[SIZE];
    int       rooms_cnt;
    client_room_config_t  rooms[DNQ_ROOM_MAX];
}client_config_t;

typedef struct client_warn
{
    char      mac[SIZE];
    int       rooms_cnt;
    client_room_t  rooms[DNQ_ROOM_MAX];
}client_warn_t;

#endif /* _DNQ_RABBITMQ_H_ */

