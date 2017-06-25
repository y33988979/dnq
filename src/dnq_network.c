/* dnq network Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  this is a network interface API, for app.
 * Note : 
 */
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ipc.h>

#include <linux/route.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/tcp.h>
#include <linux/ethtool.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netdb.h>

#include <asm/types.h>
#include <sys/ioctl.h>
#include <ctype.h>

#include "dnq_log.h"
#include "dnq_common.h"
#include "dnq_network.h"

#define BAIDU_URL    "www.baidu.com"
#define TENCENT_URL  "www.qq.com"
#define TAOBAO_URL   "www.taobao.com"
#define YOUKU_URL    "www.youku.com"

extern S32 dnq_ping_test(U32 ip, U32 sec);

static host_net_info_t g_host_netinfo;
static U8 g_server_ip[16] = {0};
static U32 g_server_port = DNQ_SERVER_PORT;

U16 ipcfg_htons(U16 n)
{
    return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

U16 ipcfg_ntohs(U16 n)
{
    return ipcfg_htons(n);
}

U32 ipcfg_htonl(U32 n)
{
    return ((n & 0xff) << 24) |
           ((n & 0xff00) << 8) |
           ((n & 0xff0000) >> 8) |
           ((n & 0xff000000) >> 24);
}

U32 ipcfg_ntohl(U32 n)
{
    return ipcfg_htonl(n);
}

#define _BUFSIZE 2048
struct _route_info
{
    u_int dstAddr;
    u_int srcAddr;
    u_int gateWay;
    char ifName[/*IF_NAMESIZE*/16];
};
int _readNlSock(int sockFd, char *bufPtr, int seqNum, int pId)
{
    struct nlmsghdr *nlHdr;
    int readLen = 0, msgLen = 0;

    do
    {
        readLen = recv(sockFd, bufPtr, _BUFSIZE - msgLen, 0);
        nlHdr = (struct nlmsghdr *)bufPtr;
        if(nlHdr->nlmsg_type == NLMSG_DONE)
            break;

        bufPtr += readLen;
        msgLen += readLen;

        if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0)
            break;
    }
    while((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));
    
    return msgLen;
}

void _parseRoutes(struct nlmsghdr *nlHdr, struct _route_info *rtInfo,unsigned int *gateway)
{
    struct rtmsg *rtMsg;
    struct rtattr *rtAttr;
    int rtLen;
    //char tempBuf[100];
    struct in_addr dst;
    struct in_addr gate;

    rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);
    if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
        return;

    rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
    rtLen = RTM_PAYLOAD(nlHdr);

    for(; RTA_OK(rtAttr,rtLen); rtAttr = RTA_NEXT(rtAttr,rtLen))
    {
        switch(rtAttr->rta_type)
        {
            case RTA_OIF:
                if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
                break;
            case RTA_GATEWAY:
                rtInfo->gateWay = *(u_int *)RTA_DATA(rtAttr);
                break;
            case RTA_PREFSRC:
                rtInfo->srcAddr = *(u_int *)RTA_DATA(rtAttr);
                break;
            case RTA_DST:
                rtInfo->dstAddr = *(u_int *)RTA_DATA(rtAttr);
                break;
        }
    }

    dst.s_addr = rtInfo->dstAddr;

    if(strstr((char *)inet_ntoa(dst), "0.0.0.0"))
    {
        gate.s_addr = rtInfo->gateWay;
        //sprintf(gateway, "%s",(char *)inet_ntoa(gate));
        *gateway = gate.s_addr;
    }
    //DNQ_ERROR(DNQ_MOD_NETWORK, "parseRoute:%s\n",gateway);
    return;
}

S32 dnq_net_ifup(U8 *if_name)
{
    int sockfd;
    int ret;
    struct ifreq ifr;
    short flag;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "socket error! errno=%d:%s", errno,strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, if_name);

    // flag = IFF_UP;  //????
    //flag |= IFF_UP | IFF_RUNNING;
    ret = ioctl(sockfd, SIOCGIFFLAGS, &ifr);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "ioctl error! errno=%d:%s", errno,strerror(errno));
        close(sockfd);
        return -1;
    }

    //set flag
    ifr.ifr_ifru.ifru_flags |= (IFF_UP | IFF_RUNNING);//flag;
    ret = ioctl(sockfd, SIOCSIFFLAGS, &ifr);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "ioctl error! errno=%d:%s", errno,strerror(errno));
        close(sockfd);
        return -1;
    }

    close(sockfd);
    DNQ_INFO(DNQ_MOD_NETWORK, "ifup %s success!", if_name);
    return 0;
}
    
S32 dnq_net_ifdown(U8 *if_name)
{
    int sockfd;
    int ret;
    struct ifreq ifr;
    short flag;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "socket error! errno=%d:%s", errno,strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, if_name);

    flag = ~IFF_UP; //????
    ret = ioctl(sockfd, SIOCGIFFLAGS, &ifr);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "ioctl error! errno=%d:%s", errno,strerror(errno));
        close(sockfd);
        return -1;
    }
    //clear flag
    ifr.ifr_ifru.ifru_flags &= flag;
    //DNQ_ERROR(DNQ_MOD_NETWORK, "ip cfg down interface : falg [%lx] \n", ifr.ifr_ifru.ifru_flags);

    ret = ioctl(sockfd, SIOCSIFFLAGS, &ifr);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "ioctl error! errno=%d:%s", errno,strerror(errno));
        close(sockfd);
        return -1;
    }

    close(sockfd);
    DNQ_INFO(DNQ_MOD_NETWORK, "ifdown %s success!", if_name);
    return 0;
}


/*
* get dns
*/
U32 dnq_net_get_dns()
{
    int i1,i2,Result;
    FILE *pFile=NULL;
    char dns_buf[20];
    unsigned int dns_addr;
    char BufferTmp[100];
    struct in_addr addr;

    //pFile = fopen("/FlashData/resolv.conf","r");
    //pFile = fopen("/etc/config/resolv.conf","r");
    pFile = fopen("/etc/resolv.conf","r");
    //printf("STBDNS_Get begin\n");
    if(pFile == NULL)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "fopen error! errno=%d:%s", errno, strerror(errno));
        return 0;
    }
    //IPSET_DEBUG(("Get DNS step1 (open file)is OK!\n"));
    i1 = 0;
    i2 = 0;
    memset(BufferTmp,0,sizeof(BufferTmp));
    Result = fread(BufferTmp,sizeof(char),100,pFile);
    if(Result==0||Result<=19) /* nameserver 0.0.0.0 */
    {
        fclose(pFile);
        DNQ_ERROR(DNQ_MOD_NETWORK, "fread error! errno=%d:%s", errno,strerror(errno));
        return 0;
    }

    while(BufferTmp[i1]!=0)
    {
        if(strncmp(BufferTmp+i1,"nameserver",10)==0)
            break;
        i1++;
    }
    i1+=10;
    if(isdigit(BufferTmp[i1])==0)//if it's not a digit
        i1++;
    i2=i1;
    while(BufferTmp[i2]!=' '  ||BufferTmp[i2]!='\r')
    {
        if(isgraph(BufferTmp[i2])==0)//if it's not graph
            break;
        i2++;
    }

    strncpy(dns_buf,BufferTmp+i1,i2-i1);
    dns_buf[i2-i1] = '\0';
    dns_addr = (inet_addr(dns_buf));
    fclose(pFile);
    addr.s_addr = dns_addr;
    DNQ_INFO(DNQ_MOD_NETWORK, "get dns success: %s", inet_ntoa(addr));

    return dns_addr;
}

S32 dnq_net_get_dns2(U32 *dns_addrs)
{
    int i1,i2,Result;
    FILE *pFile=NULL;
    char dns_buf[20];
    unsigned int dns_addr;
    char BufferTmp[100];
    struct in_addr addr;

    //pFile = fopen("/FlashData/resolv.conf","r");
    pFile = fopen("/etc/resolv.conf","r");
    //printf("STBDNS_Get begin\n");
    if(pFile == NULL)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "fopen error! errno=%d:%s", errno, strerror(errno));
        return 0;
    }
    //IPSET_DEBUG(("Get DNS step1 (open file)is OK!\n"));
    i1 = 0;
    i2 = 0;
    memset(BufferTmp,0,sizeof(BufferTmp));
    Result = fread(BufferTmp,sizeof(char),100,pFile);
    if(Result==0||Result<=19) /* nameserver 0.0.0.0 */
    {
        fclose(pFile);
        DNQ_ERROR(DNQ_MOD_NETWORK, "fread error! errno=%d:%s", errno,strerror(errno));
        return 0;
    }

    while(BufferTmp[i1]!=0)
    {
        if(strncmp(BufferTmp+i1,"nameserver",10)==0)
            break;
        i1++;
    }
    i1+=10;
    if(isdigit(BufferTmp[i1])==0)//if it's not a digit
        i1++;
    i2=i1;
    while(BufferTmp[i2]!=' '  ||BufferTmp[i2]!='\n')
    {
        if(isgraph(BufferTmp[i2])==0)//if it's not graph
            break;
        i2++;
    }

    strncpy(dns_buf,BufferTmp+i1,i2-i1);
    dns_buf[i2-i1] = '\0';
    dns_addr = (inet_addr(dns_buf));

    dns_addrs[0] = dns_addr;

    /* get dns 2 */
    i1 = i2;
    while(BufferTmp[i1]!=0)
    {
        if(strncmp(BufferTmp+i1,"nameserver",10)==0)
            break;
        i1++;
    }
    i1+=10;
    if(isdigit(BufferTmp[i1])==0)//if it's not a digit
        i1++;
    i2=i1;
    while(BufferTmp[i2]!=' '  ||BufferTmp[i2]!='\n')
    {
        if(isgraph(BufferTmp[i2])==0)//if it's not graph
            break;
        i2++;
    }

    strncpy(dns_buf,BufferTmp+i1,i2-i1);
    dns_buf[i2-i1] = '\0';
    dns_addr = (inet_addr(dns_buf));

    dns_addrs[1] = dns_addr;

    fclose(pFile);
    addr.s_addr = dns_addrs[0];
    DNQ_INFO(DNQ_MOD_NETWORK, "get dns1 success: %s", inet_ntoa(addr));
    addr.s_addr = dns_addrs[1];
    DNQ_INFO(DNQ_MOD_NETWORK, "get dns2 success: %s", inet_ntoa(addr));
    return 0;
}

/*
* set dns
*/
S32 dnq_net_set_dns(U32 dnsaddr)
{
    int Result;
    FILE *pFile=NULL;
    char *p;
    char dns_buf[20];
    struct in_addr dns_addr;
    char BufferTmp[100];
    struct in_addr addr;
    
    //system("ifconfig eth0 up");
    pFile = fopen("/etc/resolv.conf","w+");
    //printf("STBDNS_Get begin\n");
    if(pFile == NULL)
    {
        perror("fopen");
        DNQ_ERROR(DNQ_MOD_NETWORK, "fopen error! errno=%d:%s", errno, strerror(errno));
        return -1;
    }
    //IPSET_DEBUG(("Get DNS step1 (open file)is OK!\n"));

    memset(BufferTmp,0,sizeof(BufferTmp));

    /* 202.106.0.20 */
    dns_addr.s_addr = (dnsaddr);
    p = inet_ntoa(dns_addr);
    strcpy(dns_buf,p);
    strcpy(dns_buf+strlen(p)," \r\n");
    /* nameserver  */
    strncpy(BufferTmp, "nameserver ", strlen("nameserver "));
    /* nameserver 202.106.0.20 */
    strcpy((BufferTmp+11), dns_buf);

    Result = fwrite(BufferTmp, sizeof(char), strlen(BufferTmp), pFile);
    if(Result <= 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "write file error! errno=%d:%s", errno,strerror(errno));
        return -1;
    }

    fclose(pFile);
    DNQ_INFO(DNQ_MOD_NETWORK, "set dns success: %s", inet_ntoa(dns_addr));
    return 0;
}

S32 dnq_net_set_dns2(U32 dns_addr1, U32 dns_addr2)
{
    int Result;
    FILE *pFile=NULL;
    char *p;
    char dns_buf[20];
    struct in_addr dns_addr;
    char BufferTmp[100];
    int buf_pos=0;
    struct in_addr addr;
    
    pFile = fopen("/etc/resolv.conf","w+");
    //printf("STBDNS_Get begin\n");
    if(pFile == NULL)
    {
        perror("fopen");
        DNQ_ERROR(DNQ_MOD_NETWORK, "fopen error! errno=%d:%s", errno, strerror(errno));
        return -1;
    }
    //IPSET_DEBUG(("Get DNS step1 (open file)is OK!\n"));

    memset(BufferTmp,0,sizeof(BufferTmp));

    /* dns addr 1 */
    dns_addr.s_addr = (dns_addr1);
    p = inet_ntoa(dns_addr);
    strcpy(dns_buf,p);
    strcpy(dns_buf+strlen(p),"\n");
    /* nameserver  */
    buf_pos = strlen("nameserver ");
    strncpy(BufferTmp, "nameserver ", buf_pos);
    /* nameserver 202.106.0.20 */
    strcpy((BufferTmp+11), dns_buf);
    buf_pos += (strlen(p)+1);

    /* dns addr 2 */
    dns_addr.s_addr = (dns_addr2);
    p = inet_ntoa(dns_addr);
    strcpy(dns_buf,p);
    strcpy(dns_buf+strlen(p),"\n");
    /* nameserver  */
    strncpy((BufferTmp+buf_pos), "nameserver ", strlen("nameserver "));
    /* nameserver 202.106.0.20 */
    strcpy((BufferTmp+buf_pos+11), dns_buf);


    Result = fwrite(BufferTmp, sizeof(char), strlen(BufferTmp), pFile);
    if(Result <= 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "write file error! errno=%d:%s", errno,strerror(errno));
        return -1;
    }

    fclose(pFile);
    addr.s_addr = dns_addr1;
    DNQ_INFO(DNQ_MOD_NETWORK, "set dns1 success: %s", inet_ntoa(addr));
    addr.s_addr = dns_addr2;
    DNQ_INFO(DNQ_MOD_NETWORK, "set dns2 success: %s", inet_ntoa(addr));
    return 0;
}

S32 dnq_net_set_gw_addr(U8 *if_name, U32 gw)
{
    int skfd;
    struct rtentry rt;
    int err;
    struct in_addr addr;

    skfd = socket(/*PF_INET*/AF_INET, SOCK_DGRAM, 0);
    if(skfd < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "socket error! errno=%d:%s", errno,strerror(errno));
        return -1;
    }

    /* Delete existing defalt gateway */
    memset(&rt, 0, sizeof(rt));

    rt.rt_dst.sa_family = AF_INET;
    ((struct sockaddr_in *)&rt.rt_dst)->sin_addr.s_addr = 0;

    rt.rt_genmask.sa_family = AF_INET;
    ((struct sockaddr_in *)&rt.rt_genmask)->sin_addr.s_addr = 0;

    rt.rt_flags = RTF_UP;

    err = ioctl(skfd, SIOCDELRT, &rt);
    if(err < 0)
    {
        close(skfd);
        DNQ_ERROR(DNQ_MOD_NETWORK, "ioctl error!, errno=%d:%s", errno, strerror(errno));
        return -1;
    }
    
    if((err == 0 || errno == ESRCH) && gw)
    {
        /* Set default gateway */
        memset(&rt, 0, sizeof(rt));

        rt.rt_dst.sa_family = AF_INET;
        ((struct sockaddr_in *)&rt.rt_dst)->sin_addr.s_addr = 0;

        rt.rt_gateway.sa_family = AF_INET;
        ((struct sockaddr_in *)&rt.rt_gateway)->sin_addr.s_addr = (gw);

        rt.rt_genmask.sa_family = AF_INET;
        ((struct sockaddr_in *)&rt.rt_genmask)->sin_addr.s_addr = 0;

        rt.rt_flags = RTF_UP | RTF_GATEWAY;

        err = ioctl(skfd, SIOCADDRT, &rt);
        if(err < 0)
        {
            close(skfd);
            DNQ_ERROR(DNQ_MOD_NETWORK, "ioctl error!, errno=%d:%s", errno, strerror(errno));
            return -1;
        }
    }

    close(skfd);
    addr.s_addr = gw;
    DNQ_INFO(DNQ_MOD_NETWORK, "set gateway success: %s", inet_ntoa(addr));
    return err;
}

    
U32 dnq_net_get_gw_addr(U8 *if_name)
{
    U32 gw;
    struct nlmsghdr *nlMsg;
    struct rtmsg *rtMsg;
    struct _route_info *rtInfo;
    char msgBuf[_BUFSIZE];
    int sock, len, msgSeq = 0;
    struct in_addr addr;

    if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "socket error! errno=%d:%s", errno,strerror(errno));
        return 0;
    }

    memset(msgBuf, 0, _BUFSIZE);

    nlMsg = (struct nlmsghdr *)msgBuf;
    rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);

    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nlMsg->nlmsg_type = RTM_GETROUTE;

    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
    nlMsg->nlmsg_seq = msgSeq++;
    nlMsg->nlmsg_pid = getpid();

    if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "socket send error! errno=%d:%s", errno,strerror(errno));
        close(sock);
        return 0;
    }

    if((len = _readNlSock(sock, msgBuf, msgSeq, getpid())) < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "socket read error! errno=%d:%s", errno,strerror(errno));

        close(sock);
        return 0;
    }
    rtInfo = (struct _route_info *)malloc(sizeof(struct _route_info));
    for(; NLMSG_OK(nlMsg,len); nlMsg = NLMSG_NEXT(nlMsg,len))
    {
        memset(rtInfo, 0, sizeof(struct _route_info));
        _parseRoutes(nlMsg, rtInfo,&gw);
    }
    free(rtInfo);
    close(sock);
    addr.s_addr = gw;
    //DNQ_INFO(DNQ_MOD_NETWORK, "get gateway success: %s", inet_ntoa(addr));
    return (gw);
}

S32 dnq_net_set_mask(U8 *if_name, U32 submask)
{
    int sockfd;
    int ret;
    struct ifreq ifr;
    struct sockaddr_in *sin;
    struct in_addr addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "socket error! errno=%d:%s", errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, if_name);

    //ifr.ifr_ifru.ifru_addr.sin_addr = ipcfg_htonl(submask);
    sin = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_addr;
    sin->sin_addr.s_addr = (submask);
    sin->sin_family = AF_INET;

    ret = ioctl(sockfd, SIOCSIFNETMASK, &ifr);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "ioctl error!, errno=%d:%s", errno, strerror(errno));
        close(sockfd);
        return -1;
    }

    close(sockfd);
    addr.s_addr = submask;
    DNQ_INFO(DNQ_MOD_NETWORK, "set netmask success: %s", inet_ntoa(addr));
    return 0;
}

U32 dnq_net_get_mask(U8 *if_name)
{
    U32 sbmask;
    int sockfd;
    int ret;
    struct ifreq ifr;
    struct sockaddr_in   *sin;
    struct in_addr addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "socket error! errno=%d:%s", errno, strerror(errno));
        return 0;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, if_name);

    ret = ioctl(sockfd, SIOCGIFNETMASK, &ifr);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "ioctl error!, errno=%d:%s", errno, strerror(errno));
        return 0;
    }

    //sbmask = ipcfg_htonl(ifr.ifr_ifru.ifru_addr.sin_addr);
    sin = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_addr;

    sbmask = (sin->sin_addr.s_addr);

    close(sockfd);
    addr.s_addr = sbmask;
    //DNQ_INFO(DNQ_MOD_NETWORK, "get netmask success: %s", inet_ntoa(addr));

    return sbmask;
}

S32 dnq_net_set_broad_addr(U8 *if_name, U32 brdaddr)
{
    int sockfd;
    int ret;
    struct ifreq ifr;
    struct sockaddr_in *sin;
    struct in_addr addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "socket error! errno=%d:%s", errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, if_name);

    //ifr.ifr_ifru.broad_addr.sin_addr = ipcfg_htonl(brdaddr);
    sin = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_broadaddr;
    sin->sin_addr.s_addr = (brdaddr);
    sin->sin_family = AF_INET;

    ret = ioctl(sockfd, SIOCSIFBRDADDR, &ifr);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "ioctl error!, errno=%d:%s", errno, strerror(errno));
        close(sockfd);
        return -1;
    }

    close(sockfd);
    addr.s_addr = brdaddr;
    DNQ_INFO(DNQ_MOD_NETWORK, "set broadcast success: %s", inet_ntoa(addr));

    return 0;
}

U32 dnq_net_get_broad_addr(U8 *if_name)
{
    U32 bdaddr;
    int sockfd;
    int ret;
    struct ifreq ifr;
    struct sockaddr_in *sin;
    struct in_addr addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "socket error! errno=%d:%s", errno, strerror(errno));
        return 0;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, if_name);

    ret = ioctl(sockfd, SIOCGIFBRDADDR, &ifr);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "ioctl error!, errno=%d:%s", errno, strerror(errno));
        close(sockfd);
        return 0;
    }

    //bdaddr = ipcfg_htonl(ifr.ifr_ifru.broad_addr.sin_addr);
    sin = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_broadaddr;
    bdaddr = (sin->sin_addr.s_addr);

    close(sockfd);
    addr.s_addr = bdaddr;
    //DNQ_INFO(DNQ_MOD_NETWORK, "get broadcast success: %s", inet_ntoa(addr));

    return bdaddr;
}

S32 dnq_net_set_ipaddr(U8 *if_name, U32 ip_addr)
{
    int sockfd;
    int ret;
    struct ifreq ifr;
    struct sockaddr_in   *sin;
    struct in_addr addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "socket error! errno=%d:%s", errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, if_name);

    //ifr.ifr_ifru.ifru_addr.sin_addr = ipcfg_htonl(ip_addr);
    sin = (struct sockaddr_in   *)&ifr.ifr_ifru.ifru_addr;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = (ip_addr);

    //IPCFG_DownInterface(if_name);
    ret = ioctl(sockfd, SIOCSIFADDR, &ifr);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "ioctl error!, errno=%d:%s", errno, strerror(errno));
        close(sockfd);
        return -1;
    }
    //IPCFG_UpInterface(if_name);
    close(sockfd);
    addr.s_addr = ip_addr;
    DNQ_INFO(DNQ_MOD_NETWORK, "set ipaddr success: %s", inet_ntoa(addr));

    return 0; //success
}

U32 dnq_net_get_ipaddr(U8 *if_name)
{
    U32 ip_addr;
    int sockfd;
    int ret;
    struct ifreq ifr;
    struct sockaddr_in   *sin;
    struct in_addr addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "socket error! errno=%d:%s", errno, strerror(errno));
        return 0;
    }

    memset(&ifr, 0, sizeof(ifr));
    //strcpy(ifr.ifr_ifrn.ifrn_name, if_name);
    strcpy(ifr.ifr_name, if_name);

    ret = ioctl(sockfd, SIOCGIFADDR, &ifr);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "ioctl error!, errno=%d:%s", errno, strerror(errno));
        close(sockfd);
        return 0;
    }

    //ip_addr = ipcfg_htonl(ifr.ifr_ifru.ifru_addr.sin_addr);
    //sin = (struct sockaddr_in *)$ifr.ifr_addr;
    sin = (struct sockaddr_in   *)&ifr.ifr_ifru.ifru_addr;
    ip_addr = (sin->sin_addr.s_addr);
    close(sockfd);
    addr.s_addr = ip_addr;
    //DNQ_INFO(DNQ_MOD_NETWORK, "get ipaddr success: %s\n", inet_ntoa(addr));

    return ip_addr;
}

S32 dnq_net_set_macaddr(U8 *if_name, U8 *mac_addr)
{
    int sockfd;
    int ret;
    struct ifreq ifr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "socket error! errno=%d:%s", errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, if_name);

    ifr.ifr_ifru.ifru_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(ifr.ifr_ifru.ifru_hwaddr.sa_data, mac_addr, 6);

    dnq_net_ifdown(if_name);
    ret = ioctl(sockfd, SIOCSIFHWADDR, &ifr);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "ioctl error!, errno=%d:%s", errno, strerror(errno));
        close(sockfd);
        return -1;
    }
    dnq_net_ifup(if_name);

    close(sockfd);
    DNQ_INFO(DNQ_MOD_NETWORK, "set mac_addr success: %02X:%02X:%02X:%02X:%02X:%02X",\
        mac_addr[0], mac_addr[1], mac_addr[2],\
        mac_addr[3], mac_addr[4], mac_addr[5]);

    sleep(1); /* fixed ??*/
    return 0;
}

S32 dnq_net_get_macaddr(U8 *if_name, U8 *mac_addr)
{
    int sockfd;
    int ret;
    struct ifreq ifr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "socket error! errno=%d:%s", errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, if_name);

    ret = ioctl(sockfd, SIOCGIFHWADDR, &ifr);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "ioctl error!, errno=%d:%s", errno, strerror(errno));
        close(sockfd);
        return -1;
    }

    mac_addr[0] = ifr.ifr_ifru.ifru_hwaddr.sa_data[0];
    mac_addr[1] = ifr.ifr_ifru.ifru_hwaddr.sa_data[1];
    mac_addr[2] = ifr.ifr_ifru.ifru_hwaddr.sa_data[2];
    mac_addr[3] = ifr.ifr_ifru.ifru_hwaddr.sa_data[3];
    mac_addr[4] = ifr.ifr_ifru.ifru_hwaddr.sa_data[4];
    mac_addr[5] = ifr.ifr_ifru.ifru_hwaddr.sa_data[5];

    //DNQ_INFO(DNQ_MOD_NETWORK, "set mac_addr success: %02X:%02X:%02X:%02X:%02X:%02X",\
    mac_addr[0], mac_addr[1], mac_addr[2],\
    mac_addr[3], mac_addr[4], mac_addr[5]);
    close(sockfd);
    
    return 0;
}

S32 dnq_net_get_link_status(U8 *if_name)
{
    int skfd;
    struct ifreq ifr;
    struct ethtool_value edata;

    edata.cmd = ETHTOOL_GLINK;
    edata.data = 0;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_data = (char *) &edata;

    if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
       DNQ_ERROR(DNQ_MOD_NETWORK, "socket error! errno=%d:%s", errno, strerror(errno));
        return -1;
    }

    if(ioctl(skfd, SIOCETHTOOL, &ifr) == -1)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "ioctl error!, errno=%d:%s", errno, strerror(errno));
        close(skfd);
        return -1;
    }

    close(skfd);
    return edata.data;
}

U32 dnq_net_get_host_by_name(U8 *cname)
{
    struct hostent *p_hostent;
    char **pptr;
    struct in_addr ipaddr;
    struct sockaddr *sa;    /* input */
    socklen_t len;         /* input */
    U8  hbuf[1024], sbuf[12];

    p_hostent = gethostbyname(cname);
    //if (getnameinfo(sa, len, hbuf, sizeof(hbuf), sbuf,\
    //    sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV) == 0)
    //    printf("host=%s, serv=%s\n", hbuf, sbuf);
    
    if(p_hostent == NULL)
    {
        DNQ_DEBUG(DNQ_MOD_NETWORK, "gethost error! cname=%s, errno=%d:%s\n",\
            cname, errno, strerror(errno));
        return 0;
    }
    else
    {
        pptr = p_hostent->h_addr_list;
        ipaddr = *(struct in_addr *)*pptr;
        return (ipaddr.s_addr);
    }
    return 0;
}

S32 dnq_dhcp_start(U8 *if_name)
{
    S32 ret;
    U8 cmd[64];

    sprintf(cmd, "udhcpc -i %s", if_name);

    ret = dnq_system_call(cmd);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "system exec failed! command=\"%s\"", cmd);
        return -1;
    }
    return ret;
}

S32 dnq_dhcp_stop(U8 *if_name)
{
    S32 ret;
    U8 cmd[64] = "killall udhcpc";

    ret = dnq_system_call(cmd);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_NETWORK, "system exec failed! command=\"%s\"", cmd);
        return -1;
    }
    return ret;
}

S32 dnq_get_server_ip(U8 *server_ip)
{
    if(server_ip)
    {
        strcpy(server_ip, g_server_ip);
    }
    return 0;
}

U32 dnq_get_server_port()
{
    return g_server_port;
}

S32 dnq_server_link_isgood(U32 isSaveIp)
{
    U32 server_ip;
    struct in_addr addr;
    
    server_ip = dnq_net_get_ipaddr(ETH_NAME);
    if(server_ip == 0)
        return 0;

    server_ip = dnq_net_get_host_by_name(DNQ_SERVER_URL);
    if(server_ip != 0)
    {
        /* save server ipaddr */
        if(isSaveIp)
        {
            addr.s_addr = server_ip;
            strcpy(g_server_ip, inet_ntoa(addr));
        }  
        return 1;/* got ip */
    }
    
    return 0;
}

S32 dnq_net_link_isgood()
{
    S32 ret;
    U32 server_ip;
    U32 count = 4;
    struct in_addr addr;

    server_ip = dnq_net_get_ipaddr(ETH_NAME);
    if(server_ip == 0)
        return 0;

    ret = dnq_net_get_host_by_name(BAIDU_URL);
    if(ret != 0)
        return 1;
    ret = dnq_net_get_host_by_name(TENCENT_URL);
    if(ret != 0)
        return 1;
    ret = dnq_net_get_host_by_name(TAOBAO_URL);
    if(ret != 0)
        return 1;
    ret = dnq_net_get_host_by_name(YOUKU_URL);
    if(ret != 0)
        return 1;  
    
    return 0;
}

S32 dnq_get_rabbitmq_ipaddr(U8 *url, U8 *ip_addr)
{
    U32 server_ip;
    U32 count = 2;
    struct in_addr addr;

    server_ip = dnq_net_get_host_by_name(url);
    if(server_ip != 0)
    {
        addr.s_addr = server_ip;
        strcpy(ip_addr, inet_ntoa(addr));
        strcpy(g_server_ip, inet_ntoa(addr));
        return 1;/* got ip */
    }   
       
    return 0;
}

static netlink_callback netlink_status_callback = NULL;

void netlink_callback_register(netlink_callback callback)
{
    if(callback)
        netlink_status_callback = callback;
    else
        netlink_status_callback = NULL;
}

static void net_status_change(net_status_e status)
{
    switch(status)
    {
        case LINK_DOWN:
            DNQ_INFO(DNQ_MOD_NETWORK, "link down!");
        break;
        case LINK_UP:
            DNQ_INFO(DNQ_MOD_NETWORK, "link up!");
            //dnq_dhcp_start(ETH_NAME);
        break;
        case IP_REQUEST:
            DNQ_INFO(DNQ_MOD_NETWORK, "ip request!");
        break;
        case IP_BOUND:
            DNQ_INFO(DNQ_MOD_NETWORK, "ip bound!");
        break;
        case IP_LOST:
            DNQ_INFO(DNQ_MOD_NETWORK, "ip lost!");
        break;
        case HOST_ONLINE:
            DNQ_INFO(DNQ_MOD_NETWORK, "host online!");
        break;
        case HOST_OFFLINE:
            DNQ_INFO(DNQ_MOD_NETWORK, "host offline!");
        break;
        default:
            DNQ_ERROR(DNQ_MOD_NETWORK, "link unknown status!");
        break;
    }

    printf("netlink_status_callback=0x%08x\n", netlink_status_callback);
    if(netlink_status_callback)
        netlink_status_callback(status);
    //lcd_net_status_update(status);
}

void *network_task(void *args)
{
    static U32 last_status = NET_INIT;
    U32  current_status;
    S32  ret;

    sleep(3);
    while(1)
    {
        
        ret = dnq_net_get_link_status(ETH_NAME);
        if(ret == 1)
        {
            current_status = LINK_UP;
            ret = dnq_net_get_ipaddr(ETH_NAME);
            if(ret != 0)
            {
                /* have ip */
                current_status = IP_BOUND;
                ret = dnq_server_link_isgood(0);
                if(ret == 1)
                {
                    /* server link is good */
                    current_status = HOST_ONLINE;
                }
                else
                {
                    /* server link is bad */
                    current_status = HOST_OFFLINE;
                }
            }
            else
            {
                /* have no ip */
                current_status = IP_REQUEST;
            }
        }
        else if(ret == 0)
        {
            /* link down */
            current_status = LINK_DOWN;
        }

        if(current_status != last_status)
        {
            net_status_change(current_status);
            last_status = current_status;
        }

        dnq_sleep(2);
    }
}

S32 dnq_network_getinfo()
{
    U32   ip;
    U8    mac[16] = {0};
    struct in_addr addr;

    ip = dnq_net_get_link_status(ETH_NAME);
    DNQ_INFO(DNQ_MOD_NETWORK, "link: %s", ip?"on":"off");
    
    addr.s_addr = dnq_net_get_ipaddr(ETH_NAME);
    DNQ_INFO(DNQ_MOD_NETWORK, "ipaddr: %s", inet_ntoa(addr));
    addr.s_addr = dnq_net_get_mask(ETH_NAME);
    DNQ_INFO(DNQ_MOD_NETWORK, "mask: %s", inet_ntoa(addr));
    addr.s_addr = dnq_net_get_gw_addr(ETH_NAME);
    DNQ_INFO(DNQ_MOD_NETWORK, "gateway: %s", inet_ntoa(addr));
    addr.s_addr = dnq_net_get_broad_addr(ETH_NAME);
    DNQ_INFO(DNQ_MOD_NETWORK, "broadcast: %s", inet_ntoa(addr));
    
    dnq_net_get_macaddr(ETH_NAME, mac);
    DNQ_INFO(DNQ_MOD_NETWORK, "mac: %02x:%02x:%02x:%02x:%02x:%02x",\
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    addr.s_addr = dnq_net_get_host_by_name(DNQ_SERVER_URL);
    DNQ_INFO(DNQ_MOD_NETWORK, "the url: %s to ipaddr: %s",\
        DNQ_SERVER_URL, inet_ntoa(addr));
    return 0;
}

void dnq_network_printinfo()
{
    host_net_info_t *netinfo = &g_host_netinfo;
    DNQ_INFO(DNQ_MOD_NETWORK, "intf name:\t%s", netinfo->if_name);
    DNQ_INFO(DNQ_MOD_NETWORK, "link status:\t%s", netinfo->link_status);
}

S32 dnq_network_check()
{
    S32 link_status;
    U32 ipaddr;
    while(1)
    {
        ipaddr = dnq_net_get_ipaddr(ETH_NAME);
        
        link_status = dnq_net_get_link_status(ETH_NAME);
        if(link_status == LINK_DOWN)
        {
            
        }

        if(link_status == LINK_UP)
        {
            
        }

        dnq_dhcp_start(ETH_NAME);
    }
}

void dnq_get_mac_addr(U8 *mac_addr)
{
    strncpy(mac_addr, g_host_netinfo.mac, 16);
}

U8 *dnq_get_mac_string()
{
    return g_host_netinfo.mac_str;
}

host_net_info_t *dnq_get_netinfo()
{
    return &g_host_netinfo;
}

S32 dnq_netinfo_init()
{
    S32 ret;
    host_net_info_t *netinfo = &g_host_netinfo;
    
    strcpy(netinfo->if_name, ETH_NAME);
    dnq_net_get_macaddr(ETH_NAME, netinfo->mac);
    sprintf(netinfo->mac_str, "%02x%02x%02x%02x%02x%02x", \
        netinfo->mac[0], netinfo->mac[1], netinfo->mac[2],
        netinfo->mac[3], netinfo->mac[4], netinfo->mac[5]);
    netinfo->ipaddr = dnq_net_get_ipaddr(ETH_NAME);
    netinfo->mask = dnq_net_get_mask(ETH_NAME);
    netinfo->gateway = dnq_net_get_gw_addr(ETH_NAME);
    netinfo->dns = dnq_net_get_dns(ETH_NAME);
    dnq_net_get_dns2(&netinfo->dns_ex);
    return 0;
}

S32 dnq_network_init()
{
    dnq_netinfo_init();
    dnq_network_getinfo();
    dnq_task_create("network", 32*2048, network_task, NULL);
    
    return 0;
}

S32 dnq_network_deinit()
{
    return 0;
}

S32 network_test()
{
    dnq_init();
    dnq_debug_init();
    dnq_network_getinfo();
    return 0;
}

