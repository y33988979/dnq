#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <fcntl.h>

#define DEVICE     "/dev/mtdblock2"

int main(int argc, char **argv)
{
    int i, nread;
    char buffer[20480] = {0};
    int fd;
    unsigned int offset = 0;
    unsigned int len = 0x100;
    
    if(argc > 1)
    {
        offset = strtol(argv[1], NULL, 16);
    }
    if(argc > 2)
    {
        len = strtol(argv[2], NULL, 16);
    }
    printf("read %s: offset= 0x%x, len=0x%x\n", DEVICE, offset, len);
    if(offset < 0 || len < 0)
    {
        printf("read:  offset or len is error!\n");
        return -1;
    }
    

    fd = open(DEVICE, O_RDWR);
    if(fd < 0)
    {
        perror("open error");
        return -1;
    }
    
    if(lseek(fd, offset, SEEK_SET) < 0)
    {
        perror("lseek error");
        return -1;
    }

    
    nread = read(fd, buffer, len);
    if(nread < 0)
    {
        perror("read error");
        return -1;
    }

    printf("read len = %d\n", nread);
    for(i=0; i<nread; i+=4)
    {
        if(i % 16 == 0)
            printf("\n0x%08x: ", (int)(offset+i));
        
        printf("%08x ", *((int*)&buffer[i]));
    }
    printf("\n");

    close(fd);
    return 0;
}
