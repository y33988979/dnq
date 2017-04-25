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
    int i = 0, j = 0;
	int flag = 0;
	int len = 0;
    int nRet = 0;
	char sbuffer[24][SIZE] = {0};
    char sbuffer1[SIZE] = {0xA5, 0x5A, 0x03, 0x82, 0x00, 0x00};
	char sbuffer2[SIZE] = {0xA5, 0x5A, 0x03, 0x82, 0x00, 0x20};
	char sbuffer3[SIZE] = {0xA5, 0x5A, 0x03, 0x82, 0x00, 0x40};
	char sbuffer4[SIZE] = {0xA5, 0x5A, 0x03, 0x82, 0x00, 0x60};
	char sbuffer5[SIZE] = {0xA5, 0x5A, 0x03, 0x82, 0x00, 0x80};
	char sbuffer6[SIZE] = {0xA5, 0x5A, 0x03, 0x82, 0x00, 0xA0};
    char rbuffer[SIZE] = {0x00};

	if(argc > 1)
		flag = 1;
    memset(rbuffer, 0, SIZE);
	memset(sbuffer, 0, sizeof(sbuffer));
	printf("sbuffer sizeof == %d\n", sizeof(sbuffer));
	printf("build date: %s %s\n", __DATE__, __TIME__);

	//strcpy(&sbuffer1[6], "松花江小学-主楼-三楼西/一号箱-2017年5月15日 18:88:88");
	strcpy(&sbuffer1[6], " 松花江小学-主楼-三楼西/一号箱  2017年5月15日 18:33:33");
	strcpy(&sbuffer2[6], "序号     房间        室温   设置温度   状态     SN    温度校准 ");
	strcpy(&sbuffer3[6], " 1     三年二班     22.1度     32度     工作     正常     -42 ");
	strcpy(&sbuffer4[6], " 2     四年六班     12.7度     19度     工作     正常     -2  ");
	strcpy(&sbuffer5[6], " 网络状态：    正常     MAC：20-21-3E-43-FE-47-29     ↑:上一页 ↓:下一页  ");
	strcpy(&sbuffer6[6], " 当前执行命令：加热     火娃电采暖智能控制器                  ");

	for(i=0; i<23; i++)
	{
		sbuffer[i][0] = 0xA5;
		sbuffer[i][1] = 0x5A;
		sbuffer[i][2] = 0x00;
		sbuffer[i][3] = 0x82;

		len = 0x80 * i;
		sbuffer[i][4] = (len >> 8) & 0xFF;
		sbuffer[i][5] = len & 0xFF;
	}
	printf("sbuffer1  len  ===%d===\n", strlen(&sbuffer1[6]));
	/* 标题 */
	strcpy(&sbuffer[0][6], &sbuffer1[6]);

	/* 功能名称 */
	strcpy(&sbuffer[1][6], &sbuffer2[6]);

	/* 条目 */
	for(i=2; i<14; i++)
	{
		printf("addr = %02x %02x\n", sbuffer[i][4], sbuffer[i][5]);
		//sprintf(&sbuffer[i][6], "line[%d]: %s", i, "我爱祖国，我爱祖国，我爱祖国，我爱祖国，我爱祖国，我爱祖国，，，，");
		strcpy(&sbuffer[i][6], &sbuffer3[6]);
		sbuffer[i][7] = i-1+48;
		if(i>10)
		{
			printf("n==%d\n", i);
			sbuffer[i][7] = (i-1)/10+48;
			sbuffer[i][8] = (i-1)%10+48;
			//加一个空格
			strcpy(&sbuffer[i][6+3], &sbuffer3[6+2]);
		}
	}

	/* 网络状态，命令动作 */
	strcpy(&sbuffer[i][6], &sbuffer5[6]);

	/* 系统信息 */
	i++;
	strcpy(&sbuffer[i][6], &sbuffer6[6]);

    if(argc != 1)
    {
        //printf("command: 0x%02x 0x%02x 0x%02x ", sbuffer[0], sbuffer[1], sbuffer[2]);
        for(i=1; i<argc; i++)
        {
            //sbuffer[i+2] = strtol(argv[i], NULL, 16);
            //printf("0x%02x ", sbuffer[i+2]);
        }
        printf("\n");
	}

    if( SerialInit() == -1 )
    {
        perror("SerialInit Error!\n");
        return -1;
    }
	printf("test1!\n");
    while(1)
    {
		#if 0  //松花江小学
		len = sbuffer1[2] = strlen(&sbuffer1[6]);
        nRet = write(nFd, sbuffer1, len+6);
		len = sbuffer2[2] = strlen(&sbuffer2[6]);
        nRet = write(nFd, sbuffer2, len+6);
		len = sbuffer3[2] = strlen(&sbuffer3[6]);
        nRet = write(nFd, sbuffer3, len+6);
		len = sbuffer4[2] = strlen(&sbuffer4[6]);
        nRet = write(nFd, sbuffer4, len+6);
		len = sbuffer5[2] = strlen(&sbuffer5[6]);
        nRet = write(nFd, sbuffer5, len+6);
		len = sbuffer6[2] = strlen(&sbuffer6[6]);
        nRet = write(nFd, sbuffer6, len+6);
		#endif

		for(i=0; i<16; i++)
		{
			sbuffer[i][2] = strlen((char*)&sbuffer[i][6])+3;
			len = sbuffer[i][2];
			printf("send data: len=%d\n", len+3);
			if(flag)
			{
				for(j=0; j<len+6; j++)
					printf("%02x ", sbuffer[i][j]);
				printf("\n");
			}
			nRet = write(nFd, sbuffer[i], len+6);
		}

        if(-1 == nRet)
        {
            perror("send Data Error!\n");
            break;
        }
        if(0 < nRet)
        {
            sbuffer1[nRet] = 0;
            printf("send Data: len=%d\n", nRet);
        }
		break;
#if 0
	usleep(200*1000);
        memset(rbuffer, 0, SIZE);
        nRet = read(nFd, rbuffer, sizeof(rbuffer));
        if(-1 == nRet)
        {
            perror("recv Data Error!\n");
            break;
        }
        if(0 < nRet)
        {
            rbuffer[nRet] = 0;
            printf("recv Data: ");
            for(i=0; i<nRet; i++)
            {
                printf("0x%02x ", rbuffer[i]);
            }
            printf(", len=%d\n", nRet);
            break;
        }

#endif
    }


    close(nFd);
    return 0;
}
