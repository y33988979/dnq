/* dnq uart Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  this is a uart interface API, for app.
 * Note : 
 */
 
#include "dnq_uart.h"
#include "dnq_log.h"

#include <termios.h>
 
static int lcd_uart_fd = 0;
static int mcu_uart_fd = 0;
static int sensor_uart_fd = 0;

struct termios stNew;
struct termios stOld;
 
//Open Port & Set Port
S32 dnq_uart_open(U8 *dev)
{
    U32   fd;
    fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
    if(-1 == fd)
    {
        //dnq_error_en(fd, "Open Serial Port Error!\n");
        DNQ_ERROR(DNQ_MOD_UART, "Open Serial \"%s\" Port failed! errno:%s", \
        dev, strerror(errno));
        return -1;
    }
    if( (fcntl(fd, F_SETFL, 0)) < 0 )
    {
        close(fd);
        //dnq_error_en(fd, "Fcntl F_SETFL Error!\n");
        DNQ_ERROR(DNQ_MOD_UART, "Fcntl \"%s\" F_SETFL error! errno:%s", \
        dev, strerror(errno));
        return -1;
    }
    if(tcgetattr(fd, &stOld) != 0)
    {
        close(fd);
        //dnq_error_en(fd, "tcgetattr error!\n");
        DNQ_ERROR(DNQ_MOD_UART, "tcgetattr \"%s\" error! errno:%s", \
        dev, strerror(errno));
        return -1;
    }
 
    stNew = stOld;
    cfmakeraw(&stNew);//将终端设置为原始模式，该模式下所有的输入数据以字节为单位被处理
 
    //set speed
    cfsetispeed(&stNew, BAUDRATE);//115200
    cfsetospeed(&stNew, BAUDRATE);
 
    //set databits
    stNew.c_cflag |= (CLOCAL|CREAD);
    stNew.c_cflag &= ~CSIZE;
    stNew.c_cflag |= CS8;
 
    //set parity
    stNew.c_cflag &= ~PARENB;
    stNew.c_iflag &= ~INPCK;
 
    //set stopbits
    stNew.c_cflag &= ~CSTOPB;
    stNew.c_cc[VTIME]=0;    //指定所要读取字符的最小数量
    stNew.c_cc[VMIN]=1; //指定读取第一个字符的等待时间，时间的单位为n*100ms
                //如果设置VTIME=0，则无字符输入时read（）操作无限期的阻塞
    tcflush(fd,TCIFLUSH);  //清空终端未完成的输入/输出请求及数据。
    if( tcsetattr(fd,TCSANOW,&stNew) != 0 )
    {
        close(fd);
        dnq_error_en(fd, "tcsetattr error!\n");
        DNQ_ERROR(DNQ_MOD_UART, "tcsetattr \"%s\" error! errno:%s", \
        dev, strerror(errno));
        return -1;
    }
 
    return fd;
}

S32 dnq_uart_close(U32 fd)
{
    close(fd);
}

S32 dnq_uart_init()
{
    lcd_uart_fd = dnq_uart_open(DNQ_LCD_UART);
    if(lcd_uart_fd < 0)
    {
        DNQ_ERROR(DNQ_MOD_UART, "Lcd uart port6 init failed!");
        return -1;
    }
    mcu_uart_fd = dnq_uart_open(DNQ_MCU_UART);
    if(mcu_uart_fd < 0)
    {
        DNQ_ERROR(DNQ_MOD_UART, "Mcu uart port1 init failed!");
        return -1;
    }
    sensor_uart_fd = dnq_uart_open(DNQ_SENSOR_UART);
    if(sensor_uart_fd < 0)
    {
        DNQ_ERROR(DNQ_MOD_UART, "485 uart port8 init failed!");
        return -1;
    }
    DNQ_INFO(DNQ_MOD_UART, "dnq_uart_init ok!");
    return 0;
}

S32 dnq_uart_deinit()
{
    if(lcd_uart_fd != -1)
    {
        close(lcd_uart_fd);
        lcd_uart_fd = -1;
    }
    if(mcu_uart_fd != -1)
    {
        close(mcu_uart_fd);
        mcu_uart_fd = -1;
    }
    if(sensor_uart_fd != -1)
    {
        close(sensor_uart_fd);
        sensor_uart_fd = -1;
    }
}

static S32 dnq_uart_read(U32 fd, U8 *buffer, U32 len)
{
    int i;
    int rlen;
    if(fd < 0)
    {
        DNQ_ERROR(DNQ_MOD_UART, "uart fd is not opened!");
        return -1;
    }
    
    rlen = read(fd, buffer, len);
    if(rlen < 0)
    {
        dnq_error_en(fd, "read error");
        DNQ_ERROR(DNQ_MOD_UART, "read error! errno:%s", strerror(errno));
        return rlen;
    }
    DNQ_DEBUG(DNQ_MOD_UART, "read len=%d, data:", rlen);
    for(i=0; i<rlen; i++)
        DNQ_PRINT(DNQ_MOD_UART, "%02x ", buffer[i]);
    DNQ_PRINT(DNQ_MOD_UART, "\n\n");
    
    return rlen;
}

static S32 dnq_uart_write(U32 fd, U8 *buffer, U32 len)
{
    int i = 0;
    int wlen;
    if(fd < 0)
    {
        DNQ_ERROR(DNQ_MOD_UART, "uart fd is not opened!");
        return -1;
    }
        
    wlen = write(fd, buffer, len);
    if(wlen < 0)
    {
        dnq_error_en(fd, "write error");
        DNQ_ERROR(DNQ_MOD_UART, "write error! errno:%s", strerror(errno));
        return wlen;
    }
    
    DNQ_DEBUG(DNQ_MOD_UART, "send len=%d, data:", wlen);
    for(i=0; i<wlen; i++)
        DNQ_PRINT(DNQ_MOD_UART, "%02x ", buffer[i]);
    DNQ_PRINT(DNQ_MOD_UART, "\n\n");
    return wlen;
}

S32 dnq_lcd_uart_read(U8 *buffer, U32 len)
{
    return dnq_uart_read(lcd_uart_fd, buffer, len);
}

S32 dnq_lcd_uart_write(U8 *buffer, U32 len)
{
    return dnq_uart_write(lcd_uart_fd, buffer, len);
}

S32 dnq_mcu_uart_read(U8 *buffer, U32 len)
{
    return dnq_uart_read(mcu_uart_fd, buffer, len);
}

S32 dnq_mcu_uart_write(U8 *buffer, U32 len)
{
    return dnq_uart_write(mcu_uart_fd, buffer, len);
}

S32 dnq_sensor_uart_read(U8 *buffer, U32 len)
{
    return dnq_uart_read(sensor_uart_fd, buffer, len);
}

S32 dnq_sensor_uart_write(U8 *buffer, U32 len)
{
    return dnq_uart_write(sensor_uart_fd, buffer, len);
}


int uart_test(int argc, char **argv)
{
    int  nRet = 0;
    int  fd;
    char buf[1024];
    char buffer[64] = {0xA5, 0x5A, 0x03, 0x81, 0x00, 0x01};
 
    if( dnq_uart_open(DNQ_LCD_UART) == -1)
    {
        perror("SerialInit Error!\n");
        return -1;
    }
 
    memset(buf, 0, sizeof(buf));
    while(1)
    {
    	strcpy(buf, "hello !\n");
    	int i;
    	for(i=0; i<sizeof(buf); i++)
    		;//buf[i] = 0xFF;
    		
        nRet = write(fd, buffer, 6);
        if(-1 == nRet)
        {
            perror("send Data Error!\n");
            break;
        }
        if(0 < nRet)
        {
            buf[nRet] = 0;
            printf("send Data: %s, len=%d\n", buf, nRet);
        }
        usleep(100*1000);
        nRet = read(fd, buf, sizeof(buf));
        if(-1 == nRet)
        {
            perror("read Data Error!\n");
            break;
        }
        if(0 < nRet)
        {
            buf[nRet] = 0;
            printf("read Data: %s, len=%d\n", buf, nRet);
        }
        for(i=0; i<nRet; i++)
            printf("%02x ", buf[i]);
        printf("\n");
    }
 
    dnq_uart_close(fd);
    return 0;
}
