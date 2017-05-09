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



void *send_test(void *args)
{
    int n = 0;
    int cnt = 0;
    dnq_queue_t *queue;
    dnq_msg_t send_msg;

    queue = (dnq_queue_t *)args;
    send_msg.Class= 15;
    send_msg.code = 16;
    send_msg.lenght= 17;
    strcpy(send_msg.data, "nihao wjehzoq,!");
    //sleep(1111);
 
    while(1)
    {
        send_msg.Class = n+1;
        send_msg.code = n+2;
        send_msg.lenght = n+3;
        strcpy(send_msg.data, "8982ssaadhjh222989");

        dnq_msg_send(queue, &send_msg);

        printf("send msg!!\n");
        n += 100;
        usleep(100*1000);
        if(cnt++ % 5 == 0)
            sleep(60);
    }
}

int main()
{
    
    //extern int lcd_test();
    //lcd_test();
    //network_test();
    S32 len;
    U8 buffer[1024];
    U8 cmd[] = {0xFF, 0xFE, 0xFE, 0xFF, 0xA0, 0x0D, 0xB2,\
    0x21, 0x57, 0xFE, 0xFF, 0xFF, 0xFE};
#if 0
    len = dnq_mcu_uart_read(buffer, 32);
    printf("len=%d,buffer=%s",len,buffer);
    dnq_heater_ctrl_single(0, 0, 1);
    
    printf("sizeof(dnq_config_t)==%d\n", sizeof(dnq_config_t));

    sleep(100);
#endif
#if 1
    dnq_queue_t *queue = NULL;
    dnq_init();
    dnq_debug_init();
    
    dnq_uart_init();

    tt;
    //len = dnq_mcu_uart_read(buffer, 32);
    tt;
    //printf("len=%d,buffer=%s",len,buffer);
    while(1)
    {
        memset(buffer, 0, sizeof(buffer));
        len = dnq_mcu_uart_write(cmd, sizeof(cmd));
        printf("write len=%d,buffer=%s\n",len,buffer);
        len = dnq_mcu_uart_read(buffer, 32);
        printf("recv len=%d,buffer=%s\n",len,buffer);
        sleep(1);
    }
    
    dnq_heater_ctrl_single(0, 0, 1);
    
    printf("sizeof(dnq_config_t)==%d\n", sizeof(dnq_config_t));

    //sleep(100);

    
    dnq_lcd_init();
    dnq_keypad_init();

    queue = dnq_queue_create(QUEUE_SIZE_MAX);
    if(queue == NULL)
        goto exit;

    dnq_task_create("send", 32*2048, send_test, (void*)queue);

    dnq_msg_t recv_msg;
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
    dnq_uart_deinit();
    dnq_deinit();
#endif
    return 0;
}

