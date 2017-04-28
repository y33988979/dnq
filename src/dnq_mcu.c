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

typedef enum uart_cmd
{
    UART_CMD_CTRL_ALL,
    UART_CMD_CTRL_SINGLE,
    UART_CMD_SET_TIME,
    UART_CMD_GET_TIME
}uart_cmd_e;

typedef struct uart_data
{
    int  len;
    char data[64];
}uart_data_t;

static uart_data_t *g_databuf_ctrlall;
static uart_data_t *g_databuf_ctrlsingle;

static char g_rs232_databuf[5][64] =
{
    //ctrl all rooms
    {
        0xFF,0xFE,0xFE,0xFF,  /* frame header, 帧头 */
        0xAA,                 /* Flag , 硬件标志位*/
        0x26,                 /* data lenght, 数据长度 */
        0xB0,                 /* mode, 0xB0: power mode, 0xB1: switch mode */
        0xFF,                 /* ctrl all rooms */
        /*
        * 24路房间的配置
        * 调压模式: 0x01表示100%功率，0x02表示75%功率,0x03表示关闭
        * 继电器模式: 忽略0x02，0x01表示打开，0x03表示关闭
        */
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

        0x00,0x00,            /* CRC16 */
        0xFE,0xFF,0xFF,0xFE   /* frame footer */
    },

    //ctrl single room
    {
        0xFF,0xFE,0xFE,0xFF,  /* frame header, 帧头 */
        0xAA,                 /* Flag , 硬件标志位*/
        0x00,                 /* data lenght, 数据长度 */
        0xB0,                 /* mode, 0xB0: power mode, 0xB1: switch mode */
        
        0x00,                 /* ctrl single room, 0x00~0x18 */               
        0x00,                 /* power mode:  0x01:100%, 0x02:75%, 0x03:close */ 
                              /* switch mode: 0x01:open, 0x03:close */ 

        0x00,0x00,            /* CRC16 */
        0xFE,0xFF,0xFF,0xFE   /* frame footer */
    },
    
};


static char g_rs485_cmdbuf[][5] =
{
  0  
};


S32 send_cmd_prepare(uart_cmd_e cmd_id)
{
    char *cmdbuf = g_rs232_databuf[cmd_id];
    
    cmdbuf[9] = crc16(cmdbuf, cmdbuf[5], 0);
    return 0;
}


S32 send_cmd_to_mcu(uart_cmd_e cmd_id)
{
    S32 ret;
    ret = dnq_mcu_uart_write(g_rs232_databuf[cmd_id], g_rs232_databuf[cmd_id][5]);
    return 0;
}

S32 heater_config_single(U32 id, U32 mode, U32 value)
{
    char *cmdbuf = g_rs232_databuf[1];
    
    cmdbuf[6] = mode;
    cmdbuf[7] = id;
    cmdbuf[8] = value;
    cmdbuf[9] = crc16(cmdbuf, cmdbuf[5], 0);
    return 0;
}

S32 heater_config_single_for_whole(U32 id, U32 mode, U32 value)
{
    char *cmdbuf = g_rs232_databuf[0];
    
    cmdbuf[6] = mode;
    cmdbuf[8+id] = value;  /* ID=0~16*/
    cmdbuf[32] = crc16(cmdbuf, cmdbuf[5], 0);
    return 0;
}

S32 heater_config_whole(U32 mode, U32 value)
{
    int  i;
    char *cmdbuf = g_rs232_databuf[0];
    
    cmdbuf[6] = mode;
    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        cmdbuf[8+i] = value; /* ID=0~16*/
    }
    cmdbuf[32] = crc16(cmdbuf, cmdbuf[5], 0);
    return 0;
}

S32 dnq_heater_ctrl_single(U32 id, U32 mode, U32 value)
{
    S32 ret;
    ret = heater_config_single(id, mode, value);
    ret = send_cmd_prepare(UART_CMD_CTRL_SINGLE);
    ret = send_cmd_to_mcu(UART_CMD_CTRL_SINGLE);
    return ret;
}

S32 dnq_heater_ctrl_whole(U32 mode, U32 *value_array)
{
    S32 i;
    S32 ret;
    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        ret = heater_config_single_for_whole(i, mode, value_array[i]);
    }
    ret = send_cmd_prepare(UART_CMD_CTRL_ALL);
    ret = send_cmd_to_mcu(UART_CMD_CTRL_ALL);
    return ret;
}



S32 dnq_mcu_init()
{
    
    return 0;
}

S32 dnq_mcu_deinit()
{
    
    return 0;
}

