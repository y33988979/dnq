/* 
 * cpu and mcu communicate Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  this is a communicate Program between cpu and mcu.
 *  mcu1:  heater control      protocol: RS232
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
#include "dnq_sensor.h"

dnq_task_t *g_sensor_task = NULL;
static U8 g_rs485_cmdbuf[2][64] =
{
    // 0: get room temperature
    {
        0xFF,0xFE,0xFE,0xFF,  
        0xF0,       /* get temperature, 获取温度 */
        0x0D,       /* data lenght, 数据长度 */            
        0x00,       /* room_id, 房间id */             
                             
        0x00,0x00,  /* crc16 */
        0xFE,0xFF,0xFF,0xFE   
    },
};

static S32 dnq_rs485_ctrl_enable()
{
    S32 ret = 0;
    ret |= dnq_gpio_open(GPIO_PH11);
    ret |= dnq_gpio_set_direction(GPIO_PH11, GPIO_OUT);
    return ret;
}

static S32 dnq_rs485_ctrl_disable()
{
    S32 ret = 0;
    ret |= dnq_gpio_close(GPIO_PH11);
    return ret;
}

static S32 dnq_rs485_ctrl_high()
{
    S32 ret = 0;
    ret |= dnq_gpio_write_bit(GPIO_PH11, GPIO_HIGH);
    return ret;
}

static S32 dnq_rs485_ctrl_low()
{
    S32 ret = 0;
    ret |= dnq_gpio_write_bit(GPIO_PH11, GPIO_LOW);
    return ret;
}

static S32 send_cmd_to_sensor(U8 *cmdbuf, U32 len)
{
    S32 ret = 0;
    ret = dnq_sensor_uart_write(cmdbuf, len);
    return ret;
}

static S32 recv_cmd_from_sensor(U8 *cmdbuf, U32 len)
{
    S32 time = 3;
    S32 ret = 0;
    S32 rlen = 0;
    S32 total_len = 0;
    U16 crc16 = 0;
    while(time--)
    {
        rlen = dnq_sensor_uart_read(cmdbuf+total_len, len-total_len);
        if(rlen < 0)
        {
            DNQ_ERROR(DNQ_MOD_SENSOR, "recv error!");
            return -1;
        }
        total_len += rlen;
        if(total_len >= len)  /* recv complete */
            break;
        dnq_msleep(100);
    }

    if(time < 0)
    {
        DNQ_DEBUG(DNQ_MOD_SENSOR, "recv timeout! received %d bytes!", total_len);
        return -1;
    }
    if(total_len != len)
    {
        DNQ_ERROR(DNQ_MOD_SENSOR, "datalen error! received %d bytes, \
but %d bytes is expected!", total_len, len);
        return -1;
    }  

    return 0;
}

static S32 dnq_get_room_temperature(U32 room_id)
{
    S32   ret;
    S32   temperature;
    U8 *cmdbuf = g_rs485_cmdbuf[0];
    U8  recvbuf[64] = {0};
    U16 crc_value;

    cmdbuf[6] = room_id; /* Fixed! */

    crc_value = crc16(cmdbuf, cmdbuf[5]-6, 0);
    cmdbuf[7] = crc_value>>8 & 0xFF;
    cmdbuf[8] = crc_value & 0xFF;

#ifndef DNQ_RS485_AUTO_TXRX_SUPPORT 
    dnq_rs485_ctrl_high();
#endif
    
    ret = dnq_sensor_uart_write(cmdbuf, SENSOR_REQUEST_LEN);
    if(ret < 0)
        printf("dnq_sensor_uart_write error!\n");
    dnq_sensor_uart_sync();
    dnq_msleep(10);
    
#ifndef DNQ_RS485_AUTO_TXRX_SUPPORT 
    dnq_rs485_ctrl_low(); 
#endif

    ret = recv_cmd_from_sensor(recvbuf, SENSOR_RESPONSE_LEN);
    if(ret < 0)
    {
        return -1;
    }

    temperature = recvbuf[7]<<8|recvbuf[8];
    DNQ_DEBUG(DNQ_MOD_SENSOR, "room %d temperature is %d.%d'C!",\
        room_id, temperature/100, (temperature%100)/10);

    return temperature;
}


static S32 update_sn_status(U32 room_id, U32 status)
{
    dnq_msg_t msg = {0};
    room_item_t *room = dnq_get_room_item(room_id);

    if(room->sn_status == status)
        return 0;
    
    msg.Class = MSG_CLASS_MCU;
    msg.code = room_id;          /* room's id */
    msg.lenght = ROOM_ITEM_SN_STATUS; /* room's item id */
    msg.payload = (void*)status; /* SN work status */

    /* update room's SN status */
    return send_msg_to_lcd(&msg);
}

static S32 update_temperature(U32 room_id, U32 temperature)
{
    dnq_msg_t msg = {0};
    
    msg.Class = MSG_CLASS_MCU;
    msg.code = room_id;               /* room's id */
    msg.lenght = ROOM_ITEM_CURRENT_TEMP; /* room's item id */
    msg.payload = (void*)temperature; /* room's temperature */

    /* update room's current temperature */
    return send_msg_to_lcd(&msg);
}

/*  
* function:
* 1、get and sync room temperature from sensor [RS485]
*
*/
void *sensor_task(void *args)
{
    U32 i;
    S32 temperature;
    
    room_item_t *rooms = dnq_get_rooms();

    while(1)
    {
        /* get room temperature from sensor */
        for(i=0; i<DNQ_ROOM_CNT; i++)
        {
            if(i >= 4)
                continue;
            temperature = dnq_get_room_temperature(i);
            if(temperature < 0)
            {
                update_sn_status(i, STOP_STATUS);
                dnq_sleep(1);
                continue;
            }

            update_sn_status(i, WORK_STATUS);
            update_temperature(i, temperature);
            
            dnq_sleep(DNQ_SENSOR_SCAN_INTERVAL);
        }
    }
}

S32 dnq_sensor_init()
{
    S32 ret;
    dnq_task_t *task;
    
#ifndef DNQ_RS485_AUTO_TXRX_SUPPORT 
    dnq_rs485_ctrl_enable();
#endif
    
    task = dnq_task_create("sensor_task", 64*2048, sensor_task, NULL);
    if(task == NULL)
        return -1;

    g_sensor_task = task;
    return 0;
}

S32 dnq_sensor_deinit()
{   
    dnq_task_delete(g_sensor_task);
#ifndef DNQ_RS485_AUTO_TXRX_SUPPORT 
    dnq_rs485_ctrl_disable();
#endif
    return 0;
}

S32 sensor_test()
{
    S32 i;
    S32 val;
    U8 buffer[16] = "nihao !!!!\n";
    printf("---------------rs485_test---------------\n");
    printf("this is rs485 uart [CPU <--> SENSOR] test! \n");
    while(1)
    {
        for(i=0; i<4; i++)
        {
            printf("get room[%d]'s temperature start\n", i);
            val = dnq_get_room_temperature(i);
            printf("get room[%d]'s temperature end\n", i);
            sleep(1);
        }
    }
}

