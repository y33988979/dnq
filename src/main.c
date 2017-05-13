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

S32 rtc_test()
{
    U8 datetime[16];
    
    dnq_rtc_get_time(datetime);
    printf("datatime: %04d-%02d-%02d %02d:%02d:%02d!\n", \
        2000+datetime[0],datetime[1],datetime[2],\
        datetime[3],datetime[4],datetime[5] );
    
    return 0;
}

S32 room_ctrl_test()
{
    U32 i , len;
    U32 array1[17];
    U32 array2[17];
    U8  buffer[64];
    sleep(1);
    //len = dnq_mcu_uart_read(buffer, 32);
    //printf("len=%d,buffer=%s",len,buffer);
    while(1)
    {
        
        memset(buffer, 0, sizeof(buffer));
        //len = dnq_mcu_uart_write(cmd, sizeof(cmd));
        //printf("write len=%d,buffer=%s\n",len,buffer);
        //dnq_heater_ctrl_single(0, 0xB1, 1);
        //sleep(1);
        for(i=0;i<16;i++)
        {
            array1[i] = 2;
            array2[i] = 3;
        }
        dnq_heater_ctrl_whole(0xB0, array1);
        //sleep(1);
        //dnq_heater_ctrl_whole(0xB0, array2);
        //sleep(1);

        len = dnq_mcu_uart_read(buffer, 32);
        printf("recv len=%d!\n",len,buffer);
        for(i=0; i<len; i++)
            printf("0x%02x ", buffer[i]);
        printf("\n");
        sleep(1);
    }
    return 0;

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
    dnq_mcu_init();

    int i;
    #if 1// rs485 test!
    while(1)
    {
        for(i=0; i<16; i++)
        {
            dnq_room_temperature_get(i);
            usleep(100*1000);
        }
        
    }
    #endif

    U8 datatime[16] = {17, 5, 13, 22, 33, 44}; 
    dnq_rtc_set_time(datatime);
    sleep(1);
    while(1)
    {
        //room_ctrl_test();
        //rtc_test();
        //sleep(2);
    }
    

    printf("sizeof(dnq_config_t)==%d\n", sizeof(dnq_config_t));

    sleep(1000);

    
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

