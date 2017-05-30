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


void *send_test(void *args)
{
    int n = 0;
    int cnt = 0;
    dnq_queue_t *queue;
    dnq_msg_t send_msg;

    queue = (dnq_queue_t *)args;

    while(1)
    {
        send_msg.Class = n+1;
        send_msg.code = n+2;
        send_msg.lenght = n+3;
        strcpy(send_msg.data, "this is test message!");

        dnq_msg_send(queue, &send_msg);

        printf("send msg!!\n");
        n += 100;
        dnq_msleep(100);
        if(cnt++ % 5 == 0)
            sleep(600);
    }
}

int main()
{
    S32 len;
    U8 buffer[1024];
    dnq_queue_t *queue = NULL;
    dnq_msg_t recv_msg;

    int cnt = 0;
    while(1)
    {
        break;
        sleep(1);
        //printf("12312312312\n");
        if(cnt++ % 10 == 0)
        {
            dnq_system_call("cat /proc/uptime");
            printf("time_now=%d\n",dnq_time_now());
        }
    }

   
    MAIN_CHECK( dnq_init() );
    MAIN_CHECK( dnq_debug_init() );
    MAIN_CHECK( dnq_uart_init() );
    MAIN_CHECK( dnq_mcu_init() );
    MAIN_CHECK( dnq_network_init() );

    dnq_debug_setlever(1,3);
    
    //rs485_test();
    //sleep(1000);

    dnq_network_getinfo();
    //sleep(1000);

    MAIN_CHECK( dnq_lcd_init() );
    MAIN_CHECK( dnq_keypad_init() );
    MAIN_CHECK( dnq_rabbitmq_init());
    MAIN_CHECK( dnq_manage_init() );

    queue = dnq_queue_create(QUEUE_SIZE_MAX);
    if(queue == NULL)
        goto exit;

    dnq_task_create("send", 32*2048, send_test, (void*)queue);
    
    while(1)
    {
        //event_input();
        //event_proc();
        //sleep(11);
        if(dnq_msg_recv_timeout(queue, &recv_msg, 3000) < 0)
            continue;
        
        DNQ_PRINT(DNQ_MOD_ALL, "recv mgs:\n");
        DNQ_PRINT(DNQ_MOD_ALL,"type=%d, code=%d, len=%d, data=%s\n",\
            recv_msg.Class, recv_msg.code, recv_msg.lenght, recv_msg.data);

    }
    
exit:
    dnq_keypad_deinit();
    dnq_lcd_deinit();
    dnq_mcu_deinit();
    dnq_uart_deinit();
    dnq_deinit();

    return 0;
}

