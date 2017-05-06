
#include <stdio.h>
#include <string.h>

int main()
{

    char buffer[64] = "1234567890";

    printf("buffer: %s\n", buffer);
    strlcpy(buffer, "444444", 5);

    printf("buffer: %s\n", buffer);
}
