#ifndef _DNQ_LCD_H_
#define _DNQ_LCD_H_

#include "common.h"

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

typedef struct _item
{
    int      id;
    char     name[16];
    float    curr_temp;
    float    set_temp;
    char     status[16];
    char     sn[16];
    int      offset;
    char     command[64];
}house_item_t;



typedef struct lcd_item
{
    U8    id;
    U8    size;
    U16   addr;
    
}lcd_item_t;


#endif /* _DNQ_LCD_H_ */

