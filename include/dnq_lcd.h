#ifndef _DNQ_LCD_H_
#define _DNQ_LCD_H_

#include "common.h"

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

/* addr size */
#define LCD_TITLE_SIZE        0x40
#define LCD_ITEM_SIZE         0x10
#define LCD_ITEM_NAME_SIZE    0x20

/* room item address  */
#define LCD_ROOM_START_ADDR   0x100
#define LCD_ROOM_SIZE         0x100
#define LCD_ROOM_END_ADDR     0x900

#define ROOM_CNT_PER_PAGE     8
#define ONE_ROOM_ITEM_CNT     12
#define ALL_ROOM_ITEM_CNT     (ONE_ROOM_ITEM_CNT*ROOM_CNT_PER_PAGE)

#define LCD_ID_ROOM_ITEM_START    3
#define LCD_ID_ROOM_ITEM_END     (LCD_ID_ROOM_ITEM_START + ALL_ROOM_ITEM_CNT -1)
#define LCD_ITEM_MAX          188

/* font color, support two colors */
#define DEFAULT_COLOR         0
#define SECOND_COLOR          1

/* show flag for setting params by keypad */
#define SELECT_FLAG           "-->"
#define SETTING_FLAG          "↑↓"

/* lcd status */
#define LCD_STATUS_SHOWING    0
#define LCD_STATUS_SETTING    1

/* some string  */
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

typedef struct lcd_table_item
{
    U8     id;
    U32    addr;
    U8     content[40];
    U8     color;
}lcd_table_item_t;

typedef struct lcd_title
{
    U8     id;
    U32    addr;
    U8     content[128];
    U8     color;
}lcd_title_t;

typedef struct lcd_table_header
{
    U8     id;
    U32    addr;
    U8     content[128];
    U8     color;
}lcd_table_header_t;


typedef struct lcd_desc
{
    lcd_title_t          title;
    lcd_table_header_t   table_header[12];
    lcd_table_item_t     table_items[128];
    U8                   mac_addr[64];
    U8                   network_status[64];
    U8                   current_cmd[64];
    U8                   system_name[64];
    
}lcd_desc_t;

typedef struct room_item
{
    int      id;
    char     name[16];
    float    curr_temp;
    float    set_temp;
    char     status[16];
    char     sn[16];
    int      correct;
}room_item_t;

typedef struct lcd_item
{
    U8    id;
    U8    size;
    U16   addr;
    U16   addr2;
    U8    content[64];
    
}lcd_item_t;


#define FOUCS_SETTING_TEMP   0
#define FOUCS_CORRECT_TEMP   0
#define FOUCS_SELECT_FLAG    0

typedef struct lcd_status
{
    U32   status;
    U32   current_page;
    U32   current_room;
    U32   current_foucs;
}lcd_status_t;

S32 dnq_lcd_init();
S32 dnq_lcd_deinit();


#endif /* _DNQ_LCD_H_ */

