/* dnq lcd Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  this is a lcd interface API, for app.
 * Note : 
 */

#include "dnq_lcd.h"
#include "dnq_log.h"
#include "dnq_uart.h"
#include "dnq_config.h"
#include "dnq_os.h"

#define SIZE   1024

#define ITEM_ADDR_OFFSET      0x20

/* All items's address define */ 

/* title, datetime, header */
#define ITEM_ADDR_TITLE       0x00
#define ITEM_ADDR_DATE        0x40
#define ITEM_ADDR_HEADER      0x80

/* mac, command, network, system info */
#define ITEM_ADDR_MAC_INFO    0x0A00
#define ITEM_ADDR_NET_INFO    0x0A80
#define ITEM_ADDR_CMD_INFO    0x0B00
#define ITEM_ADDR_SYS_INFO    0x0B80

#define ITEM_ADDR_HELP_INFO   0x0C00

#define LCD_TITLE_SIZE        0x40
#define LCD_ITEM_SIZE         0x10
#define LCD_ITEM_NAME_SIZE    0x20

#define LCD_ROOM_START_ADDR   0x100
#define LCD_ROOM_SIZE         0x100
#define LCD_ROOM_END_ADDR     0x900

#define ROOM_CNT_PER_PAGE     8
#define ONE_ROOM_ITEM_CNT     12
#define ALL_ROOM_ITEM_CNT     (ONE_ROOM_ITEM_CNT*ROOM_CNT_PER_PAGE)

#define LCD_ID_ROOM_ITEM_START    3
#define LCD_ID_ROOM_ITEM_END     (LCD_ID_ROOM_ITEM_START + ALL_ROOM_ITEM_CNT -1)
#define LCD_ITEM_MAX          188

#define DEFAULT_COLOR         0
#define SECOND_COLOR          1

#define SELECT_FLAG           "-->"
#define SETTING_FLAG          "↑↓"

#define SOME_SPACE            "                                         "

#define TITLE_STR " 松花江小学-主楼-三楼西/一号箱  2017年5月15日 18:33:33"
#define HEADER_STR " 序号     房间       室温   设置温度    状态     SN    温度校准 "
#define DATE_STR "2017年5月15日 18:33:33"
     
#define MAC_INFO_STR "MAC：20-21-3E-43-FE-47-29 "
#define NET_INFO_STR "网络状态：    正常 "
#define CMD_INFO_STR "当前执行命令：加热 "
#define SYS_INFO_STR "火娃电采暖智能控制器 "
#define HELP_INFO_STR "↑:上一页  ↓:下一页  OK:设置  EXIT:返回"

typedef enum lcd_item_id
{
    ITEM_ID_TITLE = 0,
    ITEM_ID_DATE,
    ITEM_ID_HEADER,
    ITEM_ID_ROOM1 = LCD_ID_ROOM_ITEM_START, // 3
    ITEM_ID_ROOM2 = 15,
    ITEM_ID_ROOM3 = 27,
    
    /* .... */
    
    ITEM_ID_ROOM12 = LCD_ID_ROOM_ITEM_END, // 
    ITEM_ID_MAC_INFO,
    ITEM_ID_CMD_INFO,
    ITEM_ID_NET_INFO,
    ITEM_ID_SYS_INFO,
    ITEM_ID_HELP_INFO,
}lcd_item_id_e;


typedef enum room_item_id
{
    ROOM_ITEM_ID,
    ROOM_ITEM_NAME,
    ROOM_ITEM_CURRENT_TEMP,
    ROOM_ITEM_SETTING_TEMP,
    ROOM_ITEM_WORK_STATUS,
    ROOM_ITEM_SN_STATUS,
    ROOM_ITEM_TEMP_CORRECT,
    ROOM_ITEM_SELECT_FLAG
}room_item_id_e;

static U8   uart_command[256];
static U32  lcd_current_page = 0;

room_item_t g_items[16] = 
{
    {0, "三年二班", 22.1,32,"停止","正常",-2},
    {1, "门卫室", 24.2,26,"停止","正常",-2},
    {2, "走廊蓄热12", 11.2,5,"正常","正常",-2},
    {3, "走廊蓄热2", 11.2,5,"正常","正常",-2},
    {4, "科学实验室", 11.2,23,"正常","正常",-2},
    {5, "一年级教研室", 10.2,23,"停止","正常",-2},
    {6, "三年一班", 22.1,19,"正常","正常",-2},
    {7, "水房", 19.1,0,"停止","正常",-2},
    {8, "闲置房间1-2", 18.1,5,"正常","正常",-2},
    {9, "会议室南", 5.9, 5,"正常","正常",-2},
    {10, "会议室北",5.2,5,"正常","正常",-2},
    {11, "楼梯间2-3", 13.2,15,"正常","正常",-2},
    {0,0,0,0,0,0,0},
   
};

room_item_t *g_house_items;
lcd_item_t    g_lcd_items[LCD_ITEM_MAX] = 
{
    /* id, addr */
    {1, 0x40, 0x0000}, /* title */
    {2, 0x40, 0x0040}, /* date */
    {3, 0x40, 0x0080}, /* header */
    
    {4, 0x10, 0x0100}, /* room id */
    {5, 0x20, 0x0110}, /* room name */
    {6, 0x10, 0x0130}, /* room current temp */
    {7, 0x10, 0x0140}, /* room setting temp */
    {8, 0x10, 0x0150}, /* room work status */
    {9, 0x10, 0x0160}, /* room SN status */
    {10, 0x10, 0x0170}, /* room temp correct */

    {11, 0x10, 0x0180}, /* room setting temp */
    {12, 0x10, 0x0190}, /* room work status */
    {13, 0x10, 0x01A0}, /* room SN status */
    {14, 0x10, 0x01B0}, /* room temp correct */
    {15, 0x10, 0x01E0}, /* setting icon */

    /* ... */
    
};

S32 dnq_lcd_items_init()
{
    int  i, j;
    int  id, addr;
    int  item_offset;
    int  room_addr;
    lcd_item_t *items = g_lcd_items;
    char cmd_clear_text[64] = {0xA5, 0x5A, 0x00, 0x82, 0x00, 0x00};

    /* id set */
    for(i=0; i<LCD_ITEM_MAX; i++)
        items[i].id = i+1;

    /* address set */
    i = 0;
    /* title */
    items[i].addr = ITEM_ADDR_TITLE;
    items[i].size = LCD_TITLE_SIZE;
    i++;
    
    /* date */
    items[i].addr = ITEM_ADDR_DATE;
    items[i].size = LCD_TITLE_SIZE;
    i++;
    
    /* header */
    items[i].addr = ITEM_ADDR_HEADER;
    items[i].size = LCD_TITLE_SIZE;
    i++;

    /* rooms */
    for(i=0; i<DNQ_ROOM_CNT; i++)
    {
        item_offset = 0;

        for(j=0; j<ONE_ROOM_ITEM_CNT; j++)
        {
            id = LCD_ID_ROOM_ITEM_START + i*ONE_ROOM_ITEM_CNT + j;
            //items[id].addr = (i+1)*LCD_ROOM_SIZE + item_offset;
            /* 
            * LCD 每页最多显示ROOM_CNT_PER_PAGE个房间,
            * 多页显示时, 第2,3,,页与第1页room item使用同一个地址。
            */
            //if(i/ROOM_CNT_PER_PAGE > 0)
            room_addr = (i%ROOM_CNT_PER_PAGE+1)*LCD_ROOM_SIZE;
            items[id].addr = room_addr + item_offset;
            items[id].size = LCD_ITEM_SIZE;
 
            /* room name, size=0x20! */
            if(j == 1)
            {
                items[id].size += LCD_ITEM_SIZE;
            }
                
            /* setting icon, addr=0xe0! */
            /*if(j == 11)
            {
                items[id].addr += 0x30;
            }
            */
            
            item_offset += items[id].size;   
        }
    }

    i = LCD_ID_ROOM_ITEM_END + 1;
    /* MAC info */
    items[i].addr = ITEM_ADDR_MAC_INFO;
    items[i].size = LCD_TITLE_SIZE;
    i++;
    /* command info */
    items[i].addr = ITEM_ADDR_CMD_INFO;
    items[i].size = LCD_TITLE_SIZE;
    i++;
    /* network info */
    items[i].addr = ITEM_ADDR_NET_INFO;
    items[i].size = LCD_TITLE_SIZE;
    i++;
    /* system info */
    items[i].addr = ITEM_ADDR_SYS_INFO;
    items[i].size = LCD_TITLE_SIZE;
    i++;
    /* help info */
    items[i].addr = ITEM_ADDR_HELP_INFO;
    items[i].size = LCD_TITLE_SIZE*2;
    i++;

    DNQ_PRINT(DNQ_MOD_LCD, "items:\n id    addr\n");
    for(j=0; j<i; j++)
        DNQ_PRINT(DNQ_MOD_LCD, "%02d    0x%04x\n",items[j].id, items[j].addr);

    DNQ_INFO(DNQ_MOD_LCD, "all item init ok!");
    return 0;
}

S32 dnq_lcd_uart_cmd_prepare(U32 item_id, char *content, U32 color)
{
    U32  addr ;
    U32  len = 0;
    U32  room_item_idx = 0;
    U8  *cmd = uart_command;
    
    cmd[0] = 0xA5;
    cmd[1] = 0x5A;
    cmd[2] = 0x00;
    cmd[3] = 0x82;

    /* 
    * there are four items have double address for two color.
    * they are <set_temp, status, SN, correct>
    * check and change item's addr if need show another color
    */
    #if 1
    room_item_idx = (item_id-LCD_ID_ROOM_ITEM_START) % ONE_ROOM_ITEM_CNT;
    switch(room_item_idx)
    {
        case ROOM_ITEM_SETTING_TEMP:
        case ROOM_ITEM_WORK_STATUS:
        case ROOM_ITEM_SN_STATUS:
        case ROOM_ITEM_TEMP_CORRECT:
        break;
        case ROOM_ITEM_SELECT_FLAG:
        break;
    }
    #endif

    /* default color */
    addr = g_lcd_items[item_id].addr;

    /* need change color */
    if(color != DEFAULT_COLOR)
    {
        /* if id is a room item , maybe need change addr for second color */
        if(item_id >= LCD_ID_ROOM_ITEM_START 
            && item_id <= LCD_ID_ROOM_ITEM_END)
        {
            /* they are <set_temp, status, SN, correct> */
            if(room_item_idx >= 3 && room_item_idx <= 6)
                addr = g_lcd_items[item_id].addr+0x50;
        }
    }
    
    printf("g_lcd_items[%d].addr == 0x%04x\n",item_id, addr );
    
    cmd[4] = (addr >> 8) & 0xFF;
    cmd[5] = addr & 0xFF;

    len = strlen(content);
    strncpy(g_lcd_items[item_id].content, content, (len>64)?64:len);
    cmd[2] = len + 3;
    strcpy(&cmd[6], content);
    
    return (len+6);
    
}


S32 lcd_item_get_title(U8 *name)
{
}

S32 lcd_item_set_title(U8 *name)
{

}

S32 lcd_item_get_header(U32 id, U8 *name)
{
}

S32 lcd_item_set_header(U32 id, U8 *name)
{

}

S32 lcd_item_get_mac_info(U32 id, U8 *name)
{
}

S32 lcd_item_set_mac_info(U32 id, U8 *name)
{

}

S32 lcd_item_get_curr_cmd(U32 id, U8 *name)
{

}

S32 lcd_item_set_curr_cmd(U32 id, U8 *name)
{

}

S32 lcd_item_get_network_status(U32 id, U8 *name)
{

}

S32 lcd_item_set_network_status(U32 id, U8 *name)
{

}

S32 lcd_item_get_system_info(U32 id, U8 *name)
{

}

S32 lcd_item_set_system_info(U32 id, U8 *name)
{

}

S32 lcd_item_get_name(U32 id, U8 *name)
{

}

S32 lcd_item_set_name(U32 id, U8 *name)
{

}

S32 lcd_item_get_currtemp(U32 id, float *temp)
{

}

S32 lcd_item_set_currtemp(U32 id, float *temp)
{

}

S32 lcd_item_get_settemp(U32 id, float *temp)
{

}

S32 lcd_item_set_settemp(U32 id, float *temp)
{

}

S32 lcd_item_get_work_status(U32 id, U8 *work_status)
{

}

S32 lcd_item_set_work_status(U32 id, U8 *work_status)
{

}

U8* lcd_item_get_sn_status(U32 id, U8*sn_status)
{

}

U8* lcd_item_set_sn_status(U32 id, U8*sn_status)
{

}

S32 lcd_item_get_temp_offet(U32 id, U32 *offset)
{

}

S32 lcd_item_set_temp_offet(U32 id, S32 *offset)
{

}

S32 dnq_lcd_item_init()
{}

U32 room_in_current_page(U32 room_id)
{
    if((room_id/ROOM_CNT_PER_PAGE) == lcd_current_page)
        return 1;
    return 0;
}

S32 dnq_lcd_item_clear(U32 item_id)
{
    U32 len = 0;
    U8  some_blank[256] = {0};

    memset(some_blank, ' ', g_lcd_items[item_id].size);
    len = dnq_lcd_uart_cmd_prepare(item_id, some_blank, DEFAULT_COLOR);
    len = dnq_lcd_uart_write(uart_command, len);
    return len;
}

S32 dnq_lcd_item_draw(U32 item_id, U8 *content, U32 color)
{
    U32 len = 0;
    len = dnq_lcd_uart_cmd_prepare(item_id, content, color);
    len = dnq_lcd_uart_write(uart_command, len);
    return len;
}

S32 dnq_lcd_item_update(U32 item_id, U8 *content, U32 color)
{
    U32 ret = 0;
    ret = dnq_lcd_item_clear(item_id);
    ret = dnq_lcd_item_draw(item_id, content, color);
    return ret;
}

S32 dnq_lcd_title_update(U8 *content)
{
    U32 ret = 0;
    ret = dnq_lcd_item_clear(ITEM_ID_TITLE);
    ret = dnq_lcd_item_draw(ITEM_ID_TITLE, content, DEFAULT_COLOR);
    return ret;
}

S32 dnq_lcd_date_update(U8 *content)
{
    U32 ret = 0;
    ret = dnq_lcd_item_clear(ITEM_ID_DATE);
    ret = dnq_lcd_item_draw(ITEM_ID_DATE, content, DEFAULT_COLOR);
    return ret;
}

S32 dnq_lcd_header_update(U8 *content)
{
    U32 ret = 0;
    ret = dnq_lcd_item_clear(ITEM_ID_HEADER);
    ret = dnq_lcd_item_draw(ITEM_ID_HEADER, content, DEFAULT_COLOR);
    return ret;
}

S32 dnq_lcd_room_item_update(U32 room_id, U32 idx, U8 *content, U32 color)
{
    S32 ret = 0;
    U32 item_id = 0;

    if(!room_in_current_page(room_id))
        return 0;

    if(idx > ONE_ROOM_ITEM_CNT)
        DNQ_ERROR(DNQ_MOD_LCD, "room's index[%d] error! it must less then %d",\
        idx, ONE_ROOM_ITEM_CNT);
    
    item_id = LCD_ID_ROOM_ITEM_START+room_id*ONE_ROOM_ITEM_CNT + idx;
    ret = dnq_lcd_item_clear(item_id);
    ret = dnq_lcd_item_draw(item_id, content, color);
    return ret;
}

S32 dnq_lcd_room_id_update(U32 room_id, U32 id, U32 color)
{
    S32 ret = 0;
    char buf[16] = {0};
    sprintf(buf, "%02d", id);
    ret = dnq_lcd_room_item_update(room_id, ROOM_ITEM_ID, buf, color);
    return ret;
}

S32 dnq_lcd_room_name_update(U32 room_id, U8 *room_name, U32 color)
{
    S32 ret = 0;
    ret = dnq_lcd_room_item_update(room_id, ROOM_ITEM_NAME, room_name, color);
    return ret;
}

S32 dnq_lcd_room_current_temp_update(U32 room_id, float degree, U32 color)
{
    S32 ret = 0;
    char buf[16] = {0};
    sprintf(buf, "%2.1f", degree);
    ret = dnq_lcd_room_item_update(room_id, ROOM_ITEM_CURRENT_TEMP, buf, color);
    return ret;
}

S32 dnq_lcd_room_setting_temp_update(U32 room_id, float degree, U32 color)
{
    S32 ret = 0;
    char buf[16] = {0};
    sprintf(buf, "%2.1f", degree);
    ret = dnq_lcd_room_item_update(room_id, ROOM_ITEM_SETTING_TEMP, buf, color);
    return ret;
}

S32 dnq_lcd_room_work_status_update(U32 room_id, U8 *status, U32 color)
{
    S32 ret = 0;
    ret = dnq_lcd_room_item_update(room_id, ROOM_ITEM_WORK_STATUS, status, color);
    return ret;
}

S32 dnq_lcd_room_sn_status_update(U32 room_id, U8 *status, U32 color)
{
    S32 ret = 0;
    ret = dnq_lcd_room_item_update(room_id, ROOM_ITEM_SN_STATUS, status, color);
    return ret;
}

S32 dnq_lcd_room_temp_correct_update(U32 room_id, S32 correct, U32 color)
{
    S32 ret = 0;
    char buf[16] = {0};
    sprintf(buf, "%d", correct);
    ret = dnq_lcd_room_item_update(room_id, ROOM_ITEM_TEMP_CORRECT, buf, color);
    return ret;
}

S32 dnq_lcd_room_select_flag_update(U32 room_id, U8 *select_flag)
{
    S32 ret = 0;
    ret = dnq_lcd_room_item_update(room_id, ROOM_ITEM_SELECT_FLAG, select_flag, DEFAULT_COLOR);
    return ret;
}

S32 dnq_lcd_room_update(U32 room_id, room_item_t *room, U32 color)
{
    S32 ret = 0;

    ret |= dnq_lcd_room_id_update(room_id, room->id, color);
    ret |= dnq_lcd_room_name_update(room_id, room->name, color);
    ret |= dnq_lcd_room_current_temp_update(room_id, room->curr_temp, color);
    ret |= dnq_lcd_room_setting_temp_update(room_id, room->set_temp, color);
    ret |= dnq_lcd_room_work_status_update(room_id, room->status, color);
    ret |= dnq_lcd_room_sn_status_update(room_id, room->sn, color);
    ret |= dnq_lcd_room_temp_correct_update(room_id, room->correct, color);

    return ret;
}

S32 dnq_lcd_net_info_update(U8 *string)
{
    S32 ret = 0;
    ret = dnq_lcd_item_update(ITEM_ID_NET_INFO, string, DEFAULT_COLOR);
    return ret;
}

S32 dnq_lcd_mac_info_update(U8 *string)
{
    S32 ret = 0;
    ret = dnq_lcd_item_update(ITEM_ID_MAC_INFO, string, DEFAULT_COLOR);
    return ret;
}

S32 dnq_lcd_cmd_info_update(U8 *string)
{
    S32 ret = 0;
    ret = dnq_lcd_item_update(ITEM_ID_CMD_INFO, string, DEFAULT_COLOR);
    return ret;
}

S32 dnq_lcd_sys_info_update(U8 *string)
{
    S32 ret = 0;
    ret = dnq_lcd_item_update(ITEM_ID_SYS_INFO, string, DEFAULT_COLOR);
    return ret;
}

S32 dnq_lcd_help_info_update(U8 *string)
{
    S32 ret = 0;
    ret = dnq_lcd_item_update(ITEM_ID_HELP_INFO, string, DEFAULT_COLOR);
    return ret;
}

S32 dnq_lcd_clear_all()
{
    U32 i = 0, j = 0;
    U32 ret = 0;
    ret = dnq_lcd_item_clear(ITEM_ID_TITLE);
    ret = dnq_lcd_item_clear(ITEM_ID_DATE);
    ret = dnq_lcd_item_clear(ITEM_ID_HEADER);
    
    for(i=0; i<ROOM_CNT_PER_PAGE; i++)
    {
        for(j=0; j<ONE_ROOM_ITEM_CNT; j++)
        {
            ret = dnq_lcd_item_clear(LCD_ID_ROOM_ITEM_START+i*ONE_ROOM_ITEM_CNT+j);
        }
    }
    ret = dnq_lcd_item_clear(ITEM_ID_MAC_INFO);
    ret = dnq_lcd_item_clear(ITEM_ID_NET_INFO);
    ret = dnq_lcd_item_clear(ITEM_ID_CMD_INFO);
    ret = dnq_lcd_item_clear(ITEM_ID_SYS_INFO);
    ret = dnq_lcd_item_clear(ITEM_ID_HELP_INFO);
    return ret;
}

S32 dnq_lcd_update_all()
{
    int i,j;
    U32 ret = 0;
    ret = dnq_lcd_title_update(TITLE_STR);
    
    ret = dnq_lcd_date_update("2017-04-23 15:35:55");
    ret = dnq_lcd_header_update(HEADER_STR);

    
    for(i=0; i<ROOM_CNT_PER_PAGE; i++)
    {
        j = 0;
        ret = dnq_lcd_room_id_update(i, g_items[i].id, DEFAULT_COLOR);
        ret = dnq_lcd_room_name_update(i, g_items[i].name, DEFAULT_COLOR);
        ret = dnq_lcd_room_current_temp_update(i, g_items[i].curr_temp, DEFAULT_COLOR);
        ret = dnq_lcd_room_setting_temp_update(i, g_items[i].set_temp, SECOND_COLOR);
        ret = dnq_lcd_room_work_status_update(i, g_items[i].status, SECOND_COLOR);
        ret = dnq_lcd_room_sn_status_update(i, g_items[i].sn, SECOND_COLOR);
        ret = dnq_lcd_room_temp_correct_update(i, g_items[i].correct, SECOND_COLOR);

        ret = dnq_lcd_room_select_flag_update(i, SETTING_FLAG);
        //ret = dnq_lcd_item_update(3+i*ONE_ROOM_ITEM_CNT, "test");
    }
    ret = dnq_lcd_room_select_flag_update(3, SELECT_FLAG);
    
    ret = dnq_lcd_mac_info_update(MAC_INFO_STR);
    ret = dnq_lcd_net_info_update(NET_INFO_STR);
    ret = dnq_lcd_cmd_info_update(CMD_INFO_STR);
    ret = dnq_lcd_sys_info_update(SYS_INFO_STR);
    ret = dnq_lcd_help_info_update(HELP_INFO_STR);

    return ret;
}



void *lcd_task(void *args)
{
    dnq_task_t *lcd_task;
    dnq_msg_t  sendMsg;
    dnq_msg_t  recvMsg;
    dnq_msg_t *pSendMsg = &sendMsg;
    dnq_msg_t *pRecvMsg = &recvMsg;

    lcd_task = (dnq_task_t*)args;
    while(1)
    {
        dnq_msg_recv_timeout(NULL, pRecvMsg, 400);

        switch(pRecvMsg->type)
        {
            case 1:
                break;
        }
    }

    dnq_free(lcd_task);
}

S32 dnq_lcd_init()
{
    dnq_task_t *lcd_task;
    
    lcd_task = dnq_os_task_create("lcd", 16*2048, lcd_task, (void*)lcd_task);
    if(lcd_task == NULL)
    {
        DNQ_ERROR(DNQ_MOD_LCD, "lcd task create error: %s", strerror(errno));
        return -1;
    }

    return 0;
}

int lcd_test()
{
    dnq_init();

    dnq_debug_setlever(1, 3);
    dnq_uart_init();
    dnq_lcd_items_init();

    while(1)
    {
        dnq_lcd_clear_all();
        printf("--clear!\n");
        sleep(1);
        dnq_lcd_update_all();
        printf("--update!\n");
        sleep(2);

    }
    
    dnq_uart_deinit();
    dnq_deinit();
}


#ifdef uart
int test_lcd(int argc, char **argv)
{
    int i = 0, j = 0 ,k = 0;
	int flag = 0;
	int len = 0;
    int nRet = 0;
	char sbuffer[24][SIZE] = {0};
    char sbuffer1[SIZE] = {0xA5, 0x5A, 0x03, 0x82, 0x00, 0x00};
	char sbuffer2[SIZE] = {0xA5, 0x5A, 0x03, 0x82, 0x00, 0x20};
	char sbuffer3[SIZE] = {0xA5, 0x5A, 0x03, 0x82, 0x00, 0x40};
	char sbuffer4[SIZE] = {0xA5, 0x5A, 0x03, 0x82, 0x00, 0x60};
	char sbuffer5[SIZE] = {0xA5, 0x5A, 0x03, 0x82, 0x00, 0x80};
	char sbuffer6[SIZE] = {0xA5, 0x5A, 0x03, 0x82, 0x00, 0xA0};
    char rbuffer[SIZE] = {0x00};
    char temp_buffer[SIZE] = {0x00};
    char item_buffer[SIZE] = {0x00};
    
    if( SerialInit() == -1 )
    {
        perror("SerialInit Error!\n");
        return -1;
    }
    
	if(argc > 1)
		flag = 1;
    memset(rbuffer, 0, SIZE);
	memset(sbuffer, 0, sizeof(sbuffer));
	printf("sbuffer sizeof == %d\n", sizeof(sbuffer));
	printf("build date: %s %s\n", __DATE__, __TIME__);

	//strcpy(&sbuffer1[6], "松花江小学-主楼-三楼西/一号箱-2017年5月15日 18:88:88");
	strcpy(&sbuffer1[6], " 松花江小学-主楼-三楼西/一号箱  2017年5月15日 18:33:33");
	strcpy(&sbuffer2[6], "序号     房间       室温    设置温度   状态     SN    温度校准 ");
	strcpy(&sbuffer3[6], " 1     三年二班     22.1度     32度     工作     正常     -42 ");
	strcpy(&sbuffer4[6], " 2     四年六班     12.7度     19度     工作     正常     -2  ");
	strcpy(&sbuffer5[6], " 网络状态：    正常     MAC：20-21-3E-43-FE-47-29     ↑:上一页 ↓:下一页  ");
	strcpy(&sbuffer6[6], " 当前执行命令：加热     火娃电采暖智能控制器                  ");

	for(i=0; i<22; i++)
	{
		sbuffer[i][0] = 0xA5;
		sbuffer[i][1] = 0x5A;
		sbuffer[i][2] = 0x00;
		sbuffer[i][3] = 0x82;

		len = 0x80 * i;
		sbuffer[i][4] = (len >> 8) & 0xFF;
		sbuffer[i][5] = len & 0xFF;
	}

    i=0;
	printf("sbuffer1  len  ===%d===\n", strlen(&sbuffer1[6]));
	/* 标题 */
	strcpy(&sbuffer[i][6], &sbuffer1[6]);
    sbuffer[i][2] = strlen((char*)&sbuffer[i][6])+3;
    write_data(sbuffer[i],sbuffer[i][2] + 3);
    i++;
    
	/* 功能名称 */
	strcpy(&sbuffer[1][6], &sbuffer2[6]);
    sbuffer[i][2] = strlen((char*)&sbuffer[i][6])+3;
    write_data(sbuffer[i],sbuffer[i][2] + 3);
    i++;
    
    /* 初始化帧头 */
    for(j = 0; j<7; j++)
    {
        sbuffer[j][0] = 0xA5;
        sbuffer[j][1] = 0x5A;
        sbuffer[j][2] = 0x00;
        sbuffer[j][3] = 0x82;
    }
    
	/* 条目buff初始化 */
	for(i=0; i<12; i++)
	{
        /* 条目序号 */
        j = 0;
        len = i*0x100 + j*0x20 + 0x100;
        sbuffer[j][4] = (len >> 8) & 0xFF;
        sbuffer[j][5] = len & 0xFF;
        g_items[i].id = i;
        sprintf(temp_buffer, "%02d" , g_items[i].id);
        strcpy(&sbuffer[j][6], temp_buffer);
        sbuffer[j][2] = strlen(temp_buffer) + 3;
        j++;
        
        /* 房间名称 */
        len = i*0x100 + j*0x20 + 0x100;
        sbuffer[j][4] = (len >> 8) & 0xFF;
        sbuffer[j][5] = len & 0xFF;
        strcpy(&sbuffer[j][6], g_items[i].name);
        printf("[%d]:name=%s, len=%d\n", i, g_items[i].name, strlen(g_items[i].name));
        sbuffer[j][2] = strlen(&sbuffer[j][6]) + 3;
        j++;
        
        /* 室温 */
        len = i*0x100 + j*0x20 + 0x100;
        sbuffer[j][4] = (len >> 8) & 0xFF;
        sbuffer[j][5] = len & 0xFF;
        sprintf(temp_buffer, "%2.1f'C" , g_items[i].curr_temp);
        strcpy(&sbuffer[j][6], temp_buffer);
        sbuffer[j][2] = strlen(temp_buffer) + 3;
        j++;
        
        /* 设置温度 */
        len = i*0x100 + j*0x20 + 0x100;
        sbuffer[j][4] = (len >> 8) & 0xFF;
        sbuffer[j][5] = len & 0xFF;
        sprintf(temp_buffer, "%2.1f'C" , g_items[i].set_temp);
        strcpy(&sbuffer[j][6], temp_buffer);
        sbuffer[j][2] = strlen(temp_buffer) + 3;
        j++;
        
        /* 状态 */
        len = i*0x100 + j*0x20 + 0x100;
        sbuffer[j][4] = (len >> 8) & 0xFF;
        sbuffer[j][5] = len & 0xFF;
        strcpy(&sbuffer[j][6], g_items[i].status);
        sbuffer[j][2] = strlen(&sbuffer[j][6]) + 3;
        j++;
        
        /* SN */
        len = i*0x100 + j*0x20 + 0x100;
        sbuffer[j][4] = (len >> 8) & 0xFF;
        sbuffer[j][5] = len & 0xFF;
        strcpy(&sbuffer[j][6], g_items[i].sn);
        sbuffer[j][2] = strlen(&sbuffer[j][6]) + 3;
        j++;
        
        /* 温度校准 */
        len = i*0x100 + j*0x20 + 0x100;
        sbuffer[j][4] = (len >> 8) & 0xFF;
        sbuffer[j][5] = len & 0xFF;
        sprintf(temp_buffer, "%d" , g_items[i].offset);
        strcpy(&sbuffer[j][6], temp_buffer);
        sbuffer[j][2] = strlen(temp_buffer) + 3;
        j++;
        for(k=0;k<7;k++)
        {           
            write_data(sbuffer[k],sbuffer[k][2] + 3);
        }       
	}
    
    /* 网络状态，命令动作 */
    i = 7;
    sbuffer[i][4] = 0x0D;
    sbuffer[i][5] = 0x00;
	strcpy(&sbuffer[i][6], &sbuffer5[6]);
    sbuffer[i][2] = strlen(&sbuffer6[6]) + 3;
    write_data(sbuffer[i],sbuffer[i][2] + 3);
    
	/* 系统信息 */
    i++;
    sbuffer[i][4] = 0x0E;
    sbuffer[i][5] = 0x00;
	strcpy(&sbuffer[i][6], &sbuffer6[6]);
    sbuffer[i][2] = strlen(&sbuffer6[6]) + 3;
    write_data(sbuffer[i],sbuffer[i][2] + 3);

    
    #if 0
    if(flag)
    {
    i = 1;
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    }
    else 
    {
    i = 1;
    strcpy(&sbuffer[i++][6],"序号     房间      室温    设置温度   状态    SN   温度校准 ");
    strcpy(&sbuffer[i++][6]," 01    三年二班    22.1℃      32℃     工作   正常    -2 ");
    strcpy(&sbuffer[i++][6]," 02     门卫室     24.5℃      26℃     工作   正常    -10 ");
    strcpy(&sbuffer[i++][6]," 03   走廊蓄热12    11.2℃       5℃      停止   正常   -2 ");
    strcpy(&sbuffer[i++][6]," 04   走廊蓄热2    10.2℃       5℃     停止   正常    12 ");
    strcpy(&sbuffer[i++][6]," 05   科学实验室   22.1℃      23℃     停止   正常    -2 ");
    strcpy(&sbuffer[i++][6]," 06  一年级教研室   19.1℃      23℃     工作   正常    -2 ");
    strcpy(&sbuffer[i++][6]," 07    三年一班    18.1℃      19℃     停止   正常    -2 ");
    strcpy(&sbuffer[i++][6]," 08     水房      5.1℃       0℃     工作   正常    -2 ");
    strcpy(&sbuffer[i++][6]," 09   闲置房间1-2   4.9℃       5℃     停止   正常    -2 ");
    strcpy(&sbuffer[i++][6]," 10    会议室南    5.9℃       5℃     停止   正常    -2 ");
    strcpy(&sbuffer[i++][6]," 11    会议室北    5.2℃       5℃     停止   正常     1 ");
    strcpy(&sbuffer[i++][6]," 12    楼梯间2-3    13.2℃      15℃     停止   正常       ");
}
#endif

    i = 0;
    strcpy(&sbuffer[i][6], "工作");
    sbuffer[i][2] = strlen(&sbuffer[i][6]) + 3;
    sbuffer[i][4] = 0x04;
    sbuffer[i][5] = 0x80;
    
    i++;
    strcpy(&sbuffer[i][6], "停止");
    sbuffer[i][2] = strlen(&sbuffer[i][6]) + 3;
    sbuffer[i][4] = 0x04;
    sbuffer[i][5] = 0xE0;
    
    i++;
    strcpy(&sbuffer[i][6], "    ");
    sbuffer[i][2] = strlen(&sbuffer[i][6]) + 3;
    sbuffer[i][4] = 0x04;
    sbuffer[i][5] = 0x80;
    
    i++;
    strcpy(&sbuffer[i][6], "    ");
    sbuffer[i][2] = strlen(&sbuffer[i][6]) + 3;
    sbuffer[i][4] = 0x04;
    sbuffer[i][5] = 0xE0;
    
    i = 20;
    while(i--)
    {
        write_data(sbuffer[2],sbuffer[2][2] + 3);
        write_data(sbuffer[3],sbuffer[3][2] + 3);
        write_data(sbuffer[0],sbuffer[0][2] + 3);
        sleep(1);
        write_data(sbuffer[2],sbuffer[3][2] + 3);
        write_data(sbuffer[3],sbuffer[3][2] + 3);
        write_data(sbuffer[1],sbuffer[1][2] + 3);
        sleep(1);
    }
    
    close(nFd);
    return 0;
    
    while(1)
    {
		#if 0  //松花江小学
		len = sbuffer1[2] = strlen(&sbuffer1[6]);
        nRet = write(nFd, sbuffer1, len+6);
		len = sbuffer2[2] = strlen(&sbuffer2[6]);
        nRet = write(nFd, sbuffer2, len+6);
		len = sbuffer3[2] = strlen(&sbuffer3[6]);
        nRet = write(nFd, sbuffer3, len+6);
		len = sbuffer4[2] = strlen(&sbuffer4[6]);
        nRet = write(nFd, sbuffer4, len+6);
		len = sbuffer5[2] = strlen(&sbuffer5[6]);
        nRet = write(nFd, sbuffer5, len+6);
		len = sbuffer6[2] = strlen(&sbuffer6[6]);
        nRet = write(nFd, sbuffer6, len+6);
		#endif

		for(i=0; i<16; i++)
		{
			sbuffer[i][2] = strlen((char*)&sbuffer[i][6])+3;
			len = sbuffer[i][2];
			printf("send data: len=%d\n", len+3);
			if(flag)
			{
				for(j=0; j<len+6; j++)
					printf("%02x ", sbuffer[i][j]);
				printf("\n");
			}
			nRet = write(nFd, sbuffer[i], len+3);
		}

        if(-1 == nRet)
        {
            perror("send Data Error!\n");
            break;
        }
        if(0 < nRet)
        {
            sbuffer1[nRet] = 0;
            printf("send Data: len=%d\n", nRet);
        }
		break;
#if 0
	usleep(200*1000);
        memset(rbuffer, 0, SIZE);
        nRet = read(nFd, rbuffer, sizeof(rbuffer));
        if(-1 == nRet)
        {
            perror("recv Data Error!\n");
            break;
        }
        if(0 < nRet)
        {
            rbuffer[nRet] = 0;
            printf("recv Data: ");
            for(i=0; i<nRet; i++)
            {
                printf("0x%02x ", rbuffer[i]);
            }
            printf(", len=%d\n", nRet);
            break;
        }

#endif
    }


    close(nFd);
    return 0;
}
#endif
