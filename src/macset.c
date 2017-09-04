
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <regex.h>

int isValidMac(char *value)
{
    int r; //r=0:valid, else not valid
    char *reg="^[0-9A-Fa-f]\\([0-9A-Fa-f]\\:[0-9A-Fa-f]\\)\\{5\\}[0-9A-Fa-f]$";
    //char *reg="^([0-9a-fA-F]{2})(([/\s:][0-9a-fA-F]{2}){5})$";
    r=ereg(reg,value);
    return r;
}

int isValidIp(char *value)
{
    int r; //r=0:valid, else not valid
    char *reg="^[0-9]\\{1,3\\}\\.[0-9]\\{1,3\\}\\.[0-9]\\{1,3\\}\\.[0-9]\\{1,3\\}$";
    r=ereg(reg,value);
    return r;
}

int ereg(char *pattern, char *value)
{
    int r,cflags=0;
    regmatch_t pm[10];
    const size_t nmatch = 10;
    regex_t reg;

    r=regcomp(&reg, pattern, cflags);
    if(r==0){
        r=regexec(&reg, value, nmatch, pm, cflags);
    }

    regfree(&reg);
    return r;
}

int main(int argc, char **argv)
{
    int r;
    char mac[128] = "AA:DD:EE:BB:11:33";
    char ip[128] = "1988.168.3.1";
    char cmd[128] = {0};

    if(argc == 1)
    {
        printf("Usage: ./macset [AA:BB:CC:DD:EE:FF]\n");
        return -1;
    }
    if(argc == 2)
    {
        strcpy(mac, argv[1]);
    }
    if(argc == 3)
    {
        strcpy(mac, argv[1]);
        strcpy(ip, argv[2]);
    }

    //r = isValidIp(ip);
    //printf("ip is %s, r=%d\n",ip, r);
    r = isValidMac(mac);

    //sprintf(cmd, "ifconfig eth0 hw ether %s", mac);
    sprintf(cmd, "sed -i 's/.*ifconfig eth0 hw.*$/ifconfig eth0 hw ether %s/g' /etc/init.d/rcS", mac);
    if(r == 0)
    {
        r = system(cmd);
        if(r == -1)
        {
            printf("cmd: \"%s\"\n", cmd);
            printf("command exec failed! WEXITSTATUS=%d\n", WEXITSTATUS(r));
            return -1;
        }
        else
        {
	        if (WIFEXITED(r) && 0 == (WEXITSTATUS(r)))
            {
                system("cat /etc/init.d/rcS");
                printf("\nnew mac[%s]\n", mac);
                return 0;
            }
            else
            {
                printf(" ERROR:  WEXITSTATUS=%d\n", WEXITSTATUS(r));
                return -1;
            }
        }
    }
    else
    {
        printf("the mac[%s] is bad! please check! ret=%d\n", mac, r);
    }

    return 0;
}

