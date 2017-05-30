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
#include "ngx_palloc.h"
#include "dnq_gpio.h"
#include "dnq_uart.h"
#include "dnq_os.h"
#include "dnq_log.h"
#include "dnq_lcd.h"
#include "dnq_mcu.h"

static uart_data_t *g_databuf_ctrlall;
static uart_data_t *g_databuf_ctrlsingle;
static datetime_t g_datatime;

static char g_rs232_databuf[5][64] =
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

static char g_rs485_cmdbuf[2][64] =
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

S32 recv_data_verify(cmd_id_e cmd_id, U8 *data, U32 len)
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
        DNQ_ERROR(DNQ_MOD_MCU, "ctrl all room response:");
    else if(CMD_ID_SET_TIME== cmd_id)
        DNQ_ERROR(DNQ_MOD_MCU, "set datetime response:");
    else if(CMD_ID_GET_TIME == cmd_id)
        DNQ_ERROR(DNQ_MOD_MCU, "set datetime response:");
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
        total_len += rlen;
        if(total_len >= len)  /* recv complete */
            break;
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
    ret = send_cmd_to_mcu(CMD_ID_CTRL_SINGLE);
    ret = recv_cmd_from_mcu(CMD_ID_CTRL_SINGLE, recvbuf, MCU_RESPONSE_LEN_REPLY);
    return ret;
}

S32 dnq_heater_open(U32 id)
{
    S32 ret;
    ret = dnq_heater_ctrl_single(id, HEATER_MODE_SWITCH, HEATER_OPEN);
    return ret;
}

S32 dnq_heater_close(U32 id)
{
    S32 ret;
    ret = dnq_heater_ctrl_single(id, HEATER_MODE_SWITCH, HEATER_CLOSE);
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
    ret = send_cmd_to_mcu(CMD_ID_CTRL_ALL);
    ret = recv_cmd_from_mcu(CMD_ID_CTRL_ALL, recvbuf, MCU_RESPONSE_LEN_REPLY);
    return ret;
}

S32 dnq_rtc_set_datetime(datetime_t *datetime)
{
    S32 ret;
    U8  recvbuf[64] = {0};

    ret = cmdbuf_update_time(CMD_ID_SET_TIME, datetime);
    ret = cmdbuf_update_crc(CMD_ID_SET_TIME);
    ret = send_cmd_to_mcu(CMD_ID_SET_TIME);
    ret = recv_cmd_from_mcu(CMD_ID_SET_TIME, recvbuf, MCU_RESPONSE_LEN_REPLY);
    if(ret < 0)
        return -1;
    return ret;
}

S32 dnq_rtc_get_datetime_str(U8 *datetime)
{
    S32 ret;
    U8  recvbuf[64];
    ret = send_cmd_to_mcu(CMD_ID_GET_TIME);
    ret = recv_cmd_from_mcu(CMD_ID_GET_TIME, recvbuf, MCU_RESPONSE_LEN_GETTIME);
    if(ret < 0)
        return -1;
    memcpy(datetime, &recvbuf[7], 6);
    datetime[7] = '\0';
    return ret;
}

S32 dnq_rtc_get_datetime(datetime_t *datetime)
{
    S32 ret;
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

    return ret;
}

U32 dnq_current_time()
{
    U32 second;
    second = g_datatime.hour*3600+g_datatime.minute*60+g_datatime.second;
    return second;
}

void dnq_current_datetime(datetime_t *datetime)
{
    memcpy(datetime, &g_datatime, sizeof(datetime_t));
}

S32 dnq_rs485_ctrl_enable()
{
    S32 ret = 0;
    ret |= dnq_gpio_open(GPIO_PH11);
    ret |= dnq_gpio_set_direction(GPIO_PH11, GPIO_OUT);
    return ret;
}

S32 dnq_rs485_ctrl_disable()
{
    S32 ret = 0;
    ret |= dnq_gpio_close(GPIO_PH11);
    return ret;
}

S32 dnq_rs485_ctrl_high()
{
    S32 ret = 0;
    ret |= dnq_gpio_write_bit(GPIO_PH11, GPIO_HIGH);
    return ret;
}

S32 dnq_rs485_ctrl_low()
{
    S32 ret = 0;
    ret |= dnq_gpio_write_bit(GPIO_PH11, GPIO_LOW);
    return ret;
}

S32 dnq_get_room_temperature(U32 room_id)
{
    S32   ret;
    S32   temperature;
    U8 *cmdbuf = g_rs485_cmdbuf[0];
    U8  recvbuf[64] = {0};
    U16 crc_value;

    cmdbuf[6] = room_id; /* Fixed! */

    printf("room_id=%d, len=%d\n",room_id, cmdbuf[5]-6);
    crc_value = crc16(cmdbuf, cmdbuf[5]-6, 0);
    cmdbuf[7] = crc_value>>8 & 0xFF;
    cmdbuf[8] = crc_value & 0xFF;

#if 1
    dnq_rs485_ctrl_high();
    
    printf("cmdbuf:\n");
    for(ret=0;ret<cmdbuf[5];ret++)
        printf("%02x ", cmdbuf[ret]);
    printf("\n");
    
    ret = dnq_sensor_uart_write(cmdbuf, SENSOR_REQUEST_LEN);
    if(ret < 0)
        printf("dnq_sensor_uart_write error!\n");
    //dnq_sensor_uart_sync();
    dnq_msleep(500);
    #endif
    
    dnq_rs485_ctrl_low(); 
    
    ret = dnq_sensor_uart_read(recvbuf, SENSOR_RESPONSE_LEN);
    if(ret == 0)
    {
        return -1;
    }
    printf("recv len:%d\n", ret);
    for(ret=0;ret<16;ret++)
        printf("%02x ", recvbuf[ret]);
    printf("\n");

    temperature = recvbuf[7]<<8|recvbuf[8];
    DNQ_INFO(DNQ_MOD_MCU, "room %d temperature is %d'C!", room_id, temperature);
    //exit(1);

    return temperature;
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

/*  
* function:
* 1: get and sync localtime from mcu [RS232]
* 2: get and sync room temperature from sensor [RS485]
* 3: heartbeat check
*
*/

void *sensor_task(void *args)
{
    U32 i;
    S32 temperature;
    
    dnq_msg_t msg = {0};
    
    /* get room temperature from sensor */
    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        temperature = dnq_get_room_temperature(i);
        if(temperature < 0)
        {
            dnq_sleep(1);
            continue;
        }

        msg.Class = MSG_CLASS_MCU;
        msg.code = i;             /* room's id */
        msg.lenght = temperature; /* room's temperature */

        /* update room's current temperature */
        send_msg_to_lcd(&msg);
        
        dnq_sleep(1);
    }
}

void *mcu_task(void *args)
{
    S32 ret;
    U32 count = 0;
    U32 heart_drop_cnt = 0;
    datetime_t datetime = {17,5,6,10,11,12};

    dnq_rtc_set_datetime(&datetime);
    count = 0;
    while(1)
    {
        if(count%5 == 0)
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
            
        if(count%10 == 0)
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
    dnq_rs485_ctrl_enable();
    dnq_heater_ctrl_test();
    

    task = dnq_task_create("mcu_task", 64*2048, mcu_task, NULL);
    if(task == NULL)
        return -1;
    
    //task = dnq_task_create("sensor_task", 64*2048, sensor_task, NULL);
    if(task == NULL)
        return -1;
    
    return 0;
}

S32 dnq_mcu_deinit()
{
    dnq_rs485_ctrl_disable();
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

S32 rs485_test()
{
    S32 i;
    S32 val;
    U8 buffer[16] = "nihao !!!!\n";
    printf("---------------rs485_test---------------\n");
    printf("this is rs485 uart [CPU <--> SENSOR] test! \n");
    while(1)
    {
    #if 0
        for(i=0; i<16; i++)
        {
            dnq_sensor_uart_write(buffer, strlen(buffer));
            printf("485 uart write test \n");
            sleep(1);
        }
    #endif
        for(i=0; i<16; i++)
        {
            printf("id[%d]: --get temperature begin--.\n", i, val);
            val = dnq_get_room_temperature(i);
            printf("id[%d]: --get temperature end--.\n", i, val);
            sleep(2);
        }
    }
}

