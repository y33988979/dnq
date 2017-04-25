/****************************************************************************
 *                                                                          *
 * Copyright (c) 2014 Nuvoton Technology Corp. All rights reserved.         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 *
 * FILENAME
 *     uart_test.c
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     This is the test program used to test the UARTs on NUC970 EV board
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *     None
 *
 * HISTORY
 *
 *
 * REMARK
 *     None
 ****************************************************************************/
#include     <stdio.h>
#include     <stdlib.h>
#include     <unistd.h>
#include     <sys/types.h>
#include     <sys/stat.h>
#include     <fcntl.h>
#include     <termios.h>
#include     <errno.h>
#include     <string.h>
#include 	<signal.h>
#include    <pthread.h>

#define FALSE 0
#define TRUE  1

int fd[2];

pthread_t threads[10];

char buff[101];

static struct termios newtios,oldtios; /*termianal settings */
static int saved_portfd=-1;            /*serial port fd */


static void reset_tty_atexit(void)
{
	if(saved_portfd != -1)
	{
		//tcsetattr(saved_portfd,TCSANOW,&oldtios);
	}
}

/*cheanup signal handler */
static void reset_tty_handler(int signal)
{
	if(saved_portfd != -1)
	{
		//tcsetattr(saved_portfd,TCSANOW,&oldtios);
	}
	_exit(EXIT_FAILURE);
}

static int open_port(const char *portname)
{
	struct sigaction sa;
	int ret;
	int portfd;

	printf("opening serial port:%s\n",portname);
	/*open serial port */
	if((portfd=open(portname,O_RDWR | O_NOCTTY | O_NDELAY)) < 0 ) // | O_NOCTTY |O_NDELAY
	{
   		printf("open serial port %s fail \n ",portname);
   		return portfd;
	}
#if 1
	if( (fcntl(portfd, F_SETFL, 0)) < 0 )
    {
        perror("Fcntl F_SETFL Error!\n");
        return -1;
    }
#endif
	/*get serial port parnms,save away */
	ret = tcgetattr(portfd,&newtios);
	printf("line%d: ret=%d\n", __LINE__, ret);
	memcpy(&oldtios,&newtios,sizeof newtios);

	printf("--------------------------------------\n");
	printf("c_iflag= 0x%04x\n", newtios.c_iflag);
	printf("c_oflag= 0x%04x\n", newtios.c_oflag);
	printf("c_cflag= 0x%04x\n", newtios.c_cflag);
	printf("c_lflag= 0x%04x\n", newtios.c_lflag);
	printf("c_line = 0x%x\n", newtios.c_line);
	printf("c_cc   = 0x%d\n", NCCS);
	printf("--------------------------------------\n");
	/* configure new values */
	cfmakeraw(&newtios); /*see man page */
	printf("line%d: ret=%d\n", __LINE__, ret);
	//newtios.c_iflag |=IGNPAR; /*ignore parity on input */
	//newtios.c_oflag &= ~(OPOST | ONLCR | OLCUC | OCRNL | ONOCR | ONLRET | OFILL);
	//newtios.c_cflag = CS8 | CLOCAL | CREAD;
	newtios.c_cflag &= ~CSIZE; //????
	newtios.c_cflag |=  (CS8 | CLOCAL | CREAD);
	printf("newtios.c_cflag & CS8  ===== %d\n", newtios.c_cflag & CS8);
	//newtios.c_cflag |=  CS8;
	printf("newtios.c_cflag & CS8  ===== %d\n", newtios.c_cflag & CS8);
	newtios.c_cflag &= ~PARENB;
	newtios.c_iflag &= ~INPCK; 
	newtios.c_cflag &= ~CSTOPB; // must care
	

	printf("buid time = %s, %s\n", __DATE__, __TIME__);

	newtios.c_cc[VMIN]=0; /* block until 1 char received */
	newtios.c_cc[VTIME]=1; /*no inter-character timer */
	/* 115200 bps */
	ret = cfsetospeed(&newtios,B115200);
	printf("line%d: ret=%d\n", __LINE__, ret);
	ret = cfsetispeed(&newtios,B115200);
	printf("line%d: ret=%d\n", __LINE__, ret);
	#if 0
	/* register cleanup stuff */
	atexit(reset_tty_atexit);
	memset(&sa,0,sizeof sa);
	sa.sa_handler = reset_tty_handler;
	sigaction(SIGHUP,&sa,NULL);
	sigaction(SIGINT,&sa,NULL);
	sigaction(SIGPIPE,&sa,NULL);
	sigaction(SIGTERM,&sa,NULL);
	#endif
	/*apply modified termios */
	saved_portfd=portfd;
	ret = tcflush(portfd,TCIFLUSH);
	printf("line%d: ret=%d\n", __LINE__, ret);
	//ret = tcsetattr(portfd,TCSADRAIN,&newtios);//raw
	ret = tcsetattr(portfd,TCSANOW,&newtios);
	printf("line%d: ret=%d\n", __LINE__, ret);
	return portfd;
}

void * lcd(void* arg)
{
	int portfd = (int) arg;
	unsigned char i, j;
	int send, recv;
	int rev1 = 0, rev2 = 0;
	char buffer[64] = { 0xA5, 0x5A, 0x03, 0x81, 0x00, 0x01};

	send = write(portfd, buffer, 6);
	printf("\n uart6 send %d byts\n", send);
	for(i=0; i<send; i++)
		printf("%02x ", buffer[i]);
	printf("\n");
	
	usleep(300*1000);
	recv = read(portfd, buffer, 100);
	printf("\n uart6 recv %d byts\n", recv);
	for(i=0; i<recv; i++)
		printf("%02x ", buffer[i]);
	printf("\n");
}

void * process1(void* arg)
{
	int portfd = (int) arg;
	unsigned char i, j;
	int rev1, rev2;
	char RxBuffer[101];

	rev1 =0;
	rev2 =0;

	while(rev2 < 100)
   	{
		rev1 = write(portfd,(buff+rev2),100);
		rev2 += rev1;
   	}

	printf("\n uart1 send %d byts\n", rev2);
	for(i = 0; i < 100; i++)
		printf("%02x ", buff[i]);
	printf("\n");

	rev1 = 0;
	rev2 = 0;

	while(rev2 < 100)
	{
		rev1 = read(portfd,(RxBuffer+rev2),100);
		rev2 += rev1;
	}

	printf("\n uart1 receive %d bytes\n", rev2);

	for(i = 0; i < 100; i++)
	{
		printf("%02x ", RxBuffer[i]);
		if(i != RxBuffer[i])
		{
			printf("\n uart1 compare Error!!");

			//while(1);
		}
	}

	printf("\n uart1 compare correct!!\n");
	printf("\n uart1 test done!!\n");

}

void * process2(void* arg)
{
	int portfd = (int) arg;
	unsigned char i, j;
	int rev1, rev2;
	char RxBuffer[101];
sleep(100);
	rev1 =0;
	rev2 =0;

	while(rev2 < 100)
   	{
		rev1 = write(portfd,(buff+rev2),100);
		rev2 += rev1;
   	}

	printf("\n uart2 send %d bytes \n", rev2);

	rev1 = 0;
	rev2 = 0;

	while(rev2 < 100)
	{
		rev1 = read(portfd,(RxBuffer+rev2),100);
		rev2 += rev1;
	}

	printf("\n uart2 receive %d bytes \n", rev2);

	for(i = 0; i < 100; i++)
	{
		if(i != RxBuffer[i])
		{
			printf("\n uart2 compare Error!!");
			while(1);
		}
	}

	printf("\n uart2 compare correct!!\n");
	printf("\n uart2 test done!!\n");

}

/**
*@breif 	main()
*/
int main(int argc, char **argv)
{
	char *dev[10]={"/dev/ttyS6", "/dev/ttyS2"};
	unsigned int i;

	printf("\n demo uart1/uart2 external loop back function \n");

	for(i = 0; i < 100; i++)
	{
		buff[i] = (i & 0xff);
	}


	for(i = 0; i < 2; i++)
	{
		if((fd[i] = open_port(dev[i]))<0)
   			return -1;
	}

	pthread_create(&threads[0], NULL, lcd, (void*)(fd[0]));
	//pthread_create(&threads[1], NULL, process2, (void*)(fd[1]));

	pthread_join(threads[0], NULL);
	//pthread_join(threads[1], NULL);


  return 0;
}
