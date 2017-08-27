#ifndef _DNQ_LCD_H_
#define _DNQ_LCD_H_

#include "common.h"
#include "dnq_os.h"

#define ITEM_ADDR_OFFSET      0x20

/* All items's address define */ 

/* title, datetime, header */
#define ITEM_ADDR_TITLE       0x00
#define ITEM_ADDR_DATE        0x40
#define ITEM_ADDR_HEADER      0x80

/* mac, command, network, system info */
#define ITEM_ADDR_MAC_INFO    0x0A00
#define ITEM_ADDR_NET_INFO    0x0A80
//#define ITEM_ADDR_NET_STATUS  0x0A90
#define ITEM_ADDR_CMD_INFO    0x0B00
#define ITEM_ADDR_SYS_INFO    0x0B80
#define ITEM_ADDR_HELP_INFO   0x0C00

/* addr size */
#define LCD_TITLE_SIZE        0x20
#define ROOM_ITEM_SIZE        0x06
#define ROOM_ITEM_NAME_SIZE   0x20
#define ROOM_ITEM_ADDR_OFFSET 0x10

/* room item address  */
#define LCD_ROOM_START_ADDR   0x100
#define LCD_ROOM_SIZE         0x100
#define LCD_ROOM_END_ADDR     0x900

#define ROOM_CNT_PER_PAGE     8
#define ONE_ROOM_ITEM_CNT     12
#define ALL_ROOM_ITEM_CNT     (ONE_ROOM_ITEM_CNT*ROOM_CNT_PER_PAGE)

#define LCD_ID_ROOM_ITEM_START    3
#define LCD_ID_ROOM_ITEM_END     (LCD_ID_ROOM_ITEM_START + ALL_ROOM_ITEM_CNT -1)
#define LCD_ITEM_MAX          218

/* used for key process, roomid: 0~(ROOM_CNT_PER_PAGE-1) */
#define FIRST_ROOM_IN_PAGE     0
#define LAST_ROOM_IN_PAGE      (ROOM_CNT_PER_PAGE-1)

/* font color, support two colors */
#define DEFAULT_COLOR         0
#define FOUCS_COLOR           1

/* show flag for setting params by keypad */
#define SELECT_FLAG           "-->"
#define SETTING_FLAG          "↑↓"
#define HIDE_FLAG             "     "

#define STATUS_STR_WORK       "工作"
#define STATUS_STR_STOP       "停止"

#define SN_STATUS_STR_WORK    "正常"
#define SN_STATUS_STR_STOP    "异常"

/* lcd status */
#define LCD_STATUS_SHOWING    0
#define LCD_STATUS_SETTING    1

/* some string  */
#define SOME_SPACE            "                                         "
#define TITLE_STR " 松花江小学-主楼-二楼东/三号箱 "
#define HEADER_STR " 序号     房间       室温   设置温度    状态     SN    温度校准 "
#define DATE_STR "2017年5月15日 18:33:33"
     
#define MAC_INFO_STR "MAC：20-21-3E-43-FE-47-29 "
#define NET_INFO_STR "网络状态：   初始化.. "
#define CMD_INFO_STR "当前执行命令：  加热 "
#define SYS_INFO_STR "火娃电采暖智能控制器 "
#define HELP_INFO_STR "↑:上一页  ↓:下一页  OK:设置  EXIT:返回"

#define WORK_STATUS      0x1
#define STOP_STATUS      0x0

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
    ITEM_ID_HELP_INFO, //102
}lcd_item_id_e;

typedef enum room_item_id
{
    ROOM_ITEM_ID,
    ROOM_ITEM_NAME,
    ROOM_ITEM_CURRENT_TEMP,
    ROOM_ITEM_SET_TEMP,
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
    U32      id;
    U8       name[SIZE_16];
    S32      curr_temp;
    S32      set_temp;
    U16      work_status;
    U16      sn_status;
    S32      correct;
}room_item_t;

#define ITEM_CONTENT_SIZE 64
typedef struct lcd_item
{
    U8    id;
    U8    size;
    U16   addr;
    U16   addr2;
    U16   color;
    U8    content[ITEM_CONTENT_SIZE];
    
}lcd_item_t;

#define FOUCS_ITEM_SELECT_FLAG    0
#define FOUCS_ITEM_SETTING_TEMP   1
#define FOUCS_ITEM_CORRECT_TEMP   2
#define FOUCS_ITEM_MAX            3
typedef struct _lcd_status
{
    U32   status;
    U32   around_enable;
    U32   room_count;
    U32   page_count;
    U32   current_page;
    U32   current_room;
    U32   current_foucs;
}lcd_status_t;

typedef enum net_msg_type
{
    NET_MSG_TYPE_NET_STATUS_CHANGE = 0x10,
    NET_MSG_TYPE_MAC_INFO_UPDATE
}net_msg_type_e;

typedef enum rabbitmq_msg_type
{
    MQ_MSG_TYPE_SET_TEMP_UPDATE = 0x10,
    MQ_MSG_TYPE_TEMP_CORRECT_UPDATE,

}rabbitmq_msg_type_e;

extern room_item_t g_rooms[DNQ_ROOM_MAX+1];


S32 dnq_lcd_init();
S32 dnq_lcd_deinit();
S32 send_msg_to_lcd(dnq_msg_t *msg);
room_item_t *dnq_get_rooms();
room_item_t *dnq_get_room_item(U32 room_id);

#endif /* _DNQ_LCD_H_ */

