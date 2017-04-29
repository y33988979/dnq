/* dnq main Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  application for nuc970.
 * Note : 
 */


#include "dnq_common.h"
#include "dnq_config.h"
#include "ngx_palloc.h"
#include "dnq_keypad.h"
#include "dnq_rabbitmq.h"
#include "dnq_uart.h"
#include "dnq_log.h"
#include "dnq_mcu.h"
#include "dnq_lcd.h"


int main()
{
    dnq_init();

    dnq_debug_setlever(1, 3);
    dnq_uart_init();
    dnq_lcd_items_init();

    while(1)
    {
        //event_input();
        //event_proc();
        
        
    }
    
    dnq_uart_deinit();
    dnq_deinit();
    return 0;
}

