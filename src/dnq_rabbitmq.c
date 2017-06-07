/* dnq rabbitmq Program
 *
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 *
 *  this is a rabbitmq interface API, for app.
 * Note :
 */


#include "dnq_os.h"
#include "dnq_lcd.h"
#include "dnq_log.h"
#include "dnq_common.h"
#include "dnq_manage.h"
#include "dnq_network.h"
#include "dnq_rabbitmq.h"
#include "ngx_palloc.h"

#include "cJSON.h"
#include <amqp.h>
#include <amqp_framing.h>
#include <amqp_tcp_socket.h>

//#define MAC_ADDR   "70b3d5cf4924"
#define MAC_ADDR   dnq_get_mac_string()


/* recv channel: server to host */
#define CHNL_RX_QUEUE_NAME_PREFIX  "queue_host_"
#define CHNL_RX_EXCHANGE_NAME      "exchange_host"
#define CHNL_RX_ROUTEKEY_NAME      ""

/* send channel: host to server */
#define CHNL_TX_EXCHANGE_NAME  "queue_host_"

/* channel defined for rabbitmq  */
channel_t channels[10] = {
    /* server to host */
	{1,  "queue_host_",    "exchange_host",       ""}, 
	/* host to server */
	{2,  "state_",         "exchange_cloud",      "state"},
	{3,  "response_",      "exchange_cloud",      "callback"},
	{4,  "config_",        "exchange_cloud",      "config"},
	{5,  "warn_",          "exchange_cloud",      "warn"},
	{6,  "init_",          "exchange_cloud",      "init"},
	{7,  "_power",         "exchange_power",       ""}
};

static pthread_mutex_t rabbitmaq_mutex = PTHREAD_MUTEX_INITIALIZER;

#define rabbitmaq_lock()   pthread_mutex_lock(&rabbitmaq_mutex)
#define rabbitmaq_unlock() pthread_mutex_unlock(&rabbitmaq_mutex)

server_authorization_t g_authorization = {0};

server_temp_policy_t   g_temp_policy = 
{
    .rooms_cnt = DNQ_ROOM_MAX,
    .rooms = 
    {
        /* room_id, dpid, setting_cnt, setting[] */
        {0, 0, 1, {0, 60*60*24-1, 16}},
        {1, 0, 1, {0, 60*60*24-1, 18}},
        {2, 0, 1, {0, 60*60*24-1, 20}},
        {3, 0, 1, {0, 60*60*24-1, 22}},
        {4, 0, 1, {0, 60*60*24-1, 24}},
        {5, 0, 1, {0, 60*60*24-1, 25}},
        {6, 0, 1, {0, 60*60*24-1, 26}},
        {7, 0, 1, {0, 60*60*24-1, 27}},
        {8, 0, 1, {0, 60*60*24-1, 18}},
        {9, 0, 1, {0, 60*60*24-1, 19}},
        {10, 0, 1, {0, 60*60*24-1, 20}},
        {11, 0, 1, {0, 60*60*24-1, 21}},
        {12, 0, 1, {0, 60*60*24-1, 22}},
        {13, 0, 1, {0, 60*60*24-1, 23}},
        {14, 0, 1, {0, 60*60*24-1, 24}},
        {15, 0, 1, {0, 60*60*24-1, 30}}  
    },
};
server_temp_limit_t    g_temp_limit =
{
    .rooms_cnt = DNQ_ROOM_MAX,
    .rooms = 
    {
        /* room_id, max, min */
        {0, 20, 28},
        {1, 20, 28},
        {2, 20, 28},
        {3, 20, 28},
        {4, 20, 28},
        {5, 20, 28},
        {6, 20, 28},
        {7, 20, 28},
        {8, 20, 28},
        {9, 20, 28},
        {10, 20, 28},
        {11, 20, 28},
        {12, 20, 28},
        {13, 20, 28},
        {14, 20, 28},
        {15, 20, 28},
    }
};

server_temp_error_t    g_temp_error =
{
    .rooms_cnt = DNQ_ROOM_MAX,
    .rooms = 
    {
        /* room_id, error */
        {0, 4},
        {1, 4},
        {2, 4},
        {3, 4},
        {4, 4},
        {5, 4},
        {6, 4},
        {7, 4},
        {8, 4},
        {9, 4},
        {10, 4},
        {11, 4},
        {12, 4},
        {13, 4},
        {14, 4},
        {15, 4}
    }
};

server_power_config_t  g_power_config;
server_response_t      g_response;
server_temp_correct_t  g_temp_correct = 
{
    .rooms_cnt = DNQ_ROOM_MAX,
    .rooms = 
    {
        /* room_id, correct */
        {0, -2},
        {1, -2},
        {2, -2},
        {3, -1},
        {4, -1},
        {5, -2},
        {6, -2},
        {7, -2},
        {8, 1},
        {9, 1},
        {10, -1},
        {11, -1},
        {12, -1},
        {13, -1},
        {14, 2},
        {15, -2}
    }
};
server_init_info_t     g_init;

dnq_config_t g_dnq_config = 
{
    .inited = 0,
    .authorization = {0},
    .temp_policy = 
    {
        .rooms_cnt = DNQ_ROOM_MAX,
        .rooms = 
        {
            /* room_id, dpid, setting_cnt, setting[] */
            {0, 0, 1, {0, 60*60*24-1, 30}},
            {1, 0, 1, {0, 60*60*24-1, 32}},
            {2, 0, 1, {0, 60*60*24-1, 20}},
            {3, 0, 1, {0, 60*60*24-1, 22}},
            {4, 0, 1, {0, 60*60*24-1, 24}},
            {5, 0, 1, {0, 60*60*24-1, 25}},
            {6, 0, 1, {0, 60*60*24-1, 26}},
            {7, 0, 1, {0, 60*60*24-1, 27}},
            {8, 0, 1, {0, 60*60*24-1, 18}},
            {9, 0, 1, {0, 60*60*24-1, 19}},
            {10, 0, 1, {0, 60*60*24-1, 20}},
            {11, 0, 1, {0, 60*60*24-1, 21}},
            {12, 0, 1, {0, 60*60*24-1, 22}},
            {13, 0, 1, {0, 60*60*24-1, 23}},
            {14, 0, 1, {0, 60*60*24-1, 24}},
            {15, 0, 1, {0, 60*60*24-1, 30}}  
        },
    },
    .temp_limit = 
    {
        .rooms_cnt = DNQ_ROOM_MAX,
        .rooms = 
        {
            /* room_id, max, min */
            {0, 20, 28},
            {1, 20, 28},
            {2, 20, 28},
            {3, 20, 28},
            {4, 20, 28},
            {5, 20, 28},
            {6, 20, 28},
            {7, 20, 28},
            {8, 20, 28},
            {9, 20, 28},
            {10, 20, 28},
            {11, 20, 28},
            {12, 20, 28},
            {13, 20, 28},
            {14, 20, 28},
            {15, 20, 28},
        }
    },
    .temp_error = 
    {
        .rooms_cnt = DNQ_ROOM_MAX,
        .rooms = 
        {
            /* room_id, error */
            {0, 1},
            {1, 2},
            {2, 1},
            {3, 1},
            {4, 1},
            {5, 1},
            {6, 1},
            {7, 1},
            {8, 1},
            {9, 1},
            {10, 1},
            {11, 1},
            {12, 1},
            {13, 1},
            {14, 1},
            {15, 1}
        }
    },
    .power_config = {0},
    .response = {0},
    .temp_correct = 
    {
        .rooms_cnt = DNQ_ROOM_MAX,
        .rooms = 
        {
            /* room_id, correct */
            {0, -2},
            {1, -2},
            {2, -2},
            {3, -1},
            {4, -1},
            {5, -2},
            {6, -2},
            {7, -2},
            {8, 1},
            {9, 1},
            {10, -1},
            {11, -1},
            {12, -1},
            {13, -1},
            {14, 2},
            {15, -2}
        }
    },
    .init = {0},

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

#define YLOG  printf

#define copy_json_item_to_struct_item(obj, json, item_name, item_addr, item_type)  \
    do{\
        obj = cJSON_GetObjectItem(json, item_name);\
        if(!obj)\
        {\
            DNQ_ERROR(DNQ_MOD_RABBITMQ, "item \"%s\" not found!", item_name);\
            return -1;\
        }\
        if(obj->type != item_type)\
        {\
            DNQ_ERROR(DNQ_MOD_RABBITMQ, "item \"%s\" type=%d error, should be %d!", \
            item_name, obj->type, item_type);\
            return -1;\
        }\
        if(item_type == cJSON_String)\
        {\
            DNQ_DEBUG(DNQ_MOD_RABBITMQ, "%s:\t%s!", item_name, obj->valuestring);\
            strcpy((char*)item_addr, (char*)obj->valuestring); \
            break;\
        }\
        else if(item_type == cJSON_Number)\
        {\
            DNQ_DEBUG(DNQ_MOD_RABBITMQ, "%s:\t%d!", item_name, obj->valueint);\
            *(U16*)item_addr = obj->valueint;\
            break;\
        }\
    }while(0);\
    obj = NULL;

S32 dnq_config_save_file(U8 *file_name, U8 *data, U32 len);

S32 dnq_file_read(char *filepath, char *buffer, int len)
{
    S32    ret = 0;
    FILE  *fp = NULL;

    memset(buffer, 0, len);
    fp = fopen(filepath, "r");
    if(fp == NULL)
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "open file %s error! err=%s!", filepath, strerror(errno));
        return -1;
    }
    
    ret = fread(buffer, 1, len, fp);
    if(len < 0)
    {
        fclose(fp);
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "read error! err=%s!", filepath, strerror(errno));
        return -1;
    }
    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "read len=%d\n", ret);
    buffer[ret] = '\0';

    fclose(fp);
    return ret; 
}

S32 dnq_file_write(char *filepath, char *buffer, int len)
{
    S32    ret = 0;
    FILE  *fp = NULL;
    
    fp = fopen(filepath, "w+");
    if(fp == NULL)
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "open file %s error! err=%s!", filepath, strerror(errno));
        return -1;
    }
    
    ret = fwrite(buffer, 1, len, fp);
    if(len < 0)
    {
        fclose(fp);
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "write error! err=%s!", filepath, strerror(errno));
        return -1;
    }

    fclose(fp);
    return ret; 
}



S32 dnq_config_deinit(dnq_config_t *config)
{
    memset(config, 0, sizeof(dnq_config_t));
    return 0;
}

void dnq_config_print()
{
    U32 i = 0;
    U32 j = 0;

    dnq_config_t *config = &g_dnq_config;

    printf("=================================\n");
    
    DNQ_PRINT(DNQ_MOD_ALL, "====authorization config===\n");
    DNQ_PRINT(DNQ_MOD_ALL, "type=\t%s\n", config->authorization.type);
    DNQ_PRINT(DNQ_MOD_ALL, "time=\t%s\n", config->authorization.time);
    DNQ_PRINT(DNQ_MOD_ALL, "authorization=\t%s\n", config->authorization.authorization);

    DNQ_PRINT(DNQ_MOD_ALL, "====temp_policy config===\n");
    DNQ_PRINT(DNQ_MOD_ALL, "type=\t%s\n", config->temp_policy.type);
    DNQ_PRINT(DNQ_MOD_ALL, "time=\t%s\n", config->temp_policy.time);
    DNQ_PRINT(DNQ_MOD_ALL, "ctrl_id=\t%s\n", config->temp_policy.ctrl_id);
    DNQ_PRINT(DNQ_MOD_ALL, "mode=\t%d\n", config->temp_policy.mode);
    DNQ_PRINT(DNQ_MOD_ALL, "rooms_cnt=\t%d\n", config->temp_policy.rooms_cnt);
    
    for(i=0; i<config->temp_policy.rooms_cnt; i++)
    {
        DNQ_PRINT(DNQ_MOD_ALL, "id=\t%d\n", config->temp_policy.rooms[i].room_id);
        DNQ_PRINT(DNQ_MOD_ALL, "dpid=\t%d\n", config->temp_policy.rooms[i].dpid);
        DNQ_PRINT(DNQ_MOD_ALL, "setting_cnt=\t%d\n", config->temp_policy.rooms[i].time_setting_cnt);

        for(j=0; j<config->temp_policy.rooms[i].time_setting_cnt; j++)
        {
            DNQ_PRINT(DNQ_MOD_ALL, "start=\t%d\n", config->temp_policy.rooms[i].time_setting[j].start);
            DNQ_PRINT(DNQ_MOD_ALL, "starttime=\t%s\n", config->temp_policy.rooms[i].time_setting[j].starttime);
            DNQ_PRINT(DNQ_MOD_ALL, "end=\t%d\n", config->temp_policy.rooms[i].time_setting[j].end);
            DNQ_PRINT(DNQ_MOD_ALL, "endtime=\t%s\n", config->temp_policy.rooms[i].time_setting[j].endtime);
            DNQ_PRINT(DNQ_MOD_ALL, "degrees=\t%d\n", config->temp_policy.rooms[i].time_setting[j].degrees);
        }
    }
#if 0
    DNQ_PRINT(DNQ_MOD_ALL, "====temp_limit config===\n");
    DNQ_PRINT(DNQ_MOD_ALL, "type=\t%s\n", config->temp_limit.type);
    DNQ_PRINT(DNQ_MOD_ALL, "time=\t%s\n", config->temp_limit.time);
    DNQ_PRINT(DNQ_MOD_ALL, "ctrl_id=\t%s\n", config->temp_limit.ctrl_id);
    DNQ_PRINT(DNQ_MOD_ALL, "mode=\t%d\n", config->temp_limit.mode);
    
    for(i=0; i<config->temp_limit.rooms_cnt; i++)
    {
        DNQ_PRINT(DNQ_MOD_ALL, "id=\t%d\n", config->temp_limit.rooms[i].room_id);
        DNQ_PRINT(DNQ_MOD_ALL, "max=\t%d\n", config->temp_limit.rooms[i].max);
        DNQ_PRINT(DNQ_MOD_ALL, "min=\t%d\n", config->temp_limit.rooms[i].min);
    }

    //temp_error
    DNQ_PRINT(DNQ_MOD_ALL, "====temp_error config===\n");
    DNQ_PRINT(DNQ_MOD_ALL, "type=\t%s\n", config->temp_error.type);
    DNQ_PRINT(DNQ_MOD_ALL, "time=\t%s\n", config->temp_error.time);
    DNQ_PRINT(DNQ_MOD_ALL, "ctrl_id=\t%s\n", config->temp_error.ctrl_id);
    DNQ_PRINT(DNQ_MOD_ALL, "mode=\t%d\n", config->temp_error.mode);
    
    for(i=0; i<config->temp_error.rooms_cnt; i++)
    {
        DNQ_PRINT(DNQ_MOD_ALL, "id=\t%d\n", config->temp_error.rooms[i].room_id);
        DNQ_PRINT(DNQ_MOD_ALL, "error=\t%d\n", config->temp_error.rooms[i].error);
    }

    //temp_correct
    DNQ_PRINT(DNQ_MOD_ALL, "====temp_correct config===\n");
    DNQ_PRINT(DNQ_MOD_ALL, "type=\t%s\n", config->temp_correct.type);
    DNQ_PRINT(DNQ_MOD_ALL, "time=\t%s\n", config->temp_correct.time);
    DNQ_PRINT(DNQ_MOD_ALL, "ctrl_id=\t%s\n", config->temp_correct.ctrl_id);
    DNQ_PRINT(DNQ_MOD_ALL, "mode=\t%d\n", config->temp_correct.mode);
    
    for(i=0; i<config->temp_error.rooms_cnt; i++)
    {
        DNQ_PRINT(DNQ_MOD_ALL, "id=\t%d\n", config->temp_correct.rooms[i].room_id);
        DNQ_PRINT(DNQ_MOD_ALL, "correct=\t%d\n", config->temp_correct.rooms[i].correct);
    }
    #endif
    printf("=================================\n");
}

S32 dnq_config_load()
{
    U32  i;
    S32  ret = 0;
    U32  current_second;
    U8   gb2312_out[32] = {0};
    room_item_t *rooms = dnq_get_rooms();
    server_init_info_t *init_config;
    server_temp_policy_t *policy_config;
    server_temp_error_t  *temp_error;
    S32  setting_temp;
    
    ret = dnq_file_read(DNQ_DATA_FILE, &g_dnq_config, sizeof(dnq_config_t));
    if(ret < 0)
        return -1;

    init_config = &g_dnq_config.init;
    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        rooms[i].id = init_config->rooms[i].room_id;
        u2g(init_config->rooms[i].room_name, 16, gb2312_out,sizeof(gb2312_out));   
        rooms[i].correct = init_config->rooms[i].correct;
    }
    
    policy_config = &g_dnq_config.temp_policy;
    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        current_second = dnq_get_current_second();
        setting_temp = dnq_get_room_setting_temp_by_time(i, current_second);
        if(setting_temp == 0xFF)
        {
            setting_temp = 0;
        }
        rooms[i].set_temp = setting_temp*100;
        printf("rooms[%d].set_temp=%d\n", i, setting_temp);
    }
  
    return ret;
}

S32 dnq_data_file_create()
{
    S32 ret = 0;
    dnq_config_t *all_config = &g_dnq_config;
    ret |= dnq_file_write(DNQ_DATA_FILE, &g_dnq_config, sizeof(dnq_config_t));
    
#if 0
    ret |= dnq_config_save_file(JSON_FILE_AUTHORRIZATION, \
        all_config->authorization, sizeof(server_authorization_t));
    ret |= dnq_config_save_file(JSON_FILE_POLICY, \
        all_config->temp_policy, sizeof(server_temp_policy_t));
    ret |= dnq_config_save_file(JSON_FILE_LIMIT, \
        all_config->temp_limit, sizeof(server_temp_limit_t));
    ret |= dnq_config_save_file(JSON_FILE_ERROR, \
        all_config->temp_error, sizeof(server_temp_error_t));
    ret |= dnq_config_save_file(JSON_FILE_POWER, \
        all_config->power_config, sizeof(server_power_config_t));
    ret |= dnq_config_save_file(JSON_FILE_RESPONSE, \
        all_config->response, sizeof(server_response_t));
    ret |= dnq_config_save_file(JSON_FILE_CORRECT, \
        all_config->temp_correct, sizeof(server_temp_correct_t));
    ret |= dnq_config_save_file(JSON_FILE_INIT, \
        all_config->init, sizeof(server_init_info_t));
#endif
    if(ret != -1)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "create defult data success!!!");
        remove(DNQ_DATA_FILE);
    }
    else
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "create defult data failed!!!");
    return ret;
}

S32 dnq_data_file_save()
{
    return dnq_file_write(DNQ_DATA_FILE, &g_dnq_config, sizeof(dnq_config_t));
}

S32 dnq_config_load1()
{
    U8  filepath[128] = {0};
    U8  buffer[1024];
    U8  cjson_struct[3072] = {0};
    S32 ret;
    S32 type;

    dnq_config_t *config = &g_dnq_config;
    
    sprintf(filepath, "%s/%s", DNQ_CONFIG_PATH, JSON_FILE_AUTHORRIZATION);
    ret = dnq_file_read(filepath, buffer, sizeof(buffer));
    type = json_parse(buffer, config->authorization);

    sprintf(filepath, "%s/%s", DNQ_CONFIG_PATH, JSON_FILE_POLICY);
    ret = dnq_file_read(filepath, buffer, sizeof(buffer));
    type = json_parse(buffer, config->temp_policy);

    sprintf(filepath, "%s/%s", DNQ_CONFIG_PATH, JSON_FILE_LIMIT);
    ret = dnq_file_read(filepath, buffer, sizeof(buffer));
    type = json_parse(buffer, config->temp_limit);

    sprintf(filepath, "%s/%s", DNQ_CONFIG_PATH, JSON_FILE_ERROR);
    ret = dnq_file_read(filepath, buffer, sizeof(buffer));
    type = json_parse(buffer, config->temp_error);

    sprintf(filepath, "%s/%s", DNQ_CONFIG_PATH, JSON_FILE_POWER);
    ret = dnq_file_read(filepath, buffer, sizeof(buffer));
    type = json_parse(buffer, config->power_config);

    sprintf(filepath, "%s/%s", DNQ_CONFIG_PATH, JSON_FILE_RESPONSE);
    ret = dnq_file_read(filepath, buffer, sizeof(buffer));
    type = json_parse(buffer, config->response);

    sprintf(filepath, "%s/%s", DNQ_CONFIG_PATH, JSON_FILE_CORRECT);
    ret = dnq_file_read(filepath, buffer, sizeof(buffer));
    type = json_parse(buffer, config->temp_correct);

    sprintf(filepath, "%s/%s", DNQ_CONFIG_PATH, JSON_FILE_INIT);
    ret = dnq_file_read(filepath, buffer, sizeof(buffer));
    type = json_parse(buffer, config->init);

    config->inited = 1;
    /* un finished...... */

    DNQ_INFO(DNQ_MOD_RABBITMQ, "load all config!!!");
    
    return 0;
}

S32 dnq_config_init()
{
    S32 ret = 0;

    if((ret=access(DNQ_DATA_FILE, F_OK)) < 0)
    {
        ret = dnq_data_file_create(); 
    }
    else
    {
        dnq_config_load();
    }

    printf("$$$$$ret=%d, config size=%d\n", ret, sizeof(dnq_config_t));
    
    return 0;
}

server_authorization_t*
    dnq_get_authorization_config(server_authorization_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.authorization, sizeof(server_authorization_t));
    return &g_dnq_config.authorization;
}

server_temp_policy_t* 
    dnq_get_temp_policy_config(server_temp_policy_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.temp_policy, sizeof(server_temp_policy_t));
    return &g_dnq_config.temp_policy;
}

server_temp_limit_t* 
    dnq_get_temp_limit_config(server_temp_limit_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.temp_limit, sizeof(server_temp_limit_t));
    return &g_dnq_config.temp_limit;
}

server_temp_error_t*
    dnq_get_temp_error_config(server_temp_error_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.temp_error, sizeof(server_temp_error_t));
    return &g_dnq_config.temp_error;
}

server_power_config_t*
    dnq_get_power_config_config(server_power_config_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.power_config, sizeof(server_power_config_t));
    return &g_dnq_config.power_config;
}

server_response_t*
    dnq_get_response_config(server_response_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.response, sizeof(server_response_t));
    return &g_dnq_config.response;
}

server_temp_correct_t*
    dnq_get_temp_correct_config(server_temp_correct_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.temp_correct, sizeof(server_temp_correct_t));
    return &g_dnq_config.temp_correct;
}

server_init_info_t*
    dnq_get_init_config(server_init_info_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.init, sizeof(server_init_info_t));
    return &g_dnq_config.init;
}

S32 dnq_config_update_authorization(void *cjson_struct)
{
    server_authorization_t *authorization = NULL;
    authorization = (server_authorization_t *)cjson_struct;
    
    return 0;
}

S32 dnq_config_update_temp_policy(void *cjson_struct)
{
    server_temp_policy_t *temp_policy, curr_policy;
    temp_policy = (server_temp_policy_t *)cjson_struct;
    U32 i, room_id;

    dnq_get_temp_policy_config(&curr_policy);
    /* single */
    if(temp_policy->mode == 1)
    {
        room_id = temp_policy->rooms[0].room_id - 1;
        memcpy(&curr_policy.rooms[room_id],\
            &temp_policy->rooms[0], sizeof(room_temp_policy_t));
        
    }
    else if(temp_policy->mode == 0) /* whole all config */
    {
        for(i=0; i<curr_policy.rooms_cnt; i++)
        {
            memcpy(&curr_policy.rooms[i], \
                &temp_policy->rooms[0], sizeof(room_temp_policy_t));
        }
    }

    DNQ_INFO(DNQ_MOD_RABBITMQ, "update config!");
    
    return 0;
}

S32 dnq_config_update_temp_limit(void *cjson_struct)
{
    server_temp_limit_t *temp_limit;
    temp_limit = (server_temp_limit_t *)cjson_struct;
    
    return 0;
}

S32 dnq_config_update_temp_error(void *cjson_struct)
{
    server_temp_error_t *temp_error;
    temp_error = (server_temp_error_t *)cjson_struct;
    
    return 0;
}

S32 dnq_config_update_power_config(void *cjson_struct)
{
    server_power_config_t *power_config;
    power_config = (server_power_config_t *)cjson_struct;
    
    return 0;
}

S32 dnq_config_update_response(void *cjson_struct)
{
    server_response_t *response;
    response = (server_response_t *)cjson_struct;
    
    return 0;
}

S32 dnq_config_update_temp_correct(void *cjson_struct)
{
    server_temp_correct_t *temp_correct;
    temp_correct = (server_temp_correct_t *)cjson_struct;
    
    return 0;
}


S32 dnq_config_update_init_info(void *cjson_struct)
{
    server_init_info_t *init_info;
    init_info = (server_init_info_t *)cjson_struct;
    
    return 0;
}

S32 dnq_config_update(json_type_e type, void *cjson_struct)
{
    S32 ret = -1;
    switch(type)
    {
        case JSON_TYPE_AUTHORRIZATION:
            ret = dnq_config_update_authorization(cjson_struct);
        break;
        case JSON_TYPE_TEMP_POLICY:
            ret = dnq_config_update_temp_policy(cjson_struct);
        break;
        case JSON_TYPE_TEMP_LIMIT:
            ret = dnq_config_update_temp_limit(cjson_struct);
        break;
        case JSON_TYPE_TEMP_ERROR:
            ret = dnq_config_update_temp_error(cjson_struct);
        break;
        case JSON_TYPE_POWER_CONFIG:
            ret = dnq_config_update_power_config(cjson_struct);
        break;
        case JSON_TYPE_RESPONSE:
            ret = dnq_config_update_response(cjson_struct);
        break;
        case JSON_TYPE_CORRECT:
            ret = dnq_config_update_temp_correct(cjson_struct);
        break;
        case JSON_TYPE_INIT:
            ret = dnq_config_update_init_info(cjson_struct);     
        break;
        default:
            break;
    
    }
    return ret;
}

S32 dnq_json_save_file(U8 *file_name, U8 *data, U32 len)
{
    U32 i = 0;
    U8 filepath[128] = {0};
    S32 ret;
        
    for(i=0; i<3; i++)
    {
        sprintf(filepath, "%s/%s", DNQ_CONFIG_PATH, file_name);
        ret = dnq_file_write(filepath, data, len);
        if(ret == len)
            break;
        dnq_msleep(200);
    }
    
    return (ret==len)?0:-1;
}

S32 dnq_config_sync_to_lcd(json_type_e json_type, void *cjson_struct)
{
    U32 i;
    S32 ret;
    U32 current_second, setting_temp;
    dnq_msg_t msg = {0};
    server_temp_policy_t *temp_policy = (server_temp_policy_t *)cjson_struct;
    msg.Class = MSG_CLASS_RABBITMQ;
    
    switch(json_type)
    {
        case JSON_TYPE_AUTHORRIZATION:
            break;
        case JSON_TYPE_TEMP_POLICY:
            temp_policy = (server_temp_policy_t *)cjson_struct;
            current_second = dnq_get_current_second();
            for(i=0; i<temp_policy->rooms_cnt; i++)
            {
                setting_temp = dnq_get_room_setting_temp_by_time(temp_policy->rooms[i].room_id-1, current_second);

                if(setting_temp != 0xFF)
                {
                    msg.code = MQ_MSG_TYPE_SET_TEMP_UPDATE;
                    msg.lenght = temp_policy->rooms[i].room_id-1;
                    msg.payload = (void*)(setting_temp*100);
                    send_msg_to_lcd(&msg);
                }
            }
            
            break;
        case JSON_TYPE_TEMP_LIMIT:
            break;
        case JSON_TYPE_TEMP_ERROR:

            break;
        case JSON_TYPE_POWER_CONFIG:
            break;
        case JSON_TYPE_RESPONSE:         
            break;
        case JSON_TYPE_CORRECT:
            break;
        case JSON_TYPE_INIT:
            break;
        default:
            break;
    }
}

S32 dnq_config_check_and_sync(json_type_e json_type, U8 *json_data, U32 len, void *cjson_struct)
{
    
    S32 ret = 0;
    U32 i, size = 0;
    U32 room_id;
    dnq_msg_t msg = {0};
    switch(json_type)
    {
        case JSON_TYPE_AUTHORRIZATION:
            ret = dnq_config_update(JSON_TYPE_AUTHORRIZATION, cjson_struct);
            if(!ret)
            {
                memcpy(&g_dnq_config.authorization, cjson_struct, sizeof(server_authorization_t));
                dnq_json_save_file(JSON_FILE_AUTHORRIZATION, json_data, len);
            }
        break;
        case JSON_TYPE_TEMP_POLICY:
            ret = dnq_config_update(JSON_TYPE_TEMP_POLICY, cjson_struct);
            if(!ret)
            {
                server_temp_policy_t *temp_policy = (server_temp_policy_t *)cjson_struct;
                size = ((U32)&temp_policy->rooms_cnt-(U32)temp_policy)+\
                    temp_policy->rooms_cnt*sizeof(room_temp_policy_t);

                for(i=0; i<temp_policy->rooms_cnt; i++)
                {
                    room_id = temp_policy->rooms[i].room_id -1;
                    if(room_id >= 0 && room_id <= DNQ_ROOM_MAX)
                    {
                        memcpy(&g_dnq_config.temp_policy.rooms[room_id],\
                        &temp_policy->rooms[i], sizeof(room_temp_policy_t));
                    }
                }
                //memcpy(&g_dnq_config.temp_policy, cjson_struct, size);
                dnq_json_save_file(JSON_FILE_POLICY, json_data, len);
            }
            
        break;
        case JSON_TYPE_TEMP_LIMIT:
            ret = dnq_config_update(JSON_TYPE_TEMP_LIMIT, cjson_struct);
            if(!ret)
            {
                memcpy(&g_dnq_config.temp_limit, cjson_struct, sizeof(server_temp_limit_t));
                dnq_json_save_file(JSON_FILE_LIMIT, json_data, len);
            }
        break;
        case JSON_TYPE_TEMP_ERROR:
            ret = dnq_config_update(JSON_TYPE_TEMP_ERROR, cjson_struct);
            if(!ret)
            {
                server_temp_error_t *temp_error = (server_temp_error_t *)cjson_struct;

                for(i=0; i<temp_error->rooms_cnt; i++)
                {
                    room_id = temp_error->rooms[i].room_id - 1;
                    if(room_id >= 0 && room_id <= DNQ_ROOM_MAX)
                    {
                        g_dnq_config.temp_error.rooms[room_id].error = temp_error->rooms[i].error;
                    }
                }
                
                dnq_json_save_file(JSON_FILE_ERROR, json_data, len);
            }
        break;
        case JSON_TYPE_POWER_CONFIG:
            
            ret = dnq_config_update(JSON_TYPE_POWER_CONFIG, cjson_struct);
            if(!ret)
            {
                memcpy(&g_dnq_config.power_config, cjson_struct, sizeof(server_power_config_t));
                dnq_json_save_file(JSON_FILE_POWER, json_data, len);
            }
        break;
        case JSON_TYPE_RESPONSE:
            
            ret = dnq_config_update(JSON_TYPE_RESPONSE, cjson_struct);
            if(!ret)
            {
                memcpy(&g_dnq_config.response, cjson_struct, sizeof(server_response_t));
                dnq_json_save_file(JSON_FILE_RESPONSE, json_data, len);
            }
        break;
        case JSON_TYPE_CORRECT:
            ret = dnq_config_update(JSON_TYPE_CORRECT, cjson_struct);
            if(!ret)
            {
                memcpy(&g_dnq_config.temp_correct, cjson_struct, sizeof(server_temp_correct_t));
                dnq_json_save_file(JSON_FILE_CORRECT, json_data, len);
            }
        break;
        case JSON_TYPE_INIT:
            
            ret = dnq_config_update(JSON_TYPE_INIT, cjson_struct);
            if(!ret)
            {
                memcpy(&g_dnq_config.init, cjson_struct, sizeof(server_init_info_t));
                dnq_json_save_file(JSON_FILE_INIT, json_data, len);
                //dnq_data_file_save();
            }
        break;
        default:
            DNQ_ERROR(DNQ_MOD_RABBITMQ, "unknown json type: %d", json_type);
            break;
    
    }

    if(ret == 0)
        dnq_data_file_save();
    dnq_config_sync_to_lcd(json_type, cjson_struct);
    return ret;
}
    
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

S32 json_parse_authorization_manage(cJSON *pjson, server_authorization_t *pdst)
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

S32 timestring_to_second(U8 *timestring)
{
    U32 second = 0;
    timestring[2] = '\0';
    timestring[5] = '\0';
    timestring[8] = '\0';

   printf("timestring=%s, second=%d\n", timestring, second);
    second = atoi(timestring)*3600+atoi(timestring+3)*60+atoi(timestring+6);
     printf("timestring=%s, second=%d\n", timestring, second);
    return second;
}

S32 json_parse_temp_policy(cJSON *pjson, server_temp_policy_t *pdst)
{
	int     i = 0;
    int     j = 0;
    U32     second;
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

    //mode
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_MODE, &pdst->mode, cJSON_Number);

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

    /* mode value:   0:ctrl_whole,  1:ctrl_single */
    if((pdst->mode == CTRL_WHOLE_ROOM && pdst->rooms_cnt == 1)\
        ||(pdst->mode == CTRL_SINGLE_ROOM && pdst->rooms_cnt > 1))
    {
        DNQ_WARN(DNQ_MOD_RABBITMQ, "mode=%s, but room cnt=%d", \
        pdst->mode?"ctrl_single":"ctrl_whole", pdst->rooms_cnt);
    }

    //rooms array
    for(i=0; i<pdst->rooms_cnt; i++)
    {
        //item from rooms array
        room_obj = cJSON_GetArrayItem(rooms, i);

        //room ID
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ROOMID, &pdst->rooms[i].room_id, cJSON_Number);

        //dpid ID
        //copy_json_item_to_struct_item(\
        //obj, room_obj, JSON_ITEM_DPID, &pdst->rooms[i].dpid, cJSON_Number);

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

            second = timestring_to_second(pdst->rooms[i].time_setting[j].starttime);
            pdst->rooms[i].time_setting[j].start = second;
            DNQ_INFO(DNQ_MOD_RABBITMQ, "starttime:\t%s, =%d second!", \
                pdst->rooms[i].time_setting[j].starttime, second);
            
            //end time
            copy_json_item_to_struct_item(\
                obj, timesettings_obj, JSON_ITEM_END_TIME, pdst->rooms[i].time_setting[j].endtime, cJSON_String);
            
            second = timestring_to_second(pdst->rooms[i].time_setting[j].endtime);
            pdst->rooms[i].time_setting[j].end= second;
            DNQ_INFO(DNQ_MOD_RABBITMQ, "endtime:\t%s, =%d second!", \
                pdst->rooms[i].time_setting[j].endtime, second);
            
            //degrees
            copy_json_item_to_struct_item(\
                obj, timesettings_obj, JSON_ITEM_DEGREES, &pdst->rooms[i].time_setting[j].degrees, cJSON_Number);
            DNQ_INFO(DNQ_MOD_RABBITMQ, "degrees:\t%d!", pdst->rooms[i].time_setting[j].degrees);
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
S32 json_parse_temp_limit(cJSON *pjson, server_temp_limit_t *pdst)
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

    /* mode value:   0:ctrl_whole,  1:ctrl_single */
    if((pdst->mode == CTRL_WHOLE_ROOM && pdst->rooms_cnt == 1)\
        ||(pdst->mode == CTRL_SINGLE_ROOM && pdst->rooms_cnt > 1))
    {
        DNQ_WARN(DNQ_MOD_RABBITMQ, "mode=%s, but room cnt=%d", \
        pdst->mode?"ctrl_single":"ctrl_whole", pdst->rooms_cnt);
    }
    
    //rooms array
    for(i=0; i<pdst->rooms_cnt; i++)
    {
        //item from rooms array
        room_obj = cJSON_GetArrayItem(rooms, i);

        //room ID
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ROOMID, &pdst->rooms[i].room_id, cJSON_Number);

        //max
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_MAX, &pdst->rooms[i].max, cJSON_Number);

        //min
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_MIN, &pdst->rooms[i].min, cJSON_Number);
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
S32 json_parse_degree_error(cJSON * pjson, server_temp_error_t *pdst)
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

    /* mode value:   0:ctrl_whole,  1:ctrl_single */
    if((pdst->mode == CTRL_WHOLE_ROOM && pdst->rooms_cnt == 1)\
        ||(pdst->mode == CTRL_SINGLE_ROOM && pdst->rooms_cnt > 1))
    {
        DNQ_WARN(DNQ_MOD_RABBITMQ, "mode=%s, but room cnt=%d", \
        pdst->mode?"ctrl_single":"ctrl_whole", pdst->rooms_cnt);
    }
    
    //rooms array
    for(i=0; i<pdst->rooms_cnt; i++)
    {
        //item from rooms array
        room_obj = cJSON_GetArrayItem(rooms, i);

        //room ID
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ROOMID, &pdst->rooms[i].room_id, cJSON_Number);

        //degree error
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ERROR, &pdst->rooms[i].error, cJSON_Number);
    }

    return 0;
}

/*
*  server --> controller
*  parse a "power_config" message (json data)
*
*  2.5 云端向控制器发送 <设置每个房间电暖气个数，及功率配置>
*  解析一个 "功率配置" 消息结构(json)
*
*/
S32 json_parse_power_config(cJSON *pjson, server_power_config_t *pdst)
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

    /* mode value:   0:ctrl_whole,  1:ctrl_single */
    if((pdst->mode == CTRL_WHOLE_ROOM && pdst->rooms_cnt == 1)\
        ||(pdst->mode == CTRL_SINGLE_ROOM && pdst->rooms_cnt > 1))
    {
        DNQ_WARN(DNQ_MOD_RABBITMQ, "mode=%s, but room cnt=%d", \
        pdst->mode?"ctrl_single":"ctrl_whole", pdst->rooms_cnt);
    }
    
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
S32 json_parse_response(cJSON *pjson, server_response_t *pdst)
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
    obj, pjson, JSON_ITEM_STATUS, pdst->status, cJSON_String);
    
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
S32 json_parse_correct(cJSON *pjson, server_temp_correct_t *pdst)
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
    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "%s array's size=%d!", JSON_ITEM_ROOMS, pdst->rooms_cnt);

    /* mode value:   0:ctrl_whole,  1:ctrl_single */
    if((pdst->mode == CTRL_WHOLE_ROOM && pdst->rooms_cnt == 1)\
        ||(pdst->mode == CTRL_SINGLE_ROOM && pdst->rooms_cnt > 1))
    {
        DNQ_WARN(DNQ_MOD_RABBITMQ, "mode=%s, but room cnt=%d", \
        pdst->mode?"ctrl_single":"ctrl_whole", pdst->rooms_cnt);
    }
    
    //rooms array
    for(i=0; i<pdst->rooms_cnt; i++)
    {
        //item from rooms array
        room_obj = cJSON_GetArrayItem(rooms, i);

        //room ID
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ROOMID, &pdst->rooms[i].room_id, cJSON_Number);

        //rectify
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_RECTIFY, &pdst->rooms[i].correct, cJSON_Number);
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
S32 json_parse_init(cJSON *pjson, server_init_info_t *pdst)
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
    //buildingId
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_BUILDING_ID, &pdst->building_id, cJSON_Number);
    //equipmentId
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_EQUIPMENT_ID, &pdst->equipment_id, cJSON_Number);
    //projectName
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_PROJECT_NAME, pdst->project_name, cJSON_String);
    //buildingName
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_BUILDING_NAME, pdst->building_name, cJSON_String);
    //equipmentMac
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_EQUIPMENT_MAC, pdst->equipment_mac, cJSON_String);
    //time
    copy_json_item_to_struct_item(\
    obj, pjson, JSON_ITEM_TIME, pdst->time, cJSON_String);

    //rooms
    rooms = cJSON_GetObjectItem(pjson, JSON_ITEM_ROOMS);
    if(!cJSON_IsArray(rooms))
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "item %s must be a array!", JSON_ITEM_ROOMS);
        return -1;
    }

    //rooms size
    pdst->rooms_cnt = cJSON_GetArraySize(rooms);
    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "%s array's size=%d!", JSON_ITEM_ROOMS, pdst->rooms_cnt);

    //rooms array
    for(i=0; i<pdst->rooms_cnt; i++)
    {
        //item from rooms array
        room_obj = cJSON_GetArrayItem(rooms, i);
        //error
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ERROR, &pdst->rooms[i].error, cJSON_Number);
        
        //max
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_MAX, &pdst->rooms[i].max, cJSON_Number);
        //min
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_MIN, &pdst->rooms[i].min, cJSON_Number);
        //correct
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_CORRECT, &pdst->rooms[i].correct, cJSON_Number);
        //room_id
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ROOM_ID, &pdst->rooms[i].room_id, cJSON_Number);
        //room_name
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ROOM_NAME, pdst->rooms[i].room_name, cJSON_String);
        //room_order
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ROOM_ORDER, &pdst->rooms[i].room_order, cJSON_Number);
        //room_floor
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ROOM_ORDER, &pdst->rooms[i].room_floor, cJSON_Number);
        
        //position
        copy_json_item_to_struct_item(\
        obj, room_obj, JSON_ITEM_ROOM_POSITION, pdst->rooms[i].position, cJSON_String);
    }

    /* 标识  已经收到初始化信息。 */
    g_init.inited = 1;
    printf("!!!!!!!!!!!g_init.inited=%d\n",g_init.inited);
    
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
S32 json_parse(char *json,void *cjson_struct)
{
    S32 ret;
    cJSON *pjson = NULL;
    cJSON *item = NULL;
    char  *type = NULL;
    json_type_e  msg_type = -1;

    //char buffer[] = "[123,34,112,97,1,-9,-19";
    //json = buffer;
    
    if(*json != '{')
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "json first byte must be '{' !!");
        return -1;
    }

    pjson = cJSON_Parse(json);
    if(!pjson)
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "json parse failed!");
        return -1;
    }

    item = cJSON_GetObjectItem(pjson, "type");
    DNQ_INFO(DNQ_MOD_RABBITMQ, "json type=%s", item->valuestring);

    if(cJSON_IsString(item))
    {
        type = item->valuestring;
        if(strcmp(type, TYPE_STR_AUTHORRIZATION) == 0)
        {
            ret = json_parse_authorization_manage(pjson, cjson_struct);
            if(ret == 0)
                msg_type = JSON_TYPE_AUTHORRIZATION;
        }
        else if(strcmp(type, TYPE_STR_TEMP_POLICY) == 0)
        {
            ret = json_parse_temp_policy(pjson, cjson_struct);
            if(ret == 0)
                msg_type = JSON_TYPE_TEMP_POLICY;
        }
        else if(strcmp(type, TYPE_STR_TEMP_LIMIT) == 0)
        {
            ret = json_parse_temp_limit(pjson, cjson_struct);
            if(ret == 0)
                msg_type = JSON_TYPE_TEMP_LIMIT;
        }
        else if(strcmp(type, TYPE_STR_DEGREE_ERROR) == 0)
        {
            ret = json_parse_degree_error(pjson, cjson_struct);
            if(ret == 0)
                msg_type = JSON_TYPE_TEMP_ERROR;
        }
        else if(strcmp(type, TYPE_STR_POWER_CONFIG) == 0)
        {
            ret = json_parse_power_config(pjson, cjson_struct);
            if(ret == 0)
                msg_type = JSON_TYPE_POWER_CONFIG;
        }
        else if(strcmp(type, TYPE_STR_RESPONSE) == 0)
        {
            ret = json_parse_response(pjson, cjson_struct);
            if(ret == 0)
                msg_type = JSON_TYPE_RESPONSE;
        }
        else if(strcmp(type, TYPE_STR_CORRECT) == 0)
        {
            ret = json_parse_correct(pjson, cjson_struct);
            if(ret == 0)
                msg_type = JSON_TYPE_CORRECT;
        }
        else if(strcmp(type, TYPE_STR_INIT) == 0)
        {
            ret = json_parse_init(pjson, cjson_struct);
            if(ret == 0)
                msg_type = JSON_TYPE_INIT;
        }
        else
        {
            msg_type = -1;
            DNQ_ERROR(DNQ_MOD_RABBITMQ, "unkown msg! type=[%s]", item->valuestring);
        }
    }
    else
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "Error: type must be string!");
    }
    
    cJSON_Delete(pjson); 

    printf("msg_type=%d\n", msg_type); 

    if(msg_type < 0)
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "json parse failed!");
    else
        DNQ_INFO(DNQ_MOD_RABBITMQ, "json parse success!");
    
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
        cJSON_AddNumberToObject(room_obj, "degree",  pdst->rooms[i].degree);
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
        cJSON_AddNumberToObject(pjson, "loss",  pdst->rooms[i].loss);
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

    cJSON_AddStringToObject(pjson, "type", pdst->type);
    cJSON_AddStringToObject(pjson, "mac", pdst->mac);
    cJSON_AddStringToObject(pjson, "status", pdst->status);

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
        cJSON_AddNumberToObject(room_obj, "correct",  pdst->rooms[i].correct);
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
        cJSON_AddNumberToObject(room_obj, "degree",  pdst->rooms[i].degree);
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

char *json_create_init(U8 *MAC)
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

    cJSON_AddStringToObject(pjson, "mac",  MAC);

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


U32 dnq_rabbitmq_link_is_ok()
{
    return g_conn?1:0;
}

/* unused */
int send_response_msg_to_server(
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
    rabbitmaq_lock();
	die_on_error(amqp_basic_publish(conn,
	                                pchnl->chid,
	                                amqp_cstring_bytes(pchnl->exchange),
	                                amqp_cstring_bytes(pchnl->rtkey),
	                                0,
	                                0,
	                                &props,
	                                amqp_cstring_bytes(response)),
	             "Publishing1");
    rabbitmaq_unlock();
    return 0;
}

S32 send_msg_to_server(
    amqp_connection_state_t conn,
    channel_t *pchnl,
    char *message)
{
    S32 ret = -1;
	amqp_basic_properties_t props;
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	props.content_type = amqp_cstring_bytes("text/plain");
	props.delivery_mode = 2; /* persistent delivery mode */
	//snprintf(response_buffer,128, "50#%s#%s#%s",data);
    
    if(!conn)
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "can't send msg! rabbitmq link is down!");
        return -1;
    }
    rabbitmaq_lock();
	ret = die_on_error(amqp_basic_publish(conn,
	                                pchnl->chid,
	                                amqp_cstring_bytes(pchnl->exchange),
	                                amqp_cstring_bytes(pchnl->rtkey),
	                                0,
	                                0,
	                                &props,
	                                amqp_cstring_bytes(message)),
	             "Publishing");
    rabbitmaq_unlock();
    if(ret == 0)
        DNQ_INFO(DNQ_MOD_RABBITMQ, "send success! chid=%d, ex=%s, key=%s",\
        pchnl->chid, pchnl->exchange, pchnl->rtkey);
    else
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "send failed! chid=%d, ex=%s, key=%s",\
        pchnl->chid, pchnl->exchange, pchnl->rtkey);
    return ret;
}

S32 send_response_to_server(
    amqp_connection_state_t conn,
    channel_t *pchnl,
    json_type_e json_type)
{
    S32 ret = -1;
    char  *json_response = NULL;
    client_response_t response;

    /* send response to server */
    if(json_type == JSON_TYPE_AUTHORRIZATION)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: authorization");

        strcpy(response.type, TYPE_STR_AUTHORRIZATION);
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "authorization");
        
        json_response = json_create_response(&response);
        if(!json_response)
            return -1;
        ret = send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(json_type == JSON_TYPE_TEMP_POLICY)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: policy");
        
        strcpy(response.type, TYPE_STR_TEMP_POLICY);
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "policy");
        json_response = json_create_response(&response);
        if(!json_response)
            return -1;
        ret = send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(json_type == JSON_TYPE_TEMP_LIMIT)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: limit");
        strcpy(response.type, TYPE_STR_TEMP_LIMIT);
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "limit");
        
        json_response = json_create_response(&response);
        if(!json_response)
            return -1;
        ret = send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(json_type == JSON_TYPE_TEMP_ERROR)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: degree error");
        strcpy(response.type, TYPE_STR_DEGREE_ERROR);
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "error");
        json_response = json_create_response(&response);
        if(!json_response)
            return -1;
        ret = send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(json_type == JSON_TYPE_POWER_CONFIG)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: power config");
        strcpy(response.type, TYPE_STR_POWER_CONFIG);
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "power");
        
        json_response = json_create_response(&response);
        if(!json_response)
            return -1;
        ret = send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(json_type == JSON_TYPE_RESPONSE)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: response");
    }
    else if(json_type == JSON_TYPE_CORRECT)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: correct");
        strcpy(response.type, TYPE_STR_CORRECT);
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "correct");
        
        json_response = json_create_response(&response);
        if(!json_response)
            return -1;
        ret = send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(json_type == JSON_TYPE_INIT)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: init");
        strcpy(response.type, TYPE_STR_INIT);
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "init");
        
        json_response = json_create_response(&response);
        if(!json_response)
            return -1;
        ret = send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }

    return ret;
}

S32 send_room_status_to_server(client_status_t *client_status)
{
    S32 i, ret;
    U8 *msg = NULL;
    client_status_t status= {0};
    room_item_t *rooms = dnq_get_rooms();
    host_net_info_t *netinfo = dnq_get_netinfo();
    
    strcpy(status.mac, netinfo->mac_str);
    status.rooms_cnt = DNQ_ROOM_CNT;
    for(i=0; i<status.rooms_cnt; i++)
    {
        status.rooms[i].room_id = i;
        status.rooms[i].degree = rooms[i].curr_temp;
        status.rooms[i].loss =  47*i*200;
    }

    msg = json_create_status(&status);
    ret = send_msg_to_server(g_conn, &channels[1], msg);
    dnq_free(msg);
    DNQ_INFO(DNQ_MOD_RABBITMQ, "send status msg:\n%s", msg);
    
    return ret;
}

S32 send_init_request_to_server()
{
    S32 ret = 0;
    U8 *msg = NULL;
    U8  mac_addr[8] = {0};
    U8  mac_str[16] = {0};

    //ret = dnq_net_get_macaddr(ETH_NAME, mac_addr);
    if(ret < 0)
        return -1;
    
    sprintf(mac_str, "%02x%02x%02x%02x%02x%02x", \
        mac_addr[0], mac_addr[1], mac_addr[2],\
        mac_addr[3], mac_addr[4], mac_addr[5]);
 //
    //msg = json_create_init("22");
    msg = json_create_init(MAC_ADDR);
    send_msg_to_server(g_conn, &channels[5], msg);
    dnq_free(msg);
    DNQ_INFO(DNQ_MOD_RABBITMQ, "send init msg:\n%s", msg);
    
    return ret;
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

U32 msg_process(amqp_envelope_t *penve, amqp_connection_state_t conn)
{
    S32 ret = -1;
    char  *json_msg = NULL;
    char   cjson_struct[3072] = {0};
    char  *json_response = NULL;
    json_type_e  json_type = 0;
    channel_t   *pchnl = NULL;
    dnq_msg_t  sendmsg;
    U32    json_len;

    pchnl = &channels[2]; /* response  */
    json_msg = penve->message.body.bytes;
    json_len = penve->message.body.len;

    json_type = json_parse(json_msg, cjson_struct);
    if(json_type < 0)
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "json parse error!");
        return -1;
    }

    DNQ_INFO(DNQ_MOD_RABBITMQ, "json_parse: msg_type=%d", json_type);
    ret = dnq_config_check_and_sync(json_type, json_msg, json_len, cjson_struct);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "config data check error!");
        return -1;
    }

    /* send response to server */
    ret = send_response_to_server(conn, pchnl, json_type);
    if(ret < 0)
        return -1;
    
    sendmsg.Class = MSG_CLASS_RABBITMQ;
    sendmsg.code = json_type;
    
    send_msg_to_lcd(&sendmsg);

    return 0;
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
    struct timeval timeout;

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
        rabbitmaq_lock();
        amqp_maybe_release_buffers(conn);

        timeout.tv_sec  = 3;
        timeout.tv_usec = 0;
        ret = amqp_consume_message(conn, &envelope, &timeout, 0);
        rabbitmaq_unlock();
        if(ret.library_error != AMQP_STATUS_TIMEOUT)
        DNQ_INFO(DNQ_MOD_RABBITMQ, "@@@@@recv msg! type=%d,%d, errno=%d", \
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
                if(error_count == 20)
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
        dnq_msleep(100);
    }
}

int rabbitmq_init(char *serverip, int port, amqp_connection_state_t *pconn)
{
    int i = 0;
    int status;
    amqp_socket_t *socket = NULL;
    amqp_connection_state_t conn;
    amqp_queue_declare_ok_t *r = NULL;
    channel_t  *pchnl = NULL;
    
    conn = amqp_new_connection();
    if(!conn)
        dnq_error(-1, "amqp_new_connection error!");

    socket = amqp_tcp_socket_new(conn);
    if(!socket)
        dnq_error(-1, "amqp_tcp_socket_new error!");
   
    status = amqp_socket_open(socket, serverip, port);
    if(status < 0)
    {
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "amqp_socket_open error! ip=%s,port=%d", serverip, port);
        //dnq_error(status, "amqp_socket_open error! ip=%s,port=%d", serverip, port);
        return status;
    }

    DNQ_INFO(DNQ_MOD_RABBITMQ, "create new connecting! socket=%d, ip=%s, port=%d", \
        status, serverip, port);

    die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 60, AMQP_SASL_METHOD_PLAIN, \
    username, password), "Logging in");

    DNQ_INFO(DNQ_MOD_RABBITMQ, "Login success! user=%s, passwd=%s", username, password);

    pchnl = &channels[0];

    for(i=0; i<6; i++)
    {
        pchnl = &channels[i];
        //create channel
        amqp_channel_open(conn, pchnl->chid);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");
        DNQ_INFO(DNQ_MOD_RABBITMQ, "Opening channel %d success! qname=%s",\
             pchnl->chid, pchnl->qname );

        if(i == 0) /* recv channel */
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
        strcpy(pchnl->qname, "queue_host_");
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
        //dnq_msleep(100);
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

    *pconn = conn;
    run(conn);
    *pconn = NULL;
    
    /* error close the channel */
    for(i=0; i<5; i++)
    {
        pchnl = &channels[i];
        die_on_amqp_error(amqp_channel_close(conn, pchnl->chid, AMQP_REPLY_SUCCESS), "Closing channel");
    }
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
    die_on_error(amqp_destroy_connection(conn), "Ending connection");

    
    return 0;
}

S32 rabbitmq_start_notify()
{
    dnq_msg_t msg = {0};

    msg.Class = MSG_CLASS_RABBITMQ;
    msg.code = 0;

    return send_msg_to_manage(&msg);
}

S32 rabbitmq_task()
{
    U8 server_ip[16] = {0};
    U32 server_port;

    while(1)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "check netlink status...");
        if(!dnq_server_link_isgood(1))
        {
            sleep(3);
            continue;
        }
        
        DNQ_INFO(DNQ_MOD_RABBITMQ, "rabbitmq link is good!");
        dnq_get_server_ip(server_ip);
        server_port = dnq_get_server_port();

        rabbitmq_start_notify();
        
        DNQ_INFO(DNQ_MOD_RABBITMQ, "rabbitmq main_loop start..");
        rabbitmq_init(server_ip, server_port, &g_conn);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "rabbitmq main_loop exit...");
        sleep(10);
    }
}
#define TEST_MAC_ADDR  "70b3d5cf4924"
void *rabbitmq_send_test()
{
    int i = 0;
    channel_t *pchnl = NULL;
    char   *msg;
    char    buffer[1024];
    client_status_t   status = {TEST_MAC_ADDR, 2, {{1, 23, 45}, {2, 45, 67}}};
    client_loss_t     loss = {TEST_MAC_ADDR, 1, {0, 55}};
    client_response_t response = {{TEST_MAC_ADDR}, NULL};
    client_config_t   config = {TEST_MAC_ADDR, 1, {1, 2, 3, 4, 5, 6, 7}};
    client_warn_t     warn = {TEST_MAC_ADDR, 3, {{1,7}, {2,8}, {3,9}}};
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
        /* test code: host to server */
    
        msg = json_create_init(MAC_ADDR);
        send_msg_to_server(g_conn, &channels[5], msg);
        DNQ_DEBUG(DNQ_MOD_RABBITMQ, "send init msg:\n%s", msg);
        dnq_free(msg);
        sleep(time);
        
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


S32 dnq_rabbitmq_init()
{
    S32 ret;

    /* register cjson hooks! */
    cJSON_Hooks hooks = {dnq_malloc, dnq_free};
    cJSON_InitHooks(&hooks);

    ret = dnq_config_init();
    dnq_config_print();
    //sleep(10);
    ret = dnq_task_create("rabbitmq_task", 512*1024, rabbitmq_task, NULL);
    if(!ret) 
        return -1;
    
    return 0;
}

S32 dnq_rabbitmq_deinit()
{
    return 0;
}

int rabbitmq_test()
{
    ngx_pool_t *pool = NULL;
    
    dnq_init();
    cJSON_Hooks hooks = {dnq_malloc, dnq_free};
    cJSON_InitHooks(&hooks);
    
    dnq_debug_setlever(1, 3);
    
    dnq_task_create("rabbitmq_test", 64*2048, rabbitmq_send_test, NULL);
    dnq_task_create("rabbitmq_test", 512*2048, rabbitmq_task, NULL);
    //rabbitmq_task();
    return 0;
}

int json_parse_test()
{
    int type;
    char buffer[2048] = {0};
    char cbuffer[2048] = {0};

    dnq_debug_setlever(1, 5);
    
    //server config file 
    printf("==================server json===================\n");
    dnq_file_read("configs/"SERVER_CFG_FILE_AUTHORRIZATION, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    dnq_file_read("configs/"SERVER_CFG_FILE_POLICY, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    dnq_file_read("configs/"SERVER_CFG_FILE_LIMIT, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    dnq_file_read("configs/"SERVER_CFG_FILE_ERROR, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    dnq_file_read("configs/"SERVER_CFG_FILE_POWER, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    dnq_file_read("configs/"SERVER_CFG_FILE_RESPONSE, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    dnq_file_read("configs/"SERVER_CFG_FILE_CORRECT, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);


    //client config file 
    printf("==================client json===================\n");
    dnq_file_read("configs/"CLIENT_CFG_FILE_STATUS, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);
    
    dnq_file_read("configs/"CLIENT_CFG_FILE_LOSS, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);
    
    dnq_file_read("configs/"CLIENT_CFG_FILE_RESPONSE, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    dnq_file_read("configs/"CLIENT_CFG_FILE_CONFIG, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);

    dnq_file_read("configs/"CLIENT_CFG_FILE_WARN, buffer, sizeof(buffer));
    printf("json data:\n %s\n", buffer);
    type = json_parse(buffer, cbuffer);
    printf("result: type=%d\n", type);
    
    return 0;
}


int json_test()
{
    int ret = 0;
    ngx_pool_t *pool = NULL;
    
    pool = dnq_mempool_init();
    cJSON_Hooks hooks = {dnq_malloc, dnq_free};
    cJSON_InitHooks(&hooks);
    
    json_parse_test();
    
    dnq_mempool_deinit();
    
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

    dnq_mempool_init();
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


