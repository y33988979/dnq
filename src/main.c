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
#include "dnq_network.h"
#include "dnq_uart.h"
#include "dnq_log.h"
#include "dnq_mcu.h"
#include "dnq_sensor.h"
#include "dnq_lcd.h"
#include "dnq_os.h"

#define MAIN_CHECK(func) \
    do\
    {\
        if(func < 0)\
        {\
            DNQ_ERROR(DNQ_MOD_ALL, "main check error!");\
            return -1;\
        }\
    }while(0);


void *func(void *args)
{
    while(1)   sleep(3);
}

void dnq_print_version()
{
    printf("|--------DNQ VERSION %s at %s --------|\n",__DATE__,__TIME__);
    printf("|--------DNQ HWVER=0x%x  --------|\n", HWVER);
    printf("|--------DNQ SWVER=0x%x  --------|\n", SWVER);
}

int main()
{
    S32 len;
    U8 buffer[1024];
    dnq_queue_t *queue = NULL;
    dnq_msg_t recv_msg;
    sem_t sem1 = {0};
   
    dnq_print_version();
       
    //dnq_debug_setlever(1,5); 
    //dnq_debug_setlever(DNQ_MOD_CONFIG, 5);
    dnq_debug_setlever(DNQ_MOD_RABBITMQ, 5);

    MAIN_CHECK( dnq_init() );
    MAIN_CHECK( dnq_debug_init() );
    MAIN_CHECK( dnq_uart_init() );
    MAIN_CHECK( dnq_mcu_init() );
    MAIN_CHECK( dnq_config_init() );  
    MAIN_CHECK( dnq_network_init() );
    MAIN_CHECK( dnq_lcd_init() );
    MAIN_CHECK( dnq_sensor_init() );
    MAIN_CHECK( dnq_keypad_init() );
    MAIN_CHECK( dnq_manage_init() );
    MAIN_CHECK( dnq_rabbitmq_init());

    printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
    //dnq_task_create("main", 32*2048, func, (void*)queue);

    while(1)
    {
        sleep(3);
    }
    
exit:
    dnq_keypad_deinit();
    dnq_lcd_deinit();
    dnq_mcu_deinit();
    dnq_uart_deinit();
    dnq_deinit();

    return 0;
}

