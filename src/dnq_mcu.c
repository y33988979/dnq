/* 
 * cpu and mcu communicate Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  this is a communicate Program between cpu and mcu.
 *  mcu1:  heater control      protocol: RS232
 *  mcu2:  temperature sensor  protocol: RS485
 *
 * Note : 
 */


#include "dnq_common.h"
#include "dnq_config.h"
#include "ngx_palloc.h"
#include "dnq_uart.h"
#include "dnq_log.h"
#include "dnq_mcu.h"

/* heater operate mode */
#define HEATER_MODE_POWER      0xB0
#define HEATER_MODE_SWITCH     0xB1

/* heater operate status value */
#define HEATER_POWERON    0
#define HEATER_POWEROFF   1

#define HEATER_POWER_100   0
#define HEATER_POWER_75    1
#define HEATER_POWER_0     2   /* poweroff */

/* command define */
#define CMD_IDX_ALL_CTRL       0
#define CMD_IDX_SINGLE_CTRL    1

/* mcu response data lenght */
#define MCU_RESPONSE_LEN_REPLY    12
#define MCU_RESPONSE_LEN_GETTIME  18
#define MCU_RESPONSE_LEN_HEART    12

/* sensor response data lenght */
#define SENSOR_RESPONSE_LEN       12

/* error code */
#define ERR_HEADER     -1
#define ERR_FLAG       -2
#define ERR_CMD_FLAG   -3
#define ERR_VALUE      -4
#define ERR_TIME       -5
#define ERR_CRC        -6
#define ERR_FOOTER     -7
#define ERR_AGAIN      -8

typedef enum cmd_id
{
    CMD_ID_CTRL_ALL,
    CMD_ID_CTRL_SINGLE,
    CMD_ID_SET_TIME,
    CMD_ID_GET_TIME
}cmd_id_e;

typedef struct uart_data
{
    int  len;
    char data[64];
}uart_data_t;

static uart_data_t *g_databuf_ctrlall;
static uart_data_t *g_databuf_ctrlsingle;

static char g_rs232_databuf[5][64] =
{
    // 0: ctrl all rooms
    {
        0xFF,0xFE,0xFE,0xFF,  /* frame header, 帧头 */
        0xA0,                 /* Flag , 标志位*/
        0x26,                 /* data lenght, 数据长度 */
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
        0x00,                 /* data lenght, 数据长度 */
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
        0x12,                 /* data lenght, 数据长度 */
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
        0x0C,                 /* data lenght, 数据长度 */
        0xB2,                 /* get datetime flag */
                             
        0x00,0x00,            /* CRC16 */
        0xFE,0xFF,0xFF,0xFE   /* frame footer */
    },
    
};

static char g_rs485_cmdbuf[2][64] =
{
    // 0: get room temperature
    {
        0xFF,0xFE,0xFE,0xFF,  
        0xA0,                 
        0x0C,                 
        0xB2,                 
                             
        0x00,0x00,            
        0xFE,0xFF,0xFF,0xFE   
    },
};

S32 cmdbuf_update_time(cmd_id_e cmd_id, U8 *datetime)
{
    U32  pos = 0;
    char *cmdbuf = g_rs232_databuf[cmd_id];

    cmdbuf[7] = datetime[0];
    cmdbuf[8] = datetime[1];
    cmdbuf[9] = datetime[2];
    cmdbuf[10] = datetime[3];
    cmdbuf[11] = datetime[4];
    cmdbuf[12] = datetime[5];
    return 0;
}

S32 cmdbuf_update_crc(cmd_id_e cmd_id)
{
    U32  pos = 0;
    char *cmdbuf = g_rs232_databuf[cmd_id];

    if(cmd_id == CMD_ID_CTRL_ALL)
        pos = 24;
    else if(cmd_id == CMD_ID_CTRL_SINGLE)
        pos = 9;
    else if(cmd_id == CMD_ID_SET_TIME)
        pos = 13;
    else if(cmd_id == CMD_ID_GET_TIME)
        pos = 7;
    cmdbuf[pos] = crc16(cmdbuf, cmdbuf[5], 0);
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

S32 recv_data_verify(cmd_id_e cmd_id, U8 *data, U32 len)
{
    U16 CRC1 = 0;
    U16 CRC2 = 0;
    U32 flag = 0;
    U32 value = 0;
    S32 error_code = 0;
    
    CRC1 = crc16(data, len, 0);
    switch(cmd_id)
    {
        case CMD_ID_CTRL_ALL:
        case CMD_ID_CTRL_SINGLE:
        case CMD_ID_SET_TIME:
            CRC2 = *(U16*)&data[7];
            flag = data[4];
            value = data[5];
            if(CRC1 != CRC2)
                error_code = ERR_CRC;
            else if(flag != 0xBB)
                error_code = ERR_FLAG;
            else if(value != 0x10 || value != 0x11)
                error_code = ERR_VALUE;
            else if(value == 0x11)
                error_code = ERR_AGAIN;
            break;
        case CMD_ID_GET_TIME:
            CRC2 = *(U16*)&data[13];
            flag = data[4];
            if(CRC1 != CRC2)
                error_code = ERR_CRC;
            else if(flag != 0xBB)
                error_code = ERR_FLAG;
            else if(data[6] != 0x12)
                error_code = ERR_CMD_FLAG;
            else if((data[7] < 2017) 
                 || (data[8] < 1) ||(data[8] > 12)
                 || (data[9] < 1) ||(data[9] > 31)
                 || (data[10] < 0) ||(data[10] > 23)
                 || (data[11] < 0) ||(data[11] > 59)
                 || (data[12] < 0) ||(data[12] > 59))
                 error_code = ERR_TIME;
            break;
        default:
            DNQ_ERROR(DNQ_MOD_MCU, "error command type[%d]! type should be 0~3!", cmd_id);
            break;
    }
    

    if(error_code == 0)
        return 0;
    
    if(error_code == ERR_CRC)
        DNQ_ERROR(DNQ_MOD_MCU, "crc error! crc1=%d, crc2=%d!", CRC1, CRC2);
    if(error_code == ERR_FLAG)
        DNQ_ERROR(DNQ_MOD_MCU, "flag error! flag=%d!", flag);
    if(error_code == ERR_VALUE)
        DNQ_ERROR(DNQ_MOD_MCU, "value error! value=%d!", value);
    if(error_code == ERR_AGAIN)
        DNQ_WARN(DNQ_MOD_MCU, "recv mcu response! val=%d, need send again!!", value);
    if(error_code == ERR_TIME)
        DNQ_ERROR(DNQ_MOD_MCU, "date error! %4d-%2d-%2d %2d:%2d:%2d!", \
        2000+data[7], data[8], data[9], data[10], data[11], data[12]);   

    return error_code;
}

S32 send_cmd_to_mcu(cmd_id_e cmd_id)
{
    S32 ret;
    ret = dnq_mcu_uart_write(g_rs232_databuf[cmd_id], g_rs232_databuf[cmd_id][5]);
    return ret;
}

S32 recv_cmd_from_mcu(cmd_id_e cmd_id, U8 *recvbuf, U32 len)
{
    U32 time = 5;
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
        total_len + rlen;
        if(total_len >= len)  /* recv complete */
            break;
        usleep(10*1000);
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
    ret = cmdbuf_update_single_room_config(CMD_ID_CTRL_SINGLE, id, mode, value);
    ret = cmdbuf_update_crc(CMD_ID_CTRL_SINGLE);
    ret = send_cmd_to_mcu(CMD_ID_CTRL_SINGLE);
    return ret;
}

S32 dnq_heater_ctrl_whole(U32 mode, U32 *value_array)
{
    S32 i;
    S32 ret;
    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        cmdbuf_update_single_room_config(CMD_ID_CTRL_ALL, i, mode, value_array[i]);
    }
    ret = cmdbuf_update_crc(CMD_ID_CTRL_ALL);
    ret = send_cmd_to_mcu(CMD_ID_CTRL_ALL);
    return ret;
}

S32 dnq_rtc_set_time(U8 *datetime)
{
    S32 ret;
    U8  recvbuf[64];

    ret = cmdbuf_update_time(CMD_ID_SET_TIME, datetime);
    ret = cmdbuf_update_crc(CMD_ID_SET_TIME);
    ret = send_cmd_to_mcu(CMD_ID_SET_TIME);
    ret = recv_cmd_from_mcu(CMD_ID_SET_TIME, recvbuf, MCU_RESPONSE_LEN_REPLY);
    if(ret < 0)
        return -1;
    return ret;
}

S32 dnq_rtc_get_time(U8 *datetime)
{
    S32 ret;
    U8  recvbuf[64];
    ret = send_cmd_to_mcu(CMD_ID_GET_TIME);
    ret = recv_cmd_from_mcu(CMD_ID_GET_TIME, recvbuf, MCU_RESPONSE_LEN_GETTIME);
    if(ret < 0)
        return -1;
    strncpy(datetime, &recvbuf[7], 6);
    return ret;
}

S32 dnq_room_temperature_get(U32 room_id)
{
    S32   ret;
    S32   temperature;
    char *cmdbuf = g_rs485_cmdbuf[0];
    char  recvbuf[64] = {0};

    cmdbuf[2] = room_id; /* Fixed! */
    
    ret = dnq_sensor_uart_write(cmdbuf, SENSOR_RESPONSE_LEN);
    
    ret = dnq_sensor_uart_read(recvbuf, SENSOR_RESPONSE_LEN);

    temperature = recvbuf[5];
    DNQ_INFO(DNQ_MOD_MCU, "room %d temperature is %d'C!", room_id, temperature);

    return temperature;
}

void *dnq_mcu_task()
{
    S32 i;

    
    
    while(1)
    {
        for(i=0; i<DNQ_ROOM_CNT; i++)
        {
            dnq_room_temperature_get(i);
            
            
        }
        
    }
}

S32 dnq_mcu_init()
{
    
    return 0;
}

S32 dnq_mcu_deinit()
{
    
    return 0;
}

