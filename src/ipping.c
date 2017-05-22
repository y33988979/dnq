/*************************************************************************
* Copyright(c) 2009, xxxcorporation
* All rights reserved

* ipping.c : ip ping if ---arp ping if

* version: 	1.0
* author: 	ywg
* time:	2009-02-07

* history:	none
**************************************************************************/
/* system */
#include "stdio.h"
#include "string.h"
#include <stdlib.h>
#include "sys/types.h"
#include "sys/time.h"
#include "sys/stat.h"
#include "sys/ipc.h"
#include "fcntl.h"
#include "signal.h"
#include "unistd.h"
#include "time.h"
#include "pthread.h"
#include <sys/ioctl.h>
#include <ctype.h>
#include <errno.h>
#include <asm/types.h>

//#include  "linux/if.h"  //zzx add
//#include  "linux/sockios.h"   //zzx  add
/* socket */
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <netinet/ether.h>
#include "sys/socket.h"
#include "netinet/in.h"
#include "netdb.h"
#include <linux/route.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/tcp.h>
#include <linux/ethtool.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <linux/ip.h>


#define ICMP_ECHO 8
#define ICMP_ECHOREPLY 0
#define BUFSIZE 4096
// ICMP Header 
typedef struct ICMPHDR
{
  union
  {
    U16 tc;
    struct
    {
        U8 Type; // Type
        U8 Code; // Code
    }TYPECODE;
  }typecode;
  
  U16 Checksum; // Checksum
  U16 ID; // Identification
  U16 Seq; // Sequence
}ICMPHDR;

 //ICMP Echo Request
typedef struct ECHOREQUEST
{
    struct ICMPHDR icmpHdr;
    U8 cData[40];
}ECHOREQUEST;

// ICMP Echo Reply
typedef struct ECHOREPLY
{
  struct ECHOREQUEST echoRequest;
}ECHOREPLY;

U16 checksum(U16 *buffer, U32 size);
S32 WaitForEchoReply(S32 socket, U32 sec);
S32 icmp_parse_packet(U8 *buf, U32 len);

static U16 checksum(U16 *buffer, U32 size) 
{ 
	//将缓冲区中的数据加起来
	//数据（包括IP头和IP数据）在存储区中实际上是以二进制的形式存放的，
	//所以相当于将这些二进制数据加在一起
	unsigned long cksum=0; 
	while(size >1) 
	{ 
		cksum+=*buffer++; 
		size -=sizeof(U16); 
	} 
	if(size ) 
	{ 
		cksum += *(U16 *)buffer; 
	} 
	//上面完成了求和运算
	//十六位运算有可能而且很有可能产生超过十六位的结果
	//回卷  取出高十六位 加上 原来的低十六位
	cksum = (cksum >> 16) + (cksum & 0xffff); 
	//再加一次可保证计算值在十六位内
	cksum += (cksum >>16); 
	//取反返回即为校验和
	return (U16)(~cksum); 
} 

//Wait for echoRecv
S32 WaitForEchoReply(S32 socket, U32 sec)
{
    struct timeval Timeout;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socket, &readfds);

    if(sec > 0){
        Timeout.tv_sec = sec;
        Timeout.tv_usec = 0;
    }else{
        Timeout.tv_sec = 0;
        Timeout.tv_usec = 100000;
    }

    if (select(socket+1, &readfds, NULL, NULL, &Timeout)>0)
    {
        return(FD_ISSET(socket, &readfds));
    }
    else
        return -1;
}

S32 icmp_parse_packet(U8 *buf, U32 len)
{
	S32 iphdrlen, icmplen;
	struct iphdr *ip;
	struct ECHOREPLY *icmpRecv;

	ip = (struct iphdr *)buf;		/*start of  IP header*/
	iphdrlen = ip->ihl << 2;	/*length of IP header*/
	icmpRecv = (struct ECHOREPLY *)(buf + iphdrlen);
	if ( (icmplen = len - iphdrlen) < 8 )
		return -1;

	if ((icmpRecv->echoRequest.icmpHdr.typecode.TYPECODE.Type == ICMP_ECHOREPLY) && 
			(icmpRecv->echoRequest.icmpHdr.ID == 0x0200))
		return 1;
	else 
		return -1;	
}

S32 dnq_ping_test(U32 ip, U32 sec)
{
    S32 sockfd;
    S32 nRet;
    ECHOREQUEST echoReq;
    ECHOREPLY icmpRecv;
    struct sockaddr_in addrDest;

    S32 Seq = 1;
    S32 Length = 32;
    S32 Plength;
    S32 addr_len;

    U8 recvbuf[BUFSIZE];

    memset(&echoReq, 0 ,sizeof(echoReq));
    memset(&icmpRecv, 0 ,sizeof(icmpRecv));

    Plength = sizeof(ICMPHDR)+Length;

    if((sockfd = socket(AF_INET, SOCK_RAW, 1)) < 0)
    { 
        printf("socket icmp error! errno=%d:%s\n", errno, strerror(errno));
        return -1;
    } 

    addrDest.sin_family = AF_INET;
    addrDest.sin_addr.s_addr = htonl(ip);

    memset(addrDest.sin_zero, 0, 8); 
    echoReq.icmpHdr.typecode.TYPECODE.Type = ICMP_ECHO; 
    echoReq.icmpHdr.typecode.TYPECODE.Code = 0;
    echoReq.icmpHdr.ID = 0x0200;

    for (nRet = 0; nRet <Length; nRet++)
    {
        echoReq.cData[nRet] = 'a'+nRet; 
    }

    echoReq.icmpHdr.Seq = Seq++;
    echoReq.icmpHdr.Checksum = 0;
    echoReq.icmpHdr.Checksum = checksum((unsigned short*)&echoReq, Plength);

    addr_len = sizeof(struct sockaddr);

    if(sendto(sockfd, (struct ECHOREQUEST*)&echoReq, Plength, 0, (struct sockaddr *)&addrDest, addr_len) < 0)
    { 
        close(sockfd);
        return -1;
    } 

    if(WaitForEchoReply(sockfd, sec)>0)
    {
        nRet = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&addrDest, &addr_len);
        if(nRet>0)
        {
            if(icmp_parse_packet(recvbuf, nRet) == -1)  
            {
                close(sockfd);
                return -2;
            }
        }
        else
        {
            close(sockfd);
            return -2;
        }
    }
    else 
    {
        close(sockfd);
        return -2;
    }
    
    close(sockfd);
    return 1;
}

#if 0
//CheckSum 计算校验和
unsigned short checksum(unsigned short *buffer, int size) 
{ 
	//将缓冲区中的数据加起来
	//数据（包括IP头和IP数据）在存储区中实际上是以二进制的形式存放的，
	//所以相当于将这些二进制数据加在一起
	unsigned long cksum=0; 

        if (buffer == NULL)
        {
            Err_Print("[MOTO]checksum parameter is null!\n");
            return 0;
        }
    
	while(size >1) 
	{ 
		cksum+=*buffer++; 
		size -=sizeof(unsigned short); 
	} 
	if(size ) 
	{ 
		cksum += *(unsigned short *)buffer; 
	} 
	//上面完成了求和运算
	//十六位运算有可能而且很有可能产生超过十六位的结果
	//回卷  取出高十六位 加上 原来的低十六位
	cksum = (cksum >> 16) + (cksum & 0xffff); 
	//再加一次可保证计算值在十六位内
	cksum += (cksum >>16); 
	//取反返回即为校验和
	return (unsigned short)(~cksum); 
} 
#endif

