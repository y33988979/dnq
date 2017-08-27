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
    .policy_config = 
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
    .limit_config = 
    {
        .rooms_cnt = DNQ_ROOM_MAX,
        .rooms = 
        {
            /* room_id, min, max */
            {0, 20, 30},
            {1, 20, 30},
            {2, 20, 28},
            {3, 20, 30},
            {4, 20, 30},
            {5, 20, 28},
            {6, 20, 30},
            {7, 20, 30},
            {8, 20, 28},
            {9, 20, 30},
            {10, 20, 28},
            {11, 20, 30},
            {12, 20, 30},
            {13, 20, 28},
            {14, 20, 30},
            {15, 20, 28},
        }
    },
    .error_config = 
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
    .correct_config = 
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
        DNQ_ERROR(DNQ_MOD_CONFIG, "open file %s error! err=%s!", filepath, strerror(errno));
        return -1;
    }

    ret = fread(buffer, 1, len, fp);
    if(len < 0)
    {
        fclose(fp);
        DNQ_ERROR(DNQ_MOD_CONFIG, "read error! err=%s!", filepath, strerror(errno));
        return -1;
    }
    fclose(fp);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "read len=%d\n", ret);

    return ret; 
}

S32 dnq_file_write(U8 *filepath, U8 *buffer, U32 len)
{
    S32    ret = 0;
    FILE  *fp = NULL;
    
    fp = fopen(filepath, "w+");
    if(fp == NULL)
    {
        DNQ_ERROR(DNQ_MOD_CONFIG, "open file %s error! err=%s!", filepath, strerror(errno));
        return -1;
    }
    
    ret = fwrite(buffer, 1, len, fp);
    if(len < 0)
    {
        fclose(fp);
        DNQ_ERROR(DNQ_MOD_CONFIG, "write error! err=%s!", filepath, strerror(errno));
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

    dnq_config_adjust();
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

    DNQ_DEBUG(DNQ_MOD_CONFIG, "=================================");
    
    DNQ_DEBUG(DNQ_MOD_CONFIG, "====authorization config===");
    DNQ_DEBUG(DNQ_MOD_CONFIG, "type=\t%s", config->authorization.type);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "time=\t%s", config->authorization.time);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "authorization=\t%s", config->authorization.authorization);

    DNQ_DEBUG(DNQ_MOD_CONFIG, "====policy_config config===");
    DNQ_DEBUG(DNQ_MOD_CONFIG, "type=\t%s", config->policy_config.type);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "time=\t%s", config->policy_config.time);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "ctrl_id=\t%s", config->policy_config.ctrl_id);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "mode=\t%d", config->policy_config.mode);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "rooms_cnt=\t%d", config->policy_config.rooms_cnt);
    
    for(i=0; i<config->policy_config.rooms_cnt; i++)
    {
        DNQ_DEBUG(DNQ_MOD_CONFIG, "id=\t%d", config->policy_config.rooms[i].room_id);
        DNQ_DEBUG(DNQ_MOD_CONFIG, "dpid=\t%d", config->policy_config.rooms[i].dpid);
        DNQ_DEBUG(DNQ_MOD_CONFIG, "setting_cnt=\t%d", config->policy_config.rooms[i].time_setting_cnt);

        for(j=0; j<config->policy_config.rooms[i].time_setting_cnt; j++)
        {
            DNQ_DEBUG(DNQ_MOD_CONFIG, "start=\t%d", config->policy_config.rooms[i].time_setting[j].start);
            DNQ_DEBUG(DNQ_MOD_CONFIG, "starttime=\t%s", config->policy_config.rooms[i].time_setting[j].starttime);
            DNQ_DEBUG(DNQ_MOD_CONFIG, "end=\t%d", config->policy_config.rooms[i].time_setting[j].end);
            DNQ_DEBUG(DNQ_MOD_CONFIG, "endtime=\t%s", config->policy_config.rooms[i].time_setting[j].endtime);
            DNQ_DEBUG(DNQ_MOD_CONFIG, "degrees=\t%d", config->policy_config.rooms[i].time_setting[j].degrees);
        }
    }

    DNQ_DEBUG(DNQ_MOD_CONFIG, "====limit_config config===");
    DNQ_DEBUG(DNQ_MOD_CONFIG, "type=\t%s", config->limit_config.type);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "time=\t%s", config->limit_config.time);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "ctrl_id=\t%s", config->limit_config.ctrl_id);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "mode=\t%d", config->limit_config.mode);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "rooms_cnt=\t%d", config->limit_config.rooms_cnt);
    
    for(i=0; i<config->limit_config.rooms_cnt; i++)
    {
        DNQ_DEBUG(DNQ_MOD_CONFIG, "id=\t%d", config->limit_config.rooms[i].room_id);
        DNQ_DEBUG(DNQ_MOD_CONFIG, "max=\t%d", config->limit_config.rooms[i].max);
        DNQ_DEBUG(DNQ_MOD_CONFIG, "min=\t%d", config->limit_config.rooms[i].min);
    }
#if 0
    //error_config
    DNQ_DEBUG(DNQ_MOD_CONFIG, "====error_config config===\n");
    DNQ_DEBUG(DNQ_MOD_CONFIG, "type=\t%s\n", config->error_config.type);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "time=\t%s\n", config->error_config.time);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "ctrl_id=\t%s\n", config->error_config.ctrl_id);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "mode=\t%d\n", config->error_config.mode);
    
    for(i=0; i<config->error_config.rooms_cnt; i++)
    {
        DNQ_DEBUG(DNQ_MOD_CONFIG, "id=\t%d\n", config->error_config.rooms[i].room_id);
        DNQ_DEBUG(DNQ_MOD_CONFIG, "error=\t%d\n", config->error_config.rooms[i].error);
    }

    //correct_config
    DNQ_DEBUG(DNQ_MOD_CONFIG, "====correct_config config===\n");
    DNQ_DEBUG(DNQ_MOD_CONFIG, "type=\t%s\n", config->correct_config.type);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "time=\t%s\n", config->correct_config.time);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "ctrl_id=\t%s\n", config->correct_config.ctrl_id);
    DNQ_DEBUG(DNQ_MOD_CONFIG, "mode=\t%d\n", config->correct_config.mode);
    
    for(i=0; i<config->error_config.rooms_cnt; i++)
    {
        DNQ_DEBUG(DNQ_MOD_CONFIG, "id=\t%d\n", config->correct_config.rooms[i].room_id);
        DNQ_DEBUG(DNQ_MOD_CONFIG, "correct=\t%d\n", config->correct_config.rooms[i].correct);
    }
#endif
    printf("=================================\n");
}

S32 dnq_config_adjust()
{
    U32  i;    
    S32  setting_temp;
    U8   gb2312_out[32] = {0};
    room_item_t *rooms = dnq_get_rooms();
    init_info_t *init_config;
    policy_config_t *policy_config;
    error_config_t  *error_config;
    limit_config_t  *limit_config;
    
    init_config = dnq_get_init_config(NULL);
    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        rooms[i].id = init_config->rooms[i].room_order;
        u2g(init_config->rooms[i].room_name, 16, gb2312_out,sizeof(gb2312_out));   
        strncpy(rooms[i].name, gb2312_out, 16);
    }

    policy_config = dnq_get_temp_policy_config(NULL);
    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        setting_temp = dnq_get_room_current_setting_temp(i);
        if(setting_temp == DEGREES_NULL)
        {
            setting_temp = 0;
        }
        rooms[i].set_temp = setting_temp;
        DNQ_INFO(DNQ_MOD_CONFIG, "rooms[%d].set_temp=%d", i, setting_temp);
    }

    return 0;
}

S32 dnq_config_load()
{
    S32  ret = 0;
    
    ret = dnq_file_read(DNQ_DATA_FILE, (U8*)&g_dnq_config, sizeof(dnq_config_t));
    if(ret < 0)
        return -1;

    g_dnq_config.init.inited = 0;
    
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
    type = json_parse(buffer, config->policy_config);

    sprintf(filepath, "%s/%s", DNQ_CONFIG_PATH, JSON_FILE_LIMIT);
    ret = dnq_file_read(filepath, buffer, sizeof(buffer));
    type = json_parse(buffer, config->limit_config);

    sprintf(filepath, "%s/%s", DNQ_CONFIG_PATH, JSON_FILE_ERROR);
    ret = dnq_file_read(filepath, buffer, sizeof(buffer));
    type = json_parse(buffer, config->error_config);

    sprintf(filepath, "%s/%s", DNQ_CONFIG_PATH, JSON_FILE_POWER);
    ret = dnq_file_read(filepath, buffer, sizeof(buffer));
    type = json_parse(buffer, config->power_config);

    sprintf(filepath, "%s/%s", DNQ_CONFIG_PATH, JSON_FILE_RESPONSE);
    ret = dnq_file_read(filepath, buffer, sizeof(buffer));
    type = json_parse(buffer, config->response);

    sprintf(filepath, "%s/%s", DNQ_CONFIG_PATH, JSON_FILE_CORRECT);
    ret = dnq_file_read(filepath, buffer, sizeof(buffer));
    type = json_parse(buffer, config->correct_config);

    sprintf(filepath, "%s/%s", DNQ_CONFIG_PATH, JSON_FILE_INIT);
    ret = dnq_file_read(filepath, buffer, sizeof(buffer));
    type = json_parse(buffer, config->init);

    config->inited = 1;
    /* un finished...... */

    DNQ_INFO(DNQ_MOD_CONFIG, "load all config!!!");
    
    return 0;
}
#endif

S32 dnq_data_file_set_default_value()
{
    S8  utf8_out[SIZE_32] = {0};
    dnq_config_t *all_config = &g_dnq_config;

    g2u("松花江小学", SIZE_32, utf8_out, sizeof(utf8_out));
    strncpy(all_config->init.project_name, utf8_out, SIZE_32);
    
    g2u("主楼", SIZE_32, utf8_out, sizeof(utf8_out));
    strncpy(all_config->init.building_name, utf8_out, SIZE_32);
    
    g2u("二楼东", SIZE_32, utf8_out, sizeof(utf8_out));
    strncpy(all_config->init.buildPosition, utf8_out, SIZE_32);
    
    g2u("三号箱[def]", SIZE_32, utf8_out, sizeof(utf8_out));
    strncpy(all_config->init.hostName, utf8_out, SIZE_32);

    return 0;
}

S32 dnq_data_file_create()
{
    S32 ret = 0;
    dnq_config_t *all_config = &g_dnq_config;
    
    dnq_data_file_set_default_value();
    ret = dnq_file_write(DNQ_DATA_FILE, (U8*)&g_dnq_config, sizeof(dnq_config_t));
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_CONFIG, "create defult data failed!!!");
        remove(DNQ_DATA_FILE);
    }
    else
    {
        DNQ_INFO(DNQ_MOD_CONFIG, "create defult data success!!!");
    }
    
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

S32 dnq_get_room_current_setting_temp(U32 room_id)
{
    S32 ret;
    U32 i;
    U32 start_time, end_time;
    room_temp_policy_t *room_policy;
    policy_config_t *policy_config;
    timesetting_t  *room_time_setting;
    U32 current_time = dnq_get_current_second();
    
    policy_config = dnq_get_temp_policy_config(NULL);
    room_policy = &policy_config->rooms[room_id];
    room_time_setting = room_policy->time_setting;
    
    for(i=0; i<room_policy->time_setting_cnt; i++)
    {
        DNQ_DEBUG(DNQ_MOD_CONFIG, "current_time=%d, start=%d, end=%d\n", \
            current_time, room_time_setting[i].start, room_time_setting[i].end);
        if(current_time >= room_time_setting[i].start
        && current_time <= room_time_setting[i].end)
            return room_time_setting[i].degrees*100;
    }
    
    return DEGREES_NULL;
}

timesetting_t* dnq_get_room_setting_by_time(U32 room_id, U32 current_time)
{
    S32 ret;
    U32 i;
    U32 start_time, end_time;
    room_temp_policy_t *room_policy;
    policy_config_t *policy_config;
    timesetting_t  *room_time_setting;

    policy_config = dnq_get_temp_policy_config(NULL);
    room_policy = &policy_config->rooms[room_id];
    room_time_setting = room_policy->time_setting;
    
    for(i=0; i<room_policy->time_setting_cnt; i++)
    {
        DNQ_DEBUG(DNQ_MOD_CONFIG, "current_time=%d, start=%d, end=%d", \
            current_time, room_time_setting[i].start, room_time_setting[i].end);
        if(current_time >= room_time_setting[i].start
        && current_time <= room_time_setting[i].end)
            return &room_time_setting[i];
    }
    
    return NULL;
}

authorization_t*
    dnq_get_authorization_config(authorization_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.authorization, sizeof(authorization_t));
    return &g_dnq_config.authorization;
}

policy_config_t* 
    dnq_get_temp_policy_config(policy_config_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.policy_config, sizeof(policy_config_t));
    return &g_dnq_config.policy_config;
}

limit_config_t* 
    dnq_get_temp_limit_config(limit_config_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.limit_config, sizeof(limit_config_t));
    return &g_dnq_config.limit_config;
}

error_config_t*
    dnq_get_temp_error_config(error_config_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.error_config, sizeof(error_config_t));
    return &g_dnq_config.error_config;
}

power_config_t*
    dnq_get_power_config_config(power_config_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.power_config, sizeof(power_config_t));
    return &g_dnq_config.power_config;
}

response_t*
    dnq_get_response_config(response_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.response, sizeof(response_t));
    return &g_dnq_config.response;
}

correct_config_t*
    dnq_get_temp_correct_config(correct_config_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.correct_config, sizeof(correct_config_t));
    return &g_dnq_config.correct_config;
}

init_info_t*
    dnq_get_init_config(init_info_t *config)
{
    if(config)
        memcpy(config, &g_dnq_config.init, sizeof(init_info_t));
    return &g_dnq_config.init;
}

S32 dnq_config_update_authorization(void *cjson_struct)
{
    authorization_t *authorization = NULL;
    authorization = (authorization_t *)cjson_struct;
    
    return 0;
}
/* 同步云端下发的温度策略，到本地，返回需要更新的room_id */
S32 dnq_config_update_temp_policy(policy_config_t *policy_config)
{
    S32 i, room_id;
    S32 set_temp = 0;
    policy_config_t *curr_policy;

    room_item_t *rooms = dnq_get_rooms();
    curr_policy = dnq_get_temp_policy_config(NULL);

    curr_policy->mode = policy_config->mode;
    strcpy(curr_policy->time, policy_config->time);
    
    /* single mode */
    if(policy_config->mode == 1)
    {
        room_id = policy_config->rooms[0].room_id - 1;
        memcpy(&curr_policy->rooms[room_id],\
            &policy_config->rooms[0], sizeof(room_temp_policy_t));
        
        set_temp = dnq_get_room_current_setting_temp(room_id);
        if(set_temp != DEGREES_NULL)
            rooms[room_id].set_temp = set_temp;
        
    }
    else if(policy_config->mode == 0) /* whole all config */
    {
        for(i=0; i<DNQ_ROOM_CNT; i++)
        {
            memcpy(&curr_policy->rooms[i], \
                &policy_config->rooms[0], sizeof(room_temp_policy_t));
            set_temp = dnq_get_room_current_setting_temp(i);
            if(set_temp != DEGREES_NULL)
                rooms[i].set_temp = set_temp;
        }
        room_id = DNQ_ROOM_MAX;
    }
    else
        DNQ_ERROR(DNQ_MOD_CONFIG, "error mode=%d! value must be 0 or 1!", policy_config->mode);

    DNQ_INFO(DNQ_MOD_CONFIG, "update temp config!");
    
    return room_id;
}

S32 dnq_config_update_temp_limit(limit_config_t *limit_config)
{
    S32 room_id = -1;
    S32 degrees, set_temp;
    S32 low, high;
    S32 i, j;
    policy_config_t *curr_policy;
    limit_config_t *curr_limit_config;
    room_item_t *rooms = dnq_get_rooms();

    curr_policy = dnq_get_temp_policy_config(NULL);
    curr_limit_config = dnq_get_temp_limit_config(NULL);
    curr_limit_config->mode = limit_config->mode;

    /* single mode */
    if(limit_config->mode == 1)
    {
        room_id = limit_config->rooms[0].room_id - 1;

        /* check low temperature limit, must be 0'C~6'C */
        degrees = limit_config->rooms[0].min*100;
        high = DNQ_TEMP_MIN + DNQ_TEMP_THRESHOLD_OFFSET;
        low = DNQ_TEMP_MIN - DNQ_TEMP_THRESHOLD_OFFSET;
        if(degrees > high || degrees < low)
        {
            DNQ_ERROR(DNQ_MOD_CONFIG, "room[%d]'s low_limit[%d] must be %d'C~%d'C!",\
                room_id, degrees, low, high);
            return -1;
        }
        
        /* check high temperature limit, must be 27'C~33'C */
        degrees = limit_config->rooms[0].max*100;
        high = DNQ_TEMP_MAX + DNQ_TEMP_THRESHOLD_OFFSET;
        low = DNQ_TEMP_MAX - DNQ_TEMP_THRESHOLD_OFFSET;
        if(degrees > high || degrees < low)
        {
            DNQ_ERROR(DNQ_MOD_CONFIG, "room[%d]'s high_limit[%d] must be %d'C~%d'C!",\
                room_id, degrees, low, high);
             return -1;
        }
        
        memcpy(&curr_limit_config->rooms[room_id],\
            &limit_config->rooms[0], sizeof(room_temp_limit_t));
        
        /* 如果策略中设定的温度，如果在limit之外，需要更改设定温度 */
        for(i=0; i<curr_policy->rooms[room_id].time_setting_cnt; i++)
        {
            degrees = curr_policy->rooms[room_id].time_setting[i].degrees;
            if(degrees > limit_config->rooms[0].max)
                DNQ_WARN(DNQ_MOD_CONFIG, "room[%d]'s hight_limit[%d] can't less than the set_temp[%d]!",\
                room_id, limit_config->rooms[0].max, degrees);
            if(degrees < limit_config->rooms[0].min)
                DNQ_WARN(DNQ_MOD_CONFIG, "room[%d]'s low_limit[%d] can't greater than the set_temp[%d]!",\
                room_id, limit_config->rooms[0].min, degrees);
                //curr_policy->rooms[room_id].time_setting[i].degrees = limit_config->rooms[0].min;
        }
        set_temp = dnq_get_room_current_setting_temp(room_id);
        if(set_temp != DEGREES_NULL)
            rooms[room_id].set_temp = set_temp;
    }
    else if(limit_config->mode == 0) /* whole all config */
    {
        for(i=0; i<DNQ_ROOM_CNT; i++)
        {
            /* check low temperature limit, must be 0'C~6'C */
            degrees = limit_config->rooms[i].min*100;
            high = DNQ_TEMP_MIN + DNQ_TEMP_THRESHOLD_OFFSET;
            low = DNQ_TEMP_MIN - DNQ_TEMP_THRESHOLD_OFFSET;
            if(degrees > high || degrees < low)
            {
                DNQ_ERROR(DNQ_MOD_CONFIG, "room[%d]'s low_limit[%d] must be %d'C~%d'C!",\
                    i, degrees, low, high);
                return -1;
            }
            
            /* check high temperature limit, must be 27'C~33'C */
            degrees = limit_config->rooms[i].max*100;
            high = DNQ_TEMP_MAX + DNQ_TEMP_THRESHOLD_OFFSET;
            low = DNQ_TEMP_MAX - DNQ_TEMP_THRESHOLD_OFFSET;
            if(degrees > high || degrees < low)
            {
                DNQ_ERROR(DNQ_MOD_CONFIG, "room[%d]'s high_limit[%d] must be %d'C~%d'C!",\
                    i, degrees, low, high);
                 return -1;
            }
        
            memcpy(&curr_limit_config->rooms[i], \
                &limit_config->rooms[0], sizeof(room_temp_limit_t));
            for(j=0; j<curr_policy->rooms[i].time_setting_cnt; j++)
            {
                degrees = curr_policy->rooms[i].time_setting[j].degrees;
                if(degrees > limit_config->rooms[i].max)
                    DNQ_WARN(DNQ_MOD_CONFIG, "room[%d]'s hight_limit[%d] can't less than the set_temp[%d]!",\
                    i, limit_config->rooms[i].max, degrees);
                if(degrees < limit_config->rooms[i].min)
                    DNQ_WARN(DNQ_MOD_CONFIG, "room[%d]'s low_limit[%d] can't greater than the set_temp[%d]!",\
                    i, limit_config->rooms[i].min, degrees);
            }

            set_temp = dnq_get_room_current_setting_temp(i);
            if(set_temp != DEGREES_NULL)
                rooms[i].set_temp = set_temp;
        }
        room_id = DNQ_ROOM_MAX;
    }
    else
    {
        DNQ_ERROR(DNQ_MOD_CONFIG, "error mode=%d! value must be 0 or 1!", limit_config->mode);
        return -1;
    }

    DNQ_INFO(DNQ_MOD_CONFIG, "update limit config!");
    return room_id;
}

S32 dnq_config_update_temp_error(void *cjson_struct)
{
    S32 i;
    S32 room_id;
    error_config_t *error_config = (error_config_t *)cjson_struct;
    error_config_t *curr_error = dnq_get_temp_error_config(NULL);

    for(i=0; i<error_config->rooms_cnt; i++)
    {
        room_id = error_config->rooms[i].room_id - 1;
        if(room_id >= 0 && room_id < DNQ_ROOM_MAX)
        {
            curr_error->rooms[room_id].error = error_config->rooms[i].error;
        }
    }
    
    return 0;
}

S32 dnq_config_update_power_config(void *cjson_struct)
{
    power_config_t *power_config;
    power_config = (power_config_t *)cjson_struct;
    
    return 0;
}

S32 dnq_config_update_response(void *cjson_struct)
{
    response_t *response;
    response = (response_t *)cjson_struct;
    
    return 0;
}

S32 dnq_config_update_temp_correct(correct_config_t *correct_config)
{
    S32 i, room_id;
    correct_config_t *curr_correct;
    room_item_t *rooms = dnq_get_rooms();
    
    curr_correct = dnq_get_temp_correct_config(NULL);

    /* single mode */
    if(correct_config->mode == 1)
    {
        room_id = correct_config->rooms[0].room_id - 1;
        memcpy(&curr_correct->rooms[room_id],\
            &correct_config->rooms[0], sizeof(room_temp_error_t));
    }
    else if(correct_config->mode == 0) /* whole all config */
    {
        for(i=0; i<DNQ_ROOM_CNT; i++)
        {
            memcpy(&curr_correct->rooms[i], \
                &correct_config->rooms[0], sizeof(room_temp_error_t));
        }
        room_id = DNQ_ROOM_MAX;
    }
    else
        DNQ_ERROR(DNQ_MOD_CONFIG, "error mode=%d! value must be 0 or 1!", correct_config->mode);

    return room_id;
}


S32 dnq_config_update_init_info(void *cjson_struct)
{
    init_info_t *init_info;
    init_info = (init_info_t *)cjson_struct;
    
    return 0;
}

S32 dnq_config_check_and_sync(json_type_e json_type, U8 *json_data, U32 len, void *cjson_struct)
{
    
    S32 ret = -1;
    U32 i, size = 0;
    S32 room_id = -1;
    dnq_msg_t msg = {0};
    switch(json_type)
    {
        case JSON_TYPE_AUTHORRIZATION:
            ret = dnq_config_update_authorization(cjson_struct);
            if(ret < 0)
                return -1;
            memcpy(&g_dnq_config.authorization, cjson_struct, sizeof(authorization_t));
            dnq_json_save_file(JSON_FILE_AUTHORRIZATION, json_data, len);
        break;
        case JSON_TYPE_TEMP_POLICY:
            room_id = dnq_config_update_temp_policy((policy_config_t *)cjson_struct);
            if(room_id < 0)
                return -1;
            dnq_config_sync_to_lcd(json_type, cjson_struct, room_id);
            dnq_json_save_file(JSON_FILE_POLICY, json_data, len);
        break;
        case JSON_TYPE_TEMP_LIMIT:
            room_id = dnq_config_update_temp_limit((limit_config_t *)cjson_struct);
            if(room_id < 0)
                return -1;
            dnq_config_sync_to_lcd(json_type, cjson_struct, room_id);
            dnq_json_save_file(JSON_FILE_LIMIT, json_data, len);
        break;
        case JSON_TYPE_TEMP_ERROR:
            ret = dnq_config_update_temp_error(cjson_struct);
            if(ret < 0)
                return -1;
            dnq_json_save_file(JSON_FILE_ERROR, json_data, len);
        break;
        case JSON_TYPE_POWER_CONFIG:
            ret = dnq_config_update_power_config(cjson_struct);
            if(ret < 0)
                return -1;
            memcpy(&g_dnq_config.power_config, cjson_struct, sizeof(power_config_t));
            dnq_json_save_file(JSON_FILE_POWER, json_data, len);
        break;
        case JSON_TYPE_RESPONSE:
            ret = dnq_config_update_response(cjson_struct);
            if(ret < 0)
                return -1;
            memcpy(&g_dnq_config.response, cjson_struct, sizeof(response_t));
            dnq_json_save_file(JSON_FILE_RESPONSE, json_data, len);
            return 0;
        break;
        case JSON_TYPE_CORRECT:            
            room_id = dnq_config_update_temp_correct((correct_config_t*)cjson_struct);
            if(room_id < 0)
                return -1;
            dnq_config_sync_to_lcd(json_type, cjson_struct, room_id);
            dnq_json_save_file(JSON_FILE_CORRECT, json_data, len);
        break;
        case JSON_TYPE_INIT:
            ret = dnq_config_update_init_info(cjson_struct);
            if(ret < 0)
                return -1;
            memcpy(&g_dnq_config.init, cjson_struct, sizeof(init_info_t));
            dnq_json_save_file(JSON_FILE_INIT, json_data, len);
        break;
        default:
            DNQ_ERROR(DNQ_MOD_CONFIG, "unknown json type: %d", json_type);
            return -1;
            break;
    
    }

    /* save all config to file --> dnq.dat */
    dnq_data_file_save();    
    
    return ret;
}

