#include "recv.h"
 
int nFd = 0;
struct termios stNew;
struct termios stOld;
 
//Open Port & Set Port
int SerialInit()
{
    nFd = open("/dev/ttyS6", O_RDWR|O_NOCTTY|O_NDELAY);
    if(-1 == nFd)
    {
        perror("Open Serial Port Error!\n");
        return -1;
    }
    if( (fcntl(nFd, F_SETFL, 0)) < 0 )
    {
        perror("Fcntl F_SETFL Error!\n");
        return -1;
    }
    if(tcgetattr(nFd, &stOld) != 0)
    {
        perror("tcgetattr error!\n");
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
    tcflush(nFd,TCIFLUSH);  //清空终端未完成的输入/输出请求及数据。
    if( tcsetattr(nFd,TCSANOW,&stNew) != 0 )
    {
        perror("tcsetattr Error!\n");
        return -1;
    }
 
    return nFd;
}

int main(int argc, char **argv)
{
    int nRet = 0;
    char buf[SIZE];
    char buffer[64] = {0xA5, 0x5A, 0x03, 0x81, 0x00, 0x01};
 
    if( SerialInit() == -1 )
    {
        perror("SerialInit Error!\n");
        return -1;
    }
 
    memset(buf, 0, SIZE);
    while(1)
    {
	strcpy(buf, "hello !\n");
	int i;
	for(i=0; i<SIZE; i++)
		;//buf[i] = 0xFF;
        nRet = write(nFd, buffer, 6);
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
        nRet = read(nFd, buf, SIZE);
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
 
    close(nFd);
    return 0;
}
