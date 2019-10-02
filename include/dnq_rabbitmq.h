#ifndef _DNQ_RABBITMQ_H_
#define _DNQ_RABBITMQ_H_

#include "common.h"

#define SERVER_IPADDR   "112.74.43.136"

typedef struct channel{
	int chid;
	char qname[32];
	char exchange[32];
	char rtkey[32];
}channel_t;


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
#define TYPE_STR_REBOOT           "reboot"

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
#define JSON_ITEM_TIMESETTINGS    "timesetting"
#define JSON_ITEM_START_TIME      "starttime"
#define JSON_ITEM_END_TIME        "endtime"
#define JSON_ITEM_DEGREES         "degrees"
#define JSON_ITEM_MAX             "max"
#define JSON_ITEM_MIN             "min"
#define JSON_ITEM_ERROR           "error"
#define JSON_ITEM_CONFIGS         "configs"
#define JSON_ITEM_POWER           "level"
#define JSON_ITEM_NUM             "num"
#define JSON_ITEM_RECTIFY         "rectify"
#define JSON_ITEM_CORRECT         "correct"
#define JSON_ITEM_ROOM_CNT        "room_cnt"

#define JSON_ITEM_PARTITION       "partation"
#define JSON_ITEM_NO              "no"
#define JSON_ITEM_MEMO            "memo"
#define JSON_ITEM_NAME            "name"
#define JSON_ITEM_IS_DELETE       "isDelete"
#define JSON_ITEM_PROJECT_ID      "projectId"
#define JSON_ITEM_BUILDING_ID     "buildingId"
#define JSON_ITEM_EQUIPMENT_ID    "equipmentId"
#define JSON_ITEM_PROJECT_NAME    "projectName"
#define JSON_ITEM_BUILDING_NAME   "buildingName"
#define JSON_ITEM_BUILD_POSITION  "buildPosition"
#define JSON_ITEM_HOST_NAME       "hostName"
#define JSON_ITEM_WORK_MODE       "jobMode"
#define JSON_ITEM_EQUIPMENT_MAC   "equipmentMac"

#define JSON_ITEM_ID              "id"
#define JSON_ITEM_ROOM_NAME       "roomName"
#define JSON_ITEM_ROOM_ID         "roomId"
#define JSON_ITEM_ROOM_ORDER      "roomOrder"
#define JSON_ITEM_ROOM_FLOOR      "roomFloor"
#define JSON_ITEM_ROOM_POSITION   "position"


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


S32 dnq_rabbitmq_init();
S32 dnq_rabbitmq_deinit();
U32 dnq_rabbitmq_link_is_ok();
S32 send_init_request_to_server();
S32 send_room_status_to_server();

#endif /* _DNQ_RABBITMQ_H_ */

