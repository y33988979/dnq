

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ipc.h>

#include <netinet/in.h>   
#include <netdb.h> 

#include <linux/route.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/tcp.h>
#include <linux/ethtool.h>

#include <fcntl.h>
#include <errno.h>

typedef unsigned int U32;
typedef unsigned short U16;
typedef unsigned char U8;


int dnq_sock_create(int isBlock)
{
    int fd = -1;
    int sock_flag ;
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        printf("socket error! errno=%d:%s\n", errno, strerror(errno));
        return -1;
    }

    if(isBlock){
        sock_flag = fcntl(fd, F_GETFL,0);  
        fcntl(fd,F_SETFL, sock_flag|O_NONBLOCK);  
    }

    return fd;
}

int dnq_sock_close(int fd)
{
    return close(fd);
}

int dnq_sock_connect(int fd, char *ip, int port)
{
    struct sockaddr_in saddr;
    int socklen;
    int ret;

    saddr.sin_family = AF_INET;
    //inet_pton(AF_INET, ip, &saddr.sin_addr);
    saddr.sin_addr.s_addr = inet_addr(ip);
    saddr.sin_port = htons(port);

    ret = connect(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr));
    if(ret < 0) {
        //printf("connect error! errno=%d:%s\n", errno, strerror(errno));
        return -1;
    }
    
    return ret;
}

int dnq_scan_ip_port(int fd, char *ip, int mask, int port)
{
    int i;
    int ret;
    char ipaddr[64];

    //strcpy(ipaddr, "192.168.30.168");

    for(i=1; i<254; i++) {
        //printf("ip=%u.%u.%u.%u\n", \
          //  (U32)ip[3]&0xFF, (U32)ip[2]&0xFF, (U32)ip[1]&0xFF, (U32)ip[0]&0xFF);
        fd = dnq_sock_create(0);
        if(fd < 0) {
            return -1;
        }
        sprintf(ipaddr, "%d.%d.%d.%d", \
            (U32)ip[3]&0xFF, (U32)ip[2]&0xFF, (U32)ip[1]&0xFF, (U32)i&0xFF);
        //usleep(500*1000);
        ret = dnq_sock_connect(fd, ipaddr, port);
        
        dnq_sock_close(fd);
        printf("scan ip[%s] port[%d] is %s!\n", ipaddr, port, ret?"Close":"Open");
    }
    return 0;
}

int main()
{
    int fd;
    int ipaddr;
    int port;
        
    fd = dnq_sock_create(1);
    if(fd < 0)
        return -1;

    ipaddr = 0xC0A81E02;
    port = 80;
    
    dnq_scan_ip_port(fd, (char*)&ipaddr, 0, port);

    dnq_sock_close(fd);
    return 0;
}


