#include "recv.h"

int nFd = 0;
struct termios stNew;
struct termios stOld;
typedef struct item
{
    int   id ;
    char  name[16];
    float   curr_temp;
    float   set_temp;
    char  status[16];
    char  sn[16];
    int   offset;
}house_item_t;

house_item_t g_items[16] = 
{
    {0, "三年二班", 22.1,32,"停止","正常",-2},
    {1, "门卫室", 24.2,26,"停止","正常",-2},
    {2, "走廊蓄热12", 11.2,5,"正常","正常",-2},
    {3, "走廊蓄热2", 11.2,5,"正常","正常",-2},
    {4, "科学实验室", 11.2,23,"正常","正常",-2},
    {5, "一年级教研室", 10.2,23,"停止","正常",-2},
    {6, "三年一班", 22.1,19,"正常","正常",-2},
    {7, "水房", 19.1,0,"停止","正常",-2},
    {8, "闲置房间1-2", 18.1,5,"正常","正常",-2},
    {9, "会议室南", 5.9, 5,"正常","正常",-2},
    {10, "会议室北",5.2,5,"正常","正常",-2},
    {11, "楼梯间2-3", 13.2,15,"正常","正常",-2},
   
};

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

int write_data(char *data,int len)
{
    int i, nRet;
    nRet = write(nFd, data, len);
    
    for(i=0; i<len; i++)
        printf("%02x ", data[i]);
    printf("\n");
    
    if(-1 == nRet)
    {
        perror("send Data Error!\n");
        return nRet;
    }
    if(0 < nRet)
    {
        printf("send Data: len=%d\n", nRet);
    }
    return nRet;
}

int main(int argc, char **argv)
{
    int i = 0, j = 0 ,k = 0;
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
    char temp_buffer[SIZE] = {0x00};
    char item_buffer[SIZE] = {0x00};
    
    if( SerialInit() == -1 )
    {
        perror("SerialInit Error!\n");
        return -1;
    }
    
	if(argc > 1)
		flag = 1;
    memset(rbuffer, 0, SIZE);
	memset(sbuffer, 0, sizeof(sbuffer));
	printf("sbuffer sizeof == %d\n", sizeof(sbuffer));
	printf("build date: %s %s\n", __DATE__, __TIME__);

	//strcpy(&sbuffer1[6], "松花江小学-主楼-三楼西/一号箱-2017年5月15日 18:88:88");
	strcpy(&sbuffer1[6], " 松花江小学-主楼-三楼西/一号箱  2017年5月15日 18:33:33");
	strcpy(&sbuffer2[6], "序号     房间       室温    设置温度   状态     SN    温度校准 ");
	strcpy(&sbuffer3[6], " 1     三年二班     22.1度     32度     工作     正常     -42 ");
	strcpy(&sbuffer4[6], " 2     四年六班     12.7度     19度     工作     正常     -2  ");
	strcpy(&sbuffer5[6], " 网络状态：    正常     MAC：20-21-3E-43-FE-47-29     ↑:上一页 ↓:下一页  ");
	strcpy(&sbuffer6[6], " 当前执行命令：加热     火娃电采暖智能控制器                  ");

	for(i=0; i<22; i++)
	{
		sbuffer[i][0] = 0xA5;
		sbuffer[i][1] = 0x5A;
		sbuffer[i][2] = 0x00;
		sbuffer[i][3] = 0x82;

		len = 0x80 * i;
		sbuffer[i][4] = (len >> 8) & 0xFF;
		sbuffer[i][5] = len & 0xFF;
	}

    i=0;
	printf("sbuffer1  len  ===%d===\n", strlen(&sbuffer1[6]));
	/* 标题 */
	strcpy(&sbuffer[i][6], &sbuffer1[6]);
    sbuffer[i][2] = strlen((char*)&sbuffer[i][6])+3;
    write_data(sbuffer[i],sbuffer[i][2] + 3);
    i++;
    
	/* 功能名称 */
	strcpy(&sbuffer[1][6], &sbuffer2[6]);
    sbuffer[i][2] = strlen((char*)&sbuffer[i][6])+3;
    write_data(sbuffer[i],sbuffer[i][2] + 3);
    i++;
    
    /* 初始化帧头 */
    for(j = 0; j<7; j++)
    {
        sbuffer[j][0] = 0xA5;
        sbuffer[j][1] = 0x5A;
        sbuffer[j][2] = 0x00;
        sbuffer[j][3] = 0x82;
    }
    
	/* 条目buff初始化 */
	for(i=0; i<12; i++)
	{
        /* 条目序号 */
        j = 0;
        len = i*0x100 + j*0x20 + 0x100;
        sbuffer[j][4] = (len >> 8) & 0xFF;
        sbuffer[j][5] = len & 0xFF;
        g_items[i].id = i;
        sprintf(temp_buffer, "%02d" , g_items[i].id);
        strcpy(&sbuffer[j][6], temp_buffer);
        sbuffer[j][2] = strlen(temp_buffer) + 3;
        j++;
        
        /* 房间名称 */
        len = i*0x100 + j*0x20 + 0x100;
        sbuffer[j][4] = (len >> 8) & 0xFF;
        sbuffer[j][5] = len & 0xFF;
        strcpy(&sbuffer[j][6], g_items[i].name);
        printf("[%d]:name=%s, len=%d\n", i, g_items[i].name, strlen(g_items[i].name));
        sbuffer[j][2] = strlen(&sbuffer[j][6]) + 3;
        j++;
        
        /* 室温 */
        len = i*0x100 + j*0x20 + 0x100;
        sbuffer[j][4] = (len >> 8) & 0xFF;
        sbuffer[j][5] = len & 0xFF;
        sprintf(temp_buffer, "%2.1f'C" , g_items[i].curr_temp);
        strcpy(&sbuffer[j][6], temp_buffer);
        sbuffer[j][2] = strlen(temp_buffer) + 3;
        j++;
        
        /* 设置温度 */
        len = i*0x100 + j*0x20 + 0x100;
        sbuffer[j][4] = (len >> 8) & 0xFF;
        sbuffer[j][5] = len & 0xFF;
        sprintf(temp_buffer, "%2.1f'C" , g_items[i].set_temp);
        strcpy(&sbuffer[j][6], temp_buffer);
        sbuffer[j][2] = strlen(temp_buffer) + 3;
        j++;
        
        /* 状态 */
        len = i*0x100 + j*0x20 + 0x100;
        sbuffer[j][4] = (len >> 8) & 0xFF;
        sbuffer[j][5] = len & 0xFF;
        strcpy(&sbuffer[j][6], g_items[i].status);
        sbuffer[j][2] = strlen(&sbuffer[j][6]) + 3;
        j++;
        
        /* SN */
        len = i*0x100 + j*0x20 + 0x100;
        sbuffer[j][4] = (len >> 8) & 0xFF;
        sbuffer[j][5] = len & 0xFF;
        strcpy(&sbuffer[j][6], g_items[i].sn);
        sbuffer[j][2] = strlen(&sbuffer[j][6]) + 3;
        j++;
        
        /* 温度校准 */
        len = i*0x100 + j*0x20 + 0x100;
        sbuffer[j][4] = (len >> 8) & 0xFF;
        sbuffer[j][5] = len & 0xFF;
        sprintf(temp_buffer, "%d" , g_items[i].offset);
        strcpy(&sbuffer[j][6], temp_buffer);
        sbuffer[j][2] = strlen(temp_buffer) + 3;
        j++;
        for(k=0;k<7;k++)
        {           
            write_data(sbuffer[k],sbuffer[k][2] + 3);
        }       
	}
    
    /* 网络状态，命令动作 */
    i = 7;
    sbuffer[i][4] = 0x0D;
    sbuffer[i][5] = 0x00;
	strcpy(&sbuffer[i][6], &sbuffer5[6]);
    sbuffer[i][2] = strlen(&sbuffer6[6]) + 3;
    write_data(sbuffer[i],sbuffer[i][2] + 3);
    
	/* 系统信息 */
    i++;
    sbuffer[i][4] = 0x0E;
    sbuffer[i][5] = 0x00;
	strcpy(&sbuffer[i][6], &sbuffer6[6]);
    sbuffer[i][2] = strlen(&sbuffer6[6]) + 3;
    write_data(sbuffer[i],sbuffer[i][2] + 3);

    
    #if 0
    if(flag)
    {
    i = 1;
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    strcpy(&sbuffer[i++][6],"                                                                    ");
    }
    else 
    {
    i = 1;
    strcpy(&sbuffer[i++][6],"序号     房间      室温    设置温度   状态    SN   温度校准 ");
    strcpy(&sbuffer[i++][6]," 01    三年二班    22.1℃      32℃     工作   正常    -2 ");
    strcpy(&sbuffer[i++][6]," 02     门卫室     24.5℃      26℃     工作   正常    -10 ");
    strcpy(&sbuffer[i++][6]," 03   走廊蓄热12    11.2℃       5℃      停止   正常   -2 ");
    strcpy(&sbuffer[i++][6]," 04   走廊蓄热2    10.2℃       5℃     停止   正常    12 ");
    strcpy(&sbuffer[i++][6]," 05   科学实验室   22.1℃      23℃     停止   正常    -2 ");
    strcpy(&sbuffer[i++][6]," 06  一年级教研室   19.1℃      23℃     工作   正常    -2 ");
    strcpy(&sbuffer[i++][6]," 07    三年一班    18.1℃      19℃     停止   正常    -2 ");
    strcpy(&sbuffer[i++][6]," 08     水房      5.1℃       0℃     工作   正常    -2 ");
    strcpy(&sbuffer[i++][6]," 09   闲置房间1-2   4.9℃       5℃     停止   正常    -2 ");
    strcpy(&sbuffer[i++][6]," 10    会议室南    5.9℃       5℃     停止   正常    -2 ");
    strcpy(&sbuffer[i++][6]," 11    会议室北    5.2℃       5℃     停止   正常     1 ");
    strcpy(&sbuffer[i++][6]," 12    楼梯间2-3    13.2℃      15℃     停止   正常       ");
}
#endif

    i = 0;
    strcpy(&sbuffer[i][6], "工作");
    sbuffer[i][2] = strlen(&sbuffer[i][6]) + 3;
    sbuffer[i][4] = 0x04;
    sbuffer[i][5] = 0x80;
    
    i++;
    strcpy(&sbuffer[i][6], "停止");
    sbuffer[i][2] = strlen(&sbuffer[i][6]) + 3;
    sbuffer[i][4] = 0x04;
    sbuffer[i][5] = 0xE0;
    
    i++;
    strcpy(&sbuffer[i][6], "    ");
    sbuffer[i][2] = strlen(&sbuffer[i][6]) + 3;
    sbuffer[i][4] = 0x04;
    sbuffer[i][5] = 0x80;
    
    i++;
    strcpy(&sbuffer[i][6], "    ");
    sbuffer[i][2] = strlen(&sbuffer[i][6]) + 3;
    sbuffer[i][4] = 0x04;
    sbuffer[i][5] = 0xE0;
    
    i = 20;
    while(i--)
    {
        write_data(sbuffer[2],sbuffer[2][2] + 3);
        write_data(sbuffer[3],sbuffer[3][2] + 3);
        write_data(sbuffer[0],sbuffer[0][2] + 3);
        sleep(1);
        write_data(sbuffer[2],sbuffer[3][2] + 3);
        write_data(sbuffer[3],sbuffer[3][2] + 3);
        write_data(sbuffer[1],sbuffer[1][2] + 3);
        sleep(1);
    }
    
    close(nFd);
    return 0;
    
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
			nRet = write(nFd, sbuffer[i], len+3);
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
