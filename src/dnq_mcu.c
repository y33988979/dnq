/* 
 * cpu and mcu communicate Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  this is a communicate Program between cpu and mcu.
 *  mcu1:  ctrl heater and rtc  protocol: RS232
 *
 * Note : 
 */


#include "dnq_common.h"
#include "ngx_palloc.h"
#include "dnq_gpio.h"
#include "dnq_uart.h"
#include "dnq_os.h"
#include "dnq_log.h"
#include "dnq_lcd.h"
#include "dnq_mcu.h"

static datetime_t g_datetime;
static U32  g_curr_second = 0;
static dnq_task_t *g_mcu_task;
static pthread_mutex_t mcu_mutex = PTHREAD_MUTEX_INITIALIZER;

#define mcu_lock()   pthread_mutex_lock(&mcu_mutex)
#define mcu_unlock() pthread_mutex_unlock(&mcu_mutex)

static U8 g_rs232_databuf[5][64] =
{
    // 0: ctrl all rooms
    {
        0xFF,0xFE,0xFE,0xFF,  /* frame header, 帧头 */
        0xA0,                 /* Flag , 标志位*/
        0x1E,                 /* data lenght, 数据长度 */
        0xB0,                 /* mode, 0xB0: power mode, 0xB1: switch mode */
        0xFF,                 /* ctrl all rooms */
        /*
        * 16路房间的配置
        * 调压模式: 0x01表示100%功率，0x02表示75%功率,0x03表示关闭
        * 继电器模式: 忽略0x02，0x01表示打开，0x03表示关闭
        */
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

        0x00,0x00,            /* CRC16 */
        0xFE,0xFF,0xFF,0xFE   /* frame footer */
    },

    // 1: ctrl single room
    {
        0xFF,0xFE,0xFE,0xFF,  /* frame header, 帧头 */
        0xA0,                 /* Flag , 标志位*/
        0x0F,                 /* data lenght, 数据长度 */
        0xB0,                 /* mode, 0xB0: power mode, 0xB1: switch mode */
        
        0x00,                 /* ctrl single room, 0x00~0x18 */               
        0x00,                 /* power mode:  0x01:100%, 0x02:75%, 0x03:close */ 
                              /* switch mode: 0x01:open, 0x03:close */ 

        0x00,0x00,            /* CRC16 */
        0xFE,0xFF,0xFF,0xFE   /* frame footer */
    },

    // 2: set date&time
    {
        0xFF,0xFE,0xFE,0xFF,  /* frame header, 帧头 */
        0xA1,                 /* Flag , 标志位*/
        0x13,                 /* data lenght, 数据长度 */
        0x10,                 /* set datetime flag */
        
        0x11,0x00,0x00,       /* year month day */               
        0x00,0x00,0x00,       /* hour mintue second */ 
                             
        0x00,0x00,            /* CRC16 */
        0xFE,0xFF,0xFF,0xFE   /* frame footer */
    },

    // 3: get date&time
    {
        0xFF,0xFE,0xFE,0xFF,  /* frame header, 帧头 */
        0xA0,                 /* Flag , 标志位*/
        0x0D,                 /* data lenght, 数据长度 */
        0xB2,                 /* get datetime flag */
                             
        0x00,0x00,            /* CRC16 */
        0xFE,0xFF,0xFF,0xFE   /* frame footer */
    },

    // 4: heartbeat
    {
        0xFF,0xFE,0xFE,0xFF,  /* frame header, 帧头 */
        0xA0,                 /* 控制命令 */
        0x0D,                 /* data lenght, 数据长度 */
        0xB3,                 /* CPU is normal */
                             
        0x00,0x00,            /* CRC16 */
        0xFE,0xFF,0xFF,0xFE   /* frame footer */
    },
    
};

char u[] = "FF FE FE FF F0 0D 00 26 BB FE FF FF FE";

S32 cmdbuf_update_time(cmd_id_e cmd_id, datetime_t *datetime)
{
    U32  pos = 0;
    char *cmdbuf = g_rs232_databuf[cmd_id];

    cmdbuf[7] = datetime->year;
    cmdbuf[8] = datetime->month;
    cmdbuf[9] = datetime->day;
    cmdbuf[10] = datetime->hour;
    cmdbuf[11] = datetime->minute;
    cmdbuf[12] = datetime->second;
    return 0;
}

S32 cmdbuf_update_crc(cmd_id_e cmd_id)
{
    U32  pos = 0;
    U16  crc_value;
    char *cmdbuf = g_rs232_databuf[cmd_id];

    if(cmd_id == CMD_ID_CTRL_ALL)
        pos = 24;
    else if(cmd_id == CMD_ID_CTRL_SINGLE)
        pos = 9;
    else if(cmd_id == CMD_ID_SET_TIME)
        pos = 13;
    else if(cmd_id == CMD_ID_GET_TIME)
        pos = 7;
    else if(cmd_id == CMD_ID_HEARTBEAT)
        pos = 7;
    
    crc_value = crc16(cmdbuf, cmdbuf[5]-6, 0);
    cmdbuf[pos] = crc_value>>8 & 0xFF;
    cmdbuf[pos+1] = crc_value & 0xFF;
    return 0;
}

S32 cmdbuf_update_single_room_config(cmd_id_e cmd_id, U32 id, U32 mode, U32 value)
{
    char *cmdbuf = g_rs232_databuf[cmd_id];

    if(cmd_id == CMD_ID_CTRL_ALL)
    {
        cmdbuf[6] = mode;
        cmdbuf[8+id] = value;  /* ID=0~16*/
    }
    else if(cmd_id == CMD_ID_CTRL_SINGLE)
    {
        cmdbuf[6] = mode;
        cmdbuf[7] = id;
        cmdbuf[8] = value;;    /* ID=0~16*/
    }
    else
    {
        DNQ_ERROR(DNQ_MOD_MCU, "error command type[%d]! type should be 0 or 1!", cmd_id);
        return -1;
    }
    
    return 0;
}

S32 cmdbuf_update_whole_room_config(cmd_id_e cmd_id, U32 mode, U32 value)
{
    int  i;
    char *cmdbuf = g_rs232_databuf[cmd_id];
    
    cmdbuf[6] = mode;
    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        cmdbuf[8+i] = value; /* ID=0~16*/
    }
    return 0;
}

static S32 recv_data_verify(cmd_id_e cmd_id, U8 *data, U32 len)
{
    U16 CRC1 = 0;
    U16 CRC2 = 0;
    U32 flag = 0;
    U32 value = 0;
    S32 error_code = 0;
    
    CRC1 = crc16(data, len-6, 0);
    switch(cmd_id)
    {
        case CMD_ID_CTRL_ALL:
        case CMD_ID_CTRL_SINGLE:
        case CMD_ID_SET_TIME:
            CRC2 = (data[7]&0xFF)<<8 | data[8]&0xFF;
            flag = data[4];
            value = data[6];
            if(CRC1 != CRC2)
                error_code = ERR_CRC;
            else if(flag != 0xBB)
                error_code = ERR_FLAG;
            else if(value != 0x10 && value != 0x11)
                error_code = ERR_VALUE;
            else if(value == 0x10)
                error_code = ERR_AGAIN;
            break;
        case CMD_ID_GET_TIME:
            CRC2 = (data[13]&0xFF)<<8 | data[14]&0xFF;
            flag = data[4];
            if(CRC1 != CRC2)
                error_code = ERR_CRC;
            else if(flag != 0xBB)
                error_code = ERR_FLAG;
            else if(data[6] != 0x12)
                error_code = ERR_CMD_FLAG;
            else if((data[7] < 17) 
                 || (data[8] < 1) ||(data[8] > 12)
                 || (data[9] < 1) ||(data[9] > 31)
                 || (data[10] < 0) ||(data[10] > 23)
                 || (data[11] < 0) ||(data[11] > 59)
                 || (data[12] < 0) ||(data[12] > 59))
                 error_code = ERR_TIME;
            break;
        case CMD_ID_HEARTBEAT:
            CRC2 = (data[7]&0xFF)<<8 | data[8]&0xFF;
            flag = data[4];
            value = data[6];
            
            if(CRC1 != CRC2)
                error_code = ERR_CRC;
            else if(value > 0x12 || value < 0x10)
                error_code =  ERR_VALUE;
            break;
        default:
            DNQ_ERROR(DNQ_MOD_MCU, "error command type[%d]! \
                type should be 0~4!", cmd_id);
            return -1;
            break;
    }
    
    if(error_code == 0)
        return 0;

    if(CMD_ID_CTRL_ALL == cmd_id)
        DNQ_ERROR(DNQ_MOD_MCU, "ctrl all room response:");
    else if(CMD_ID_CTRL_SINGLE == cmd_id)
        DNQ_ERROR(DNQ_MOD_MCU, "ctrl single room response:");
    else if(CMD_ID_SET_TIME== cmd_id)
        DNQ_ERROR(DNQ_MOD_MCU, "set datetime response:");
    else if(CMD_ID_GET_TIME == cmd_id)
        DNQ_ERROR(DNQ_MOD_MCU, "get datetime response:");
    else if(CMD_ID_HEARTBEAT== cmd_id)
        DNQ_ERROR(DNQ_MOD_MCU, "heartbeat response:");
                
    if(error_code == ERR_CRC)
        DNQ_ERROR(DNQ_MOD_MCU, "crc error! crc1=%d, crc2=%d!", CRC1, CRC2);
    else if(error_code == ERR_FLAG)
        DNQ_ERROR(DNQ_MOD_MCU, "flag error! flag=%d!", flag);
    else if(error_code == ERR_VALUE)
        DNQ_ERROR(DNQ_MOD_MCU, "value error! value=%d!", value);
    else if(error_code == ERR_AGAIN)
        DNQ_WARN(DNQ_MOD_MCU, "recv mcu response! val=%d, need send again!!", value);
    else if(error_code == ERR_TIME)
        DNQ_ERROR(DNQ_MOD_MCU, "date error! %04d-%02d-%02d %02d:%02d:%02d!", \
        2000+data[7], data[8], data[9], data[10], data[11], data[12]);   

    return error_code;
}

static S32 send_cmd_to_mcu(cmd_id_e cmd_id)
{
    S32 ret;
    ret = dnq_mcu_uart_write(g_rs232_databuf[cmd_id], g_rs232_databuf[cmd_id][5]);
    return ret;
}

static S32 recv_cmd_from_mcu(cmd_id_e cmd_id, U8 *recvbuf, U32 len)
{
    S32 time = 3;
    S32 ret = 0;
    S32 rlen = 0;
    S32 total_len = 0;
    U16 crc16 = 0;
    while(time--)
    {
        rlen = dnq_mcu_uart_read(recvbuf+total_len, len-total_len);
        if(rlen < 0)
        {
            DNQ_ERROR(DNQ_MOD_MCU, "recv error!");
            return -1;
        }
        total_len += rlen;
        if(total_len >= len)  /* recv complete */
            break;
        dnq_msleep(50);
    }

    if(time < 0)
    {
        DNQ_ERROR(DNQ_MOD_MCU, "uart recv data timeout! received %d bytes!", total_len);
        return -1;
    }
    if(total_len > len)
    {
        DNQ_ERROR(DNQ_MOD_MCU, "uart recv data error! received %d bytes, \
            datalen is expect equal to %d!", total_len, len);
        return -1;
    }  

    /* date verify */
    recv_data_verify(cmd_id, recvbuf, len);
    
    return 0;
}

S32 dnq_heater_ctrl_single(U32 id, U32 mode, U32 value)
{
    S32 ret;
    U8  recvbuf[64] = {0};
    
    ret = cmdbuf_update_single_room_config(CMD_ID_CTRL_SINGLE, id, mode, value);
    ret = cmdbuf_update_crc(CMD_ID_CTRL_SINGLE);
    mcu_lock();
    ret = send_cmd_to_mcu(CMD_ID_CTRL_SINGLE);
    ret = recv_cmd_from_mcu(CMD_ID_CTRL_SINGLE, recvbuf, MCU_RESPONSE_LEN_REPLY);
    mcu_unlock();
    return ret;
}

S32 dnq_heater_open(U32 id)
{
    S32 ret = 0;
    room_item_t *room = dnq_get_room_item(id);
    if(room->work_status == STOP_STATUS)
    {
        ret = dnq_heater_ctrl_single(id, HEATER_MODE_SWITCH, HEATER_OPEN);
        //room->work_status = WORK_STATUS;
        DNQ_INFO(DNQ_MOD_MCU, "open heart! id=%d", id);
    }

    return ret;
}

S32 dnq_heater_close(U32 id)
{
    S32 ret = 0;
    room_item_t *room = dnq_get_room_item(id);
    if(room->work_status == WORK_STATUS)
    {
        ret = dnq_heater_ctrl_single(id, HEATER_MODE_SWITCH, HEATER_CLOSE);
        //room->work_status = STOP_STATUS;
        DNQ_INFO(DNQ_MOD_MCU, "close heart! id=%d", id);
    }
    
    return ret;
}

S32 dnq_heater_ctrl_whole(U32 mode, U32 *value_array)
{
    S32 i;
    S32 ret;
    U8  recvbuf[64] = {0};

    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        cmdbuf_update_single_room_config(CMD_ID_CTRL_ALL, i, mode, value_array[i]);
    }
    ret = cmdbuf_update_crc(CMD_ID_CTRL_ALL);
    mcu_lock();
    ret = send_cmd_to_mcu(CMD_ID_CTRL_ALL);
    ret = recv_cmd_from_mcu(CMD_ID_CTRL_ALL, recvbuf, MCU_RESPONSE_LEN_REPLY);
    mcu_unlock();
    return ret;
}

U32 dnq_get_current_second()
{
    //second = g_datetime.hour*3600+g_datetime.minute*60+g_datetime.second;
    return g_curr_second;
}

void dnq_set_current_second(U32 second)
{
    g_curr_second = second;
}

void dnq_get_current_datetime(datetime_t *datetime)
{
    memcpy(datetime, &g_datetime, sizeof(datetime_t));
}

S32 dnq_datetime_check(datetime_t *datetime)
{
    S32 ret = 0;
    if(datetime->year > 99 || datetime->year < 17
        || datetime->month > 12 || datetime->month <= 0
        || datetime->day > 31 || datetime->day <= 0
        || datetime->hour > 24 || datetime->hour < 0
        || datetime->minute > 60 || datetime->minute < 0
        || datetime->second > 60 || datetime->second < 0)
    {
        DNQ_ERROR(DNQ_MOD_ALL, "datetime is error! str=%04d-%02d-%02d %02d:%02d:%02d.",
            2000+datetime->year,datetime->month,datetime->day,\
            datetime->hour,datetime->minute,datetime->second);
        return -1;
    }

    return 0;
}

void dnq_datetime_print(datetime_t *datetime)
{
    DNQ_INFO(DNQ_MOD_ALL, "datetime: %04d-%02d-%02d %02d:%02d:%02d.", \
            2000+datetime->year,datetime->month,datetime->day,\
            datetime->hour,datetime->minute,datetime->second);
}

S32 dnq_rtc_set_datetime(datetime_t *datetime)
{
    S32 ret;
    U8  recvbuf[64] = {0};

    if(dnq_datetime_check(datetime) < 0)
        return -1;

    ret = cmdbuf_update_time(CMD_ID_SET_TIME, datetime);
    ret = cmdbuf_update_crc(CMD_ID_SET_TIME);
    mcu_lock();
    ret = send_cmd_to_mcu(CMD_ID_SET_TIME);
    ret = recv_cmd_from_mcu(CMD_ID_SET_TIME, recvbuf, MCU_RESPONSE_LEN_REPLY);
    mcu_unlock();
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_MCU, "set datatime error!");
        return -1;
    }
    
    DNQ_INFO(DNQ_MOD_MCU, "set_datetime: ret=%d\n", ret);
    return ret;
}

S32 dnq_rtc_get_datetime_str(U8 *datetime)
{
    S32 ret;
    U8  recvbuf[64];
    mcu_lock();
    ret = send_cmd_to_mcu(CMD_ID_GET_TIME);
    ret = recv_cmd_from_mcu(CMD_ID_GET_TIME, recvbuf, MCU_RESPONSE_LEN_GETTIME);
    mcu_unlock();
    if(ret < 0)
        return -1;

    memcpy(datetime, &recvbuf[7], 6);
    datetime[7] = '\0';
    return ret;
}

S32 dnq_rtc_get_datetime(datetime_t *datetime)
{
    S32 ret;
    U32 second;
    U8  str[16] = {0};

    ret = dnq_rtc_get_datetime_str(str);
    if(ret < 0)
        return -1;
    
    datetime->year = str[0];
    datetime->month = str[1];
    datetime->day = str[2];
    datetime->hour = str[3];
    datetime->minute = str[4];
    datetime->second = str[5];

    if(dnq_datetime_check(datetime) < 0)
        return -1;
    
    memcpy(&g_datetime, datetime, sizeof(datetime_t));

    second = g_datetime.hour*3600+g_datetime.minute*60+g_datetime.second;
    dnq_set_current_second(second);

    DNQ_INFO(DNQ_MOD_MCU, "datetime: %04d-%02d-%02d %02d:%02d:%02d.", \
            2000+datetime->year,datetime->month,datetime->day,\
            datetime->hour,datetime->minute,datetime->second);
    return ret;
}

S32 dnq_rtc_datetime_sync(datetime_t *new_datetime)
{
    U32 need_update = 0;
    datetime_t current_datetime = {0};

    if(dnq_datetime_check(new_datetime) < 0)
        return -1;
    
    dnq_get_current_datetime(&current_datetime);

    /* rtc时间已经不准，要更新 */
    if(new_datetime->year != current_datetime.year
        || new_datetime->month != current_datetime.month
        || new_datetime->day != current_datetime.day
        || new_datetime->hour != current_datetime.hour
        || new_datetime->minute != current_datetime.minute)
    {
        need_update = 1;
    }
    
    if(need_update)
    {
        dnq_rtc_set_datetime(new_datetime);
        dnq_datetime_print(new_datetime);
    }
    
    return 0;
}

S32 dnq_timestr_to_datetime(U8 *time_str, datetime_t *datetime)
{
    U8 time_buffer[32] = {0};
    if(!time_str || !datetime)
    {
        DNQ_ERROR(DNQ_MOD_MCU, "invalid param! time_str=0x%08x, datetime=0x%08x\n",
        time_str, datetime);
        return -1;
    }

    strcpy(time_buffer, time_str);
    
    time_buffer[4] = '\0';
    time_buffer[7] = '\0';
    time_buffer[10] = '\0';
    time_buffer[13] = '\0';
    time_buffer[16] = '\0';
    time_buffer[19] = '\0';
    datetime->year = atoi(&time_buffer[2]);
    datetime->month = atoi(&time_buffer[5]);
    datetime->day = atoi(&time_buffer[8]);
    datetime->hour = atoi(&time_buffer[11]);
    datetime->minute = atoi(&time_buffer[14]);
    datetime->second = atoi(&time_buffer[17]);
    return 0;
}

S32 dnq_mcu_heartbeat_check()
{
    S32 ret;
    U8  recvbuf[64] = {0};
    ret = cmdbuf_update_crc(CMD_ID_HEARTBEAT);
    ret = send_cmd_to_mcu(CMD_ID_HEARTBEAT);
    ret = recv_cmd_from_mcu(CMD_ID_HEARTBEAT, recvbuf, MCU_RESPONSE_LEN_HEART);
    return ret;
}

S32 dnq_open_all_heater()
{
    S32 i;
    S32 status[DNQ_ROOM_CNT];
    
    for(i=0;i<DNQ_ROOM_CNT;i++)
    {
        status[i] = HEATER_OPEN;
    }
    return dnq_heater_ctrl_whole(HEATER_MODE_SWITCH, status);
}

S32 dnq_close_all_heater()
{
    S32 i;
    S32 status[DNQ_ROOM_CNT];
    
    for(i=0;i<DNQ_ROOM_CNT;i++)
    {
        status[i] = HEATER_CLOSE;
    }
    return dnq_heater_ctrl_whole(HEATER_MODE_SWITCH, status);
}


S32 dnq_all_heater_init()
{
    S32 i;
    room_item_t *rooms = dnq_get_rooms();

    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        if(rooms[i].work_status = WORK_STATUS)
            dnq_heater_ctrl_single(i, HEATER_MODE_SWITCH, HEATER_OPEN);
        else if(rooms[i].work_status = WORK_STATUS)
            dnq_heater_ctrl_single(i, HEATER_MODE_SWITCH, HEATER_CLOSE);
    }
    return 0;
}

S32 dnq_heater_ctrl_test()
{
    S32 i;
    for(i=0; i<3; i++)
    {
        dnq_open_all_heater();
        dnq_msleep(100);
        dnq_close_all_heater();
        dnq_msleep(100);
    }
}

void *mcu_task(void *args)
{
    S32 ret;
    U32 count = 0;
    U32 heart_drop_cnt = 0;
    datetime_t datetime = {17,6,4,17,40,12};

    //dnq_rtc_set_datetime(&datetime);
    count = 1;
    while(1)
    {
        if(count%60 == 0)
        {
            /* get datetime from rtc */
            ret = dnq_rtc_get_datetime(&datetime);
            if(ret < 0)
                DNQ_ERROR(DNQ_MOD_MCU, "CPU->MCU get datetime error!");
            /*
            DNQ_INFO(DNQ_MOD_MCU, "get datetime: %04d-%02d-%02d %02d:%02d:%02d",\
                2000+datetime.year, datetime.month, datetime.day,\
                datetime.hour, datetime.minute, datetime.second);
                */
        }
            
        if(count%45 == 0)
        {
            /* heartbeat check */
            ret = dnq_mcu_heartbeat_check();
            if(ret < 0)
            {
                heart_drop_cnt++;
                DNQ_ERROR(DNQ_MOD_MCU, "CPU->MCU heartbeat check failed..[%d]\n",\
                    heart_drop_cnt);
                if(heart_drop_cnt >= 3)
                {
                    DNQ_ERROR(DNQ_MOD_MCU, "the MCU is offline..\n");
                }           
            }
            else
            {
                //DNQ_INFO(DNQ_MOD_MCU, "recv heartbeat !");
                heart_drop_cnt = 0;
            }
        }
        
        count++;
        dnq_sleep(1);
    }
}

S32 dnq_mcu_init()
{
    S32 ret;
    dnq_task_t *task;
    datetime_t datetime = {0};
   
    dnq_heater_ctrl_test();
    ret = dnq_rtc_get_datetime(&datetime);
    task = dnq_task_create("mcu_task", 64*2048, mcu_task, NULL);
    if(task == NULL)
        return -1;

    g_mcu_task = task;
    return 0;
}

S32 dnq_mcu_deinit()
{
    dnq_task_delete(g_mcu_task);
    return 0;
}

S32 rtc_test()
{
    datetime_t datetime = {17, 5, 13, 22, 33, 44};

    printf("---------------rtc_test---------------\n");
    printf("this is rtc_test [CPU <--> MCU] test! \n");
    dnq_rtc_set_datetime(&datetime);
    printf("set datetime: 2017-05-13 22:33:44 \n");
    sleep(1); 
    while(1)
    {
        dnq_rtc_get_datetime(&datetime);
        printf("get datatime: %04d-%02d-%02d %02d:%02d:%02d!\n", \
            2000+datetime.year,datetime.month,datetime.day,\
            datetime.hour,datetime.minute,datetime.second);
        sleep(1);
    }
    return 0;
}

S32 room_ctrl_test()
{
    U32 i , len;
    U8 cmd[] = {0xFF, 0xFE, 0xFE, 0xFF, 0xA0, 0x0D, 0xB2,\
    0x21, 0x57, 0xFE, 0xFF, 0xFF, 0xFE};
    U32 array1[16];
    U32 array2[16];
    U32 array3[16];
    U8  buffer[64];
    U32 SLEEP_MS = 200;

    printf("---------------room_ctrl_test---------------\n");
    printf("this is room_ctrl [CPU <--> MCU] test! \n");
    sleep(1);
    while(1)
    {
        memset(buffer, 0, sizeof(buffer));
        //len = dnq_mcu_uart_write(cmd, sizeof(cmd));
        //printf("write len=%d,buffer=%s\n",len,buffer);
        //dnq_heater_ctrl_single(0, 0xB1, 1);
        //sleep(1);
        for(i=0;i<DNQ_ROOM_MAX;i++)
        {
            array1[i] = HEATER_OPEN;
            array2[i] = HEATER_CLOSE;
        }

        printf("enter switch mode!!\n");
        for(i=0; i<5; i++)
        {
            printf("ctrl all room!!\n");
            dnq_heater_ctrl_whole(HEATER_MODE_SWITCH, array1);
            dnq_msleep(SLEEP_MS);
            dnq_heater_ctrl_whole(HEATER_MODE_SWITCH, array2);
            dnq_msleep(SLEEP_MS);           
        }

        for(i=0; i<DNQ_ROOM_MAX; i++)
        {
            printf("ctrl single room!!\n");
            dnq_heater_ctrl_single(i, HEATER_MODE_SWITCH, HEATER_OPEN);
            dnq_msleep(SLEEP_MS);
            dnq_heater_ctrl_single(i, HEATER_MODE_SWITCH, HEATER_CLOSE);
            dnq_msleep(SLEEP_MS);
        }
        
        for(i=0;i<DNQ_ROOM_MAX;i++)
        {
            array1[i] = HEATER_POWER_100;
            array2[i] = HEATER_POWER_75;
            array3[i] = HEATER_POWER_0;
        }
        printf("enter power mode!!\n");
        for(i=0; i<5; i++)
        {
            printf("ctrl all room!!\n");
            dnq_heater_ctrl_whole(HEATER_MODE_POWER, array1);
            dnq_msleep(SLEEP_MS);
            dnq_heater_ctrl_whole(HEATER_MODE_POWER, array2);
            dnq_msleep(SLEEP_MS);
            dnq_heater_ctrl_whole(HEATER_MODE_POWER, array3);
            dnq_msleep(SLEEP_MS);
        }
    }
    return 0;

}

