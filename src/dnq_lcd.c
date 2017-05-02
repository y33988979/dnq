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
#include "dnq_os.h"
#include "dnq_keypad.h"

#define SIZE   1024

static U8   uart_command[256];
static U32  lcd_current_page = 0;
static lcd_status_t  lcd_status = {LCD_STATUS_SHOWING, 0, 0, 0};
static dnq_appinfo_t  *lcd_appinfo;

room_item_t g_rooms[DNQ_ROOM_MAX+1] = 
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
    {12, "楼梯间2-3", 13.2,15,"正常","正常",-2},
    {13, "楼梯间2-3", 13.2,15,"正常","正常",-2},
    {14, "楼梯间2-3", 13.2,15,"正常","正常",-2},
    {15, "楼梯间2-3", 13.2,15,"正常","正常",-2},
    {0,0,0,0,0,0,0},
   
};

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

static U8 *lcd_get_item_content(U32 item_id)
{
    return (U8*)g_lcd_items[item_id].content;
}

static U32 lcd_get_item_color(U32 item_id)
{
    return g_lcd_items[item_id].color;
}

static U32 lcd_get_operate_status()
{
    return lcd_status.status;
}

static U32 lcd_set_operate_status(U32 status)
{
    lcd_status.status = status;
}

static U32 lcd_get_current_page()
{
    return lcd_status.current_page;
}

static U32 lcd_set_current_page(U32 current_page)
{
    lcd_status.current_page = current_page;
}

static U32 lcd_get_current_room()
{
    return lcd_status.current_room;
}

static U32 lcd_set_current_room(U32 current_room)
{
    lcd_status.current_room = current_room;
}

static U32 lcd_get_current_foucs()
{
    return lcd_status.current_foucs;
}

static U32 lcd_set_current_foucs(U32 current_foucs)
{
    lcd_status.current_room = current_foucs;
}

S32 lcd_items_init()
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

#if 1
    DNQ_PRINT(DNQ_MOD_LCD, "items:\n id    addr\n");
    for(j=0; j<i; j++)
        DNQ_PRINT(DNQ_MOD_LCD, "%02d    0x%04x\n",items[j].id, items[j].addr);
#endif
    DNQ_INFO(DNQ_MOD_LCD, "all item init ok!");
    return 0;
}

S32 lcd_uart_cmd_prepare(U32 item_id, char *content, U32 color)
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
    
    g_lcd_items[item_id].color = color;
    
    //printf("g_lcd_items[%d].addr == 0x%04x\n",item_id, addr );
    
    cmd[4] = (addr >> 8) & 0xFF;
    cmd[5] = addr & 0xFF;

    len = strlen(content);
    if(len == 0) /* not update content */
        content = lcd_get_item_content(item_id);
    strncpy(g_lcd_items[item_id].content, content, (len>64)?64:len);
    cmd[2] = len + 3;
    strcpy(&cmd[6], content);
    
    return (len+6);
    
}

U32 room_in_current_page(U32 room_id)
{
    if((room_id/ROOM_CNT_PER_PAGE) == lcd_current_page)
        return 1;
    return 0;
}

S32 lcd_item_clear(U32 item_id)
{
    U32 len = 0;
    U8  some_blank[256] = {0};

    memset(some_blank, ' ', g_lcd_items[item_id].size);
    len = lcd_uart_cmd_prepare(item_id, some_blank, DEFAULT_COLOR);
    len = dnq_lcd_uart_write(uart_command, len);
    return len;
}

S32 lcd_item_draw(U32 item_id, U8 *content, U32 color)
{
    U32 len = 0;
    len |= lcd_uart_cmd_prepare(item_id, content, color);
    len |= dnq_lcd_uart_write(uart_command, len);
    return len;
}

S32 lcd_item_update(U32 item_id, U8 *content, U32 color)
{
    U32 ret = 0;
    /* update lcd */
    ret |= lcd_item_clear(item_id);
    ret |= lcd_item_draw(item_id, content, color);
    return ret;
}

S32 lcd_item_update_color(U32 item_id, U32 color)
{
    U32 ret = 0;
    ret |= lcd_item_clear(item_id);
    ret |= lcd_item_draw(item_id, "\0", color);
    return ret;
}

S32 lcd_title_update(U8 *content)
{
    U32 ret = 0;
    ret |= lcd_item_clear(ITEM_ID_TITLE);
    ret |= lcd_item_draw(ITEM_ID_TITLE, content, DEFAULT_COLOR);
    return ret;
}

S32 lcd_date_update(U8 *content)
{
    U32 ret = 0;
    ret |= lcd_item_clear(ITEM_ID_DATE);
    ret |= lcd_item_draw(ITEM_ID_DATE, content, DEFAULT_COLOR);
    return ret;
}

S32 lcd_header_update(U8 *content)
{
    U32 ret = 0;
    ret |= lcd_item_clear(ITEM_ID_HEADER);
    ret |= lcd_item_draw(ITEM_ID_HEADER, content, DEFAULT_COLOR);
    return ret;
}

S32 lcd_room_item_update(U32 room_id, U32 idx, U8 *content, U32 color)
{
    S32 ret = 0;
    U32 item_id = 0;
    
    if(!content)
    {
        DNQ_ERROR(DNQ_MOD_LCD, "content == NULL!!");
        return -1;
    }
    
    if(!room_in_current_page(room_id))
        return 0;

    if(idx > ONE_ROOM_ITEM_CNT)
        DNQ_ERROR(DNQ_MOD_LCD, "room's index[%d] error! it must less then %d",\
        idx, ONE_ROOM_ITEM_CNT);

    item_id = LCD_ID_ROOM_ITEM_START+room_id*ONE_ROOM_ITEM_CNT + idx;
    
    ret |= lcd_item_clear(item_id);
    ret |= lcd_item_draw(item_id, content, color);
    return ret;
}

/* update color only, not content  */
S32 lcd_room_update_item_color(U32 room_id, U32 item_id, U32 color)
{
    return lcd_room_item_update(room_id, item_id, "\0", color);
}

S32 lcd_room_id_update(U32 room_id, U32 id, U32 color)
{
    S32 ret = 0;
    char buf[16] = {0};
    
    g_rooms[room_id].id = id;
    sprintf(buf, "%02d", id);
    ret = lcd_room_item_update(room_id, ROOM_ITEM_ID, buf, color);
    return ret;
}

S32 lcd_room_name_update(U32 room_id, U8 *room_name, U32 color)
{
    S32 ret = 0;
   
    strncpy(g_rooms[room_id].name, room_name, 16);
    ret = lcd_room_item_update(room_id, ROOM_ITEM_NAME, room_name, color);
    return ret;
}

S32 lcd_room_current_temp_update(U32 room_id, float degree, U32 color)
{
    S32 ret = 0;
    char buf[16] = {0};

    g_rooms[room_id].curr_temp = degree;
    sprintf(buf, "%2.1f", degree);
    ret = lcd_room_item_update(room_id, ROOM_ITEM_CURRENT_TEMP, buf, color);
    return ret;
}

S32 lcd_room_setting_temp_update(U32 room_id, float degree, U32 color)
{
    S32 ret = 0;
    char buf[16] = {0};

    g_rooms[room_id].set_temp= degree;
    sprintf(buf, "%2.1f", degree);
    ret = lcd_room_item_update(room_id, ROOM_ITEM_SETTING_TEMP, buf, color);
    return ret;
}

S32 lcd_room_work_status_update(U32 room_id, U8 *status, U32 color)
{
    S32 ret = 0;

    strncpy(g_rooms[room_id].status, status, 16);
    ret = lcd_room_item_update(room_id, ROOM_ITEM_WORK_STATUS, status, color);
    return ret;
}

S32 lcd_room_sn_status_update(U32 room_id, U8 *status, U32 color)
{
    S32 ret = 0;
    
    strncpy(g_rooms[room_id].sn, status, 16);
    ret = lcd_room_item_update(room_id, ROOM_ITEM_SN_STATUS, status, color);
    return ret;
}

S32 lcd_room_temp_correct_update(U32 room_id, S32 correct, U32 color)
{
    S32 ret = 0;
    char buf[16] = {0};

    g_rooms[room_id].correct = correct;
    sprintf(buf, "%d", correct);
    ret = lcd_room_item_update(room_id, ROOM_ITEM_TEMP_CORRECT, buf, color);
    return ret;
}

S32 lcd_room_select_flag_update(U32 room_id, U8 *select_flag)
{
    S32 ret = 0;
    ret = lcd_room_item_update(room_id, ROOM_ITEM_SELECT_FLAG, select_flag, DEFAULT_COLOR);
    return ret;
}

S32 lcd_room_update(U32 room_id, room_item_t *room, U32 color)
{
    S32 ret = 0;

    ret |= lcd_room_id_update(room_id, room->id, color);
    ret |= lcd_room_name_update(room_id, room->name, color);
    ret |= lcd_room_current_temp_update(room_id, room->curr_temp, color);
    ret |= lcd_room_setting_temp_update(room_id, room->set_temp, color);
    ret |= lcd_room_work_status_update(room_id, room->status, color);
    ret |= lcd_room_sn_status_update(room_id, room->sn, color);
    ret |= lcd_room_temp_correct_update(room_id, room->correct, color);

    return ret;
}

S32 lcd_net_info_update(U8 *string)
{
    S32 ret = 0;
    ret = lcd_item_update(ITEM_ID_NET_INFO, string, DEFAULT_COLOR);
    return ret;
}

S32 lcd_mac_info_update(U8 *string)
{
    S32 ret = 0;
    ret = lcd_item_update(ITEM_ID_MAC_INFO, string, DEFAULT_COLOR);
    return ret;
}

S32 lcd_cmd_info_update(U8 *string)
{
    S32 ret = 0;
    ret = lcd_item_update(ITEM_ID_CMD_INFO, string, DEFAULT_COLOR);
    return ret;
}

S32 lcd_sys_info_update(U8 *string)
{
    S32 ret = 0;
    ret = lcd_item_update(ITEM_ID_SYS_INFO, string, DEFAULT_COLOR);
    return ret;
}

S32 lcd_help_info_update(U8 *string)
{
    S32 ret = 0;
    ret = lcd_item_update(ITEM_ID_HELP_INFO, string, DEFAULT_COLOR);
    return ret;
}

S32 lcd_rooms_update_by_page(U32 page_num)
{
    S32 ret;
    U32 i, j;
    U32 room_offset = 0;
    U32 color;
    
    room_offset = ROOM_CNT_PER_PAGE*page_num;
    
    for(i=LCD_ID_ROOM_ITEM_START; i<LCD_ID_ROOM_ITEM_END; i++)
    {
        color = g_lcd_items[i].color;

        ret = lcd_item_update_color(i, color);
        #if 0
        ret = lcd_room_id_update(i, g_rooms[room_offset+i].id, DEFAULT_COLOR);
        ret = lcd_room_name_update(i, g_rooms[room_offset+i].name, DEFAULT_COLOR);
        ret = lcd_room_current_temp_update(i, g_rooms[room_offset+i].curr_temp, DEFAULT_COLOR);
        ret = lcd_room_setting_temp_update(i, g_rooms[room_offset+i].set_temp, DEFAULT_COLOR);
        ret = lcd_room_work_status_update(i, g_rooms[room_offset+i].status, DEFAULT_COLOR);
        ret = lcd_room_sn_status_update(i, g_rooms[room_offset+i].sn, DEFAULT_COLOR);
        ret = lcd_room_temp_correct_update(i, g_rooms[room_offset+i].correct, DEFAULT_COLOR);
        #endif
    }
    return ret;
}

S32 lcd_clear_all()
{
    U32 i = 0, j = 0;
    U32 ret = 0;
    ret = lcd_item_clear(ITEM_ID_TITLE);
    ret = lcd_item_clear(ITEM_ID_DATE);
    ret = lcd_item_clear(ITEM_ID_HEADER);
    
    for(i=0; i<ROOM_CNT_PER_PAGE; i++)
    {
        for(j=0; j<ONE_ROOM_ITEM_CNT; j++)
        {
            ret = lcd_item_clear(LCD_ID_ROOM_ITEM_START+i*ONE_ROOM_ITEM_CNT+j);
        }
    }
    ret = lcd_item_clear(ITEM_ID_MAC_INFO);
    ret = lcd_item_clear(ITEM_ID_NET_INFO);
    ret = lcd_item_clear(ITEM_ID_CMD_INFO);
    ret = lcd_item_clear(ITEM_ID_SYS_INFO);
    ret = lcd_item_clear(ITEM_ID_HELP_INFO);
    return ret;
}

S32 lcd_update_all()
{
    int i,j;
    U32 ret = 0;
    ret = lcd_title_update(TITLE_STR);
    
    ret = lcd_date_update("2017-04-23 15:35:55");
    ret = lcd_header_update(HEADER_STR);
    
    for(i=0; i<ROOM_CNT_PER_PAGE; i++)
    {
        j = 0;
        ret = lcd_room_id_update(i, g_rooms[i].id, DEFAULT_COLOR);
        ret = lcd_room_name_update(i, g_rooms[i].name, DEFAULT_COLOR);
        ret = lcd_room_current_temp_update(i, g_rooms[i].curr_temp, DEFAULT_COLOR);
        ret = lcd_room_setting_temp_update(i, g_rooms[i].set_temp, FOUCS_COLOR);
        ret = lcd_room_work_status_update(i, g_rooms[i].status, FOUCS_COLOR);
        ret = lcd_room_sn_status_update(i, g_rooms[i].sn, FOUCS_COLOR);
        ret = lcd_room_temp_correct_update(i, g_rooms[i].correct, FOUCS_COLOR);

        ret = lcd_room_select_flag_update(i, SETTING_FLAG);
        //ret = lcd_item_update(3+i*ONE_ROOM_ITEM_CNT, "test");
    }
    ret = lcd_room_select_flag_update(3, SELECT_FLAG);
    
    ret = lcd_mac_info_update(MAC_INFO_STR);
    ret = lcd_net_info_update(NET_INFO_STR);
    ret = lcd_cmd_info_update(CMD_INFO_STR);
    ret = lcd_sys_info_update(SYS_INFO_STR);
    ret = lcd_help_info_update(HELP_INFO_STR);

    return ret;
}


S32 lcd_room_next_foucs(U32 current_foucs, U32 direction)
{
    U32 new_foucs = current_foucs;
    if(direction == DNQ_KEY_LEFT)
    {
        if(current_foucs == ROOM_ITEM_SELECT_FLAG)
            new_foucs = ROOM_ITEM_TEMP_CORRECT;
        else if(current_foucs == ROOM_ITEM_SETTING_TEMP)
            new_foucs = ROOM_ITEM_SELECT_FLAG;
        else if(current_foucs == ROOM_ITEM_TEMP_CORRECT)
            new_foucs = ROOM_ITEM_SETTING_TEMP;
    }
    else if(direction == DNQ_KEY_RIGHT)
    {
        if(current_foucs == ROOM_ITEM_SELECT_FLAG)
            new_foucs = ROOM_ITEM_SETTING_TEMP;
        else if(current_foucs == ROOM_ITEM_SETTING_TEMP)
            new_foucs = ROOM_ITEM_TEMP_CORRECT;
        else if(current_foucs == ROOM_ITEM_TEMP_CORRECT)
            new_foucs = ROOM_ITEM_SELECT_FLAG;
    }

    return new_foucs;
}

S32 lcd_showing_key_process(U32 key_code, U32 key_status)
{
    S32 ret;
    U32 i;
    U32 current_page = lcd_get_current_page();

    switch (key_code)
    {
        case DNQ_KEY_UP:
            /* page update */
            current_page = current_page?0:1;
            lcd_set_current_page(current_page);
            lcd_rooms_update_by_page(current_page);
            
            break;
        case DNQ_KEY_DOWN:
            /* page update */
            current_page = current_page?0:1;
            lcd_set_current_page(current_page);
            lcd_rooms_update_by_page(current_page);
        break;
        case DNQ_KEY_LEFT:
        break;
        case DNQ_KEY_RIGHT:
        break;
        case DNQ_KEY_MENU:
            /* entry to setting status */
            lcd_set_operate_status(LCD_STATUS_SETTING);
            lcd_set_current_room(0);
            lcd_set_current_foucs(ROOM_ITEM_SELECT_FLAG);

            /* flag icon show for setting status */
            lcd_room_select_flag_update(0, SELECT_FLAG);

        break;
        case DNQ_KEY_OK:
        break;
        case DNQ_KEY_SELF_CHECK:
            /* rs485 check, register sensor! */
        break;
        case DNQ_KEY_SCAN:
        break;
        case DNQ_KEY_EXIT:
        break;
    }
 
}

S32 lcd_setting_key_process(U32 key_code, U32 key_status)
{
    S32 ret;
    U32 current_room = lcd_get_current_room();
    U32 current_foucs = lcd_get_current_foucs();
    U32 next_room;
    U32 next_foucs;
    switch (key_code)
    {
        case DNQ_KEY_UP:
            
            break;
        case DNQ_KEY_DOWN:
            /* move foucs */
        break;
            
        case DNQ_KEY_LEFT:
            /* move foucs */
            next_foucs = lcd_room_next_foucs(current_foucs, DNQ_KEY_LEFT);
            /* update color */
            lcd_room_update_item_color(current_room, current_foucs, DEFAULT_COLOR);
            lcd_room_update_item_color(current_room, next_foucs, FOUCS_COLOR);
            
        break;
        case DNQ_KEY_RIGHT:
            /* move foucs */
            next_foucs = lcd_room_next_foucs(current_foucs, DNQ_KEY_RIGHT);
            /* update color */
            lcd_room_update_item_color(current_room, current_foucs, DEFAULT_COLOR);
            lcd_room_update_item_color(current_room, next_foucs, FOUCS_COLOR);
        break;
        case DNQ_KEY_MENU:
        break;
        case DNQ_KEY_EXIT:
            
            /* entry to showing status */
            lcd_set_operate_status(LCD_STATUS_SHOWING);
            
            lcd_room_update_item_color(current_room, current_foucs, DEFAULT_COLOR);
            lcd_room_select_flag_update(current_room, HIDE_FLAG);
        break;
    }

    return ret;
}

S32 lcd_key_process(U32 key_code, U32 key_status)
{

    U32   lcd_status = lcd_get_operate_status();
    
    /* lcd show state machine */
    switch (lcd_status)
    {
        case LCD_STATUS_SHOWING:

            lcd_showing_key_process(key_code, key_status);
            break;
            
        case LCD_STATUS_SETTING:

            lcd_setting_key_process(key_code, key_status);
            break;
        default:
            DNQ_ERROR(DNQ_MOD_LCD, "unkown lcd status!");
            break;
    }
    
    return 0;
}    

void *lcd_task(void *args)
{
    S32  ret;
    S32  status;
    dnq_queue_t *lcd_queue;
    dnq_msg_t  sendMsg;
    dnq_msg_t  recvMsg;
    dnq_msg_t *pSendMsg = &sendMsg;
    dnq_msg_t *pRecvMsg = &recvMsg;

    lcd_queue = (dnq_queue_t*)lcd_appinfo->queue;
    tt;
    while(1)
    {
    tt;
        ret = dnq_msg_recv_timeout(lcd_queue, pRecvMsg, 400);
        if(ret < 0)
        {
            continue;
        }
        tt;

        /*
        * 1. 接收云端消息，更新屏幕状态
        * 2. 接收键盘消息，进行查看及设置操作
        */
        
        switch(pRecvMsg->Class)
        {
            case MSG_CLASS_KEYPAD:
                DNQ_INFO(DNQ_MOD_KEYPAD, "recv keypad msg: val=%d, status=%d",\
                    pRecvMsg->code, pRecvMsg->payload);

                lcd_key_process(pRecvMsg->code, (U32)pRecvMsg->payload);
                
            break;
            case MSG_CLASS_RABBITMQ:
                DNQ_INFO(DNQ_MOD_KEYPAD, "recv rabbitmq msg: val=%d, status=%d",\
                    pRecvMsg->code, pRecvMsg->payload);
            break;
        }
    }

    dnq_free(lcd_task);
}

S32 send_msg_to_lcd(dnq_msg_t *msg)
{
    S32 ret;
    dnq_queue_t *queue = NULL;
        
    queue = lcd_appinfo->queue;
    ret = dnq_msg_send(queue, msg);

    return ret;
}

S32 dnq_lcd_init()
{
    dnq_appinfo_t *appinfo = NULL;

    lcd_items_init();
    lcd_clear_all();
    lcd_update_all();

    appinfo = dnq_app_task_create("lcd", 2048*16, \
        QUEUE_MSG_SIZE, QUEUE_SIZE_MAX, lcd_task, (void*)&appinfo);
    if(!appinfo)
    {
        DNQ_ERROR(DNQ_MOD_LCD, "dnq_app_task_create error!");
        return -1;
    }
    sleep(2);
    lcd_appinfo = appinfo;
    DNQ_INFO(DNQ_MOD_LCD, "lcd_init ok!");
    return 0;
}

S32 dnq_lcd_deinit()
{
    S32 ret;
    
    ret = dnq_app_task_exit(lcd_appinfo);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_KEYPAD, "lcd_deinit error!");
        return -1;
    }
    
    DNQ_INFO(DNQ_MOD_KEYPAD, "lcd_deinit ok!");
    return 0;
}

int dnq_lcd_test()
{
    dnq_init();

    dnq_debug_setlever(1, 3);
    dnq_uart_init();
    lcd_items_init();

    while(1)
    {
        lcd_clear_all();
        printf("--clear!\n");
        sleep(1);
        lcd_update_all();
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
