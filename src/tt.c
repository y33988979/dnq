
#include <stdio.h>
#include <string.h>

int main()
{

    char buffer[64] = "1234567890";

    printf("buffer: %s\n", buffer);
    //strlcpy(buffer, "444444", 5);

    printf("buffer: %s\n", buffer);

    int n = 16*1024*1024;

    printf("sizeof(int)=%d, n====%d\n",sizeof(int), n);
    n = 0x7fffffff;
    printf("n===%d\n", n);
}
