/* dnq config Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  this is a common interface API, for app.
 * Note : 
 */


#include "dnq_common.h"
#include "dnq_config.h"
#include "ngx_palloc.h"
#include "dnq_checksum.h"
#include "dnq_lcd.h"
#include "dnq_log.h"
#include "cJSON.h"

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

S32 dnq_file_read(U8 *filepath, U8 *buffer, U32 len)
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
    fclose(fp);
    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "read len=%d\n", ret);

    return ret; 
}

S32 dnq_file_write(U8 *filepath, U8 *buffer, U32 len)
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

    dnq_config_print();
    
    return 0;
}

S32 dnq_config_deinit()
{
    memset(&g_dnq_config, 0, sizeof(dnq_config_t));
    return 0;
}

void dnq_config_print()
{
    U32 i = 0;
    U32 j = 0;

    dnq_config_t *config = &g_dnq_config;

    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "=================================");
    
    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "====authorization config===");
    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "type=\t%s", config->authorization.type);
    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "time=\t%s", config->authorization.time);
    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "authorization=\t%s", config->authorization.authorization);

    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "====temp_policy config===");
    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "type=\t%s", config->temp_policy.type);
    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "time=\t%s", config->temp_policy.time);
    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "ctrl_id=\t%s", config->temp_policy.ctrl_id);
    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "mode=\t%d", config->temp_policy.mode);
    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "rooms_cnt=\t%d", config->temp_policy.rooms_cnt);
    
    for(i=0; i<config->temp_policy.rooms_cnt; i++)
    {
        DNQ_DEBUG(DNQ_MOD_RABBITMQ, "id=\t%d", config->temp_policy.rooms[i].room_id);
        DNQ_DEBUG(DNQ_MOD_RABBITMQ, "dpid=\t%d", config->temp_policy.rooms[i].dpid);
        DNQ_DEBUG(DNQ_MOD_RABBITMQ, "setting_cnt=\t%d", config->temp_policy.rooms[i].time_setting_cnt);

        for(j=0; j<config->temp_policy.rooms[i].time_setting_cnt; j++)
        {
            DNQ_DEBUG(DNQ_MOD_RABBITMQ, "start=\t%d", config->temp_policy.rooms[i].time_setting[j].start);
            DNQ_DEBUG(DNQ_MOD_RABBITMQ, "starttime=\t%s", config->temp_policy.rooms[i].time_setting[j].starttime);
            DNQ_DEBUG(DNQ_MOD_RABBITMQ, "end=\t%d", config->temp_policy.rooms[i].time_setting[j].end);
            DNQ_DEBUG(DNQ_MOD_RABBITMQ, "endtime=\t%s", config->temp_policy.rooms[i].time_setting[j].endtime);
            DNQ_DEBUG(DNQ_MOD_RABBITMQ, "degrees=\t%d", config->temp_policy.rooms[i].time_setting[j].degrees);
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

    ret = dnq_file_read(DNQ_DATA_FILE, (U8*)&g_dnq_config, sizeof(dnq_config_t));
    if(ret < 0)
        return -1;

    g_dnq_config.init.inited = 0;
    init_config = &g_dnq_config.init;
    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        rooms[i].id = init_config->rooms[i].room_order;
        u2g(init_config->rooms[i].room_name, 16, gb2312_out,sizeof(gb2312_out));   
        strncpy(rooms[i].name, gb2312_out, 16);
        rooms[i].correct = init_config->rooms[i].correct;
    }

    policy_config = &g_dnq_config.temp_policy;
    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        current_second = dnq_get_current_second();
        setting_temp = dnq_get_room_setting_temp_by_time(i, current_second);
        if(setting_temp == DEGREES_NULL)
        {
            setting_temp = 0;
        }
        rooms[i].set_temp = setting_temp*100;
        DNQ_INFO(DNQ_MOD_RABBITMQ, "rooms[%d].set_temp=%d\n", i, setting_temp);
    }
  
    return ret;
}

#if 0
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
#endif

S32 dnq_data_file_create()
{
    S32 ret = 0;
    dnq_config_t *all_config = &g_dnq_config;
    ret = dnq_file_write(DNQ_DATA_FILE, (U8*)&g_dnq_config, sizeof(dnq_config_t));
    if(ret < 0)
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
    return dnq_file_write(DNQ_DATA_FILE, (U8*)&g_dnq_config, sizeof(dnq_config_t));
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
    U32 i, room_id;
    U32 current_second = 0;
    S32 setting_temp = 0;
    server_temp_policy_t *temp_policy, *curr_policy;
    temp_policy = (server_temp_policy_t *)cjson_struct;
    room_item_t *rooms = dnq_get_rooms();

    current_second = dnq_get_current_second();
    curr_policy = dnq_get_temp_policy_config(NULL);
    
    /* single mode */
    if(temp_policy->mode == 1)
    {
        room_id = temp_policy->rooms[0].room_id - 1;
        memcpy(&curr_policy->rooms[room_id],\
            &temp_policy->rooms[0], sizeof(room_temp_policy_t));
        
        setting_temp = dnq_get_room_setting_temp_by_time(room_id, current_second);
        if(setting_temp != DEGREES_NULL)
            rooms[room_id].set_temp = setting_temp;
        
    }
    else if(temp_policy->mode == 0) /* whole all config */
    {
        for(i=0; i<curr_policy->rooms_cnt; i++)
        {
            memcpy(&curr_policy->rooms[i], \
                &temp_policy->rooms[0], sizeof(room_temp_policy_t));
            setting_temp = dnq_get_room_setting_temp_by_time(i, current_second);
            if(setting_temp != DEGREES_NULL)
                rooms[i].set_temp = setting_temp;
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

S32 dnq_config_check_and_sync(json_type_e json_type, U8 *json_data, U32 len, void *cjson_struct)
{
    
    S32 ret = 0;
    U32 i, size = 0;
    U32 room_id;
    dnq_msg_t msg = {0};
    switch(json_type)
    {
        case JSON_TYPE_AUTHORRIZATION:
            ret = dnq_config_update_authorization(cjson_struct);
            if(!ret)
            {
                memcpy(&g_dnq_config.authorization, cjson_struct, sizeof(server_authorization_t));
                dnq_json_save_file(JSON_FILE_AUTHORRIZATION, json_data, len);
            }
        break;
        case JSON_TYPE_TEMP_POLICY:
            ret = dnq_config_update_temp_policy(cjson_struct);
            if(!ret)
            {
                server_temp_policy_t *temp_policy = (server_temp_policy_t *)cjson_struct;

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
            ret = dnq_config_update_temp_limit(cjson_struct);
            if(!ret)
            {
                memcpy(&g_dnq_config.temp_limit, cjson_struct, sizeof(server_temp_limit_t));
                dnq_json_save_file(JSON_FILE_LIMIT, json_data, len);
            }
        break;
        case JSON_TYPE_TEMP_ERROR:
            ret = dnq_config_update_temp_error(cjson_struct);
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
            ret = dnq_config_update_power_config(cjson_struct);
            if(!ret)
            {
                memcpy(&g_dnq_config.power_config, cjson_struct, sizeof(server_power_config_t));
                dnq_json_save_file(JSON_FILE_POWER, json_data, len);
            }
        break;
        case JSON_TYPE_RESPONSE:
            ret = dnq_config_update_response(cjson_struct);
            if(!ret)
            {
                memcpy(&g_dnq_config.response, cjson_struct, sizeof(server_response_t));
                dnq_json_save_file(JSON_FILE_RESPONSE, json_data, len);
            }
        break;
        case JSON_TYPE_CORRECT:
            ret = dnq_config_update_temp_correct(cjson_struct);
            if(!ret)
            {
                server_temp_correct_t *temp_correct = (server_temp_correct_t *)cjson_struct;

                for(i=0; i<temp_correct->rooms_cnt; i++)
                {
                    room_id = temp_correct->rooms[i].room_id - 1;
                    if(room_id >= 0 && room_id <= DNQ_ROOM_MAX)
                    {
                        g_dnq_config.temp_correct.rooms[room_id].correct = temp_correct->rooms[i].correct;
                    }
                }
                //memcpy(&g_dnq_config.temp_correct, cjson_struct, sizeof(server_temp_correct_t));
                dnq_json_save_file(JSON_FILE_CORRECT, json_data, len);
            }
        break;
        case JSON_TYPE_INIT:
            ret = dnq_config_update_init_info(cjson_struct);
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
    return ret;
}

