/* GPIO Driver Test/Example Program
 *
 * Compile with:
 *  gcc -s -Wall -Wstrict-prototypes gpio.c -o gpiotest
 *
 *
 * Note :
 *   PORT NAME[PIN] = GPIO [id]	
 *   PORTA[ 0]      = gpio[ 0x00]
 *   PORTA[ 1]      = gpio[ 0x01]	  
 *                  :
 *   PORTA[31]      = gpio[ 0x1F]
 *   PORTB[ 0]      = gpio[ 0x20]
 *                  :
 *   PORTB[31]      = gpio[ 0x3F]
 *                  :
 *                  :
 *                  :
 *   PORTI[ 0]      = gpio[ 0xC0]
 *                  :
 *                  :
 *   PORTI[31]      = gpio[ 0xDF]
 */

#include "dnq_common.h"
#include "dnq_gpio.h"
#include "dnq_log.h"

#define gpio_assert(gpio_port) \
    do\
    {\
        if(gpio_port < 0 || gpio_port > 0xFF)\
        {\
            DNQ_ERROR(DNQ_MOD_GPIO, "gpio_port %d should 0~0xDF!");\
            return -1;\
        }\
    }while(0)

S32 dnq_gpio_open(U32 gpio_port)
{
    FILE *fp = NULL;
    
    gpio_assert(gpio_port);
    
    if ((fp = fopen("/sys/class/gpio/export", "w")) == NULL) 
    {
        DNQ_ERROR(DNQ_MOD_GPIO, "Cannot open export file! err=%s\n", \
            strerror(errno));
		return -1;
	}
	fprintf(fp, "%d", gpio_port);
	fclose(fp);
    
    return 0;
}

S32 dnq_gpio_close(U32 gpio_port)
{
    FILE *fp = NULL;
    
    gpio_assert(gpio_port);
    
    if ((fp = fopen("/sys/class/gpio/unexport", "w")) == NULL) 
    {
        DNQ_ERROR(DNQ_MOD_GPIO, "Cannot open unexport file! err=%s\n", \
            strerror(errno));
		return -1;
	}
	fprintf(fp, "%d", gpio_port);
	fclose(fp);
    
    return 0;
}

S32 dnq_gpio_set_direction(U32 gpio_port, U32 dir)
{
    S32 ret;
    U8  str[128];
    FILE *fp = NULL;
    
    gpio_assert(gpio_port);
    
    sprintf(str,"/sys/class/gpio/gpio%d/direction", gpio_port);
	if ((fp = fopen(str, "rb+")) == NULL) 
    {
		DNQ_ERROR(DNQ_MOD_GPIO, "Cannot open direction file! err=%s\n", \
            strerror(errno));
        return -1;
	}
    if(dir == GPIO_IN)
	    fprintf(fp, "in");
    else
        fprintf(fp, "out");
	fclose(fp);
}

S32 dnq_gpio_get_direction(U32 gpio_port, U32 *dir)
{
    S32 ret;
    U8  str[128];
    U8  buffer[16] = {0};
    FILE *fp = NULL;
    
    gpio_assert(gpio_port);
    
    sprintf(str,"/sys/class/gpio/gpio%d/direction", gpio_port);
	if ((fp = fopen(str, "rb+")) == NULL) 
    {
		DNQ_ERROR(DNQ_MOD_GPIO, "Cannot open direction file! err=%s\n", \
            strerror(errno));
        return -1;
	}
    
    if(fread(buffer, sizeof(char), sizeof(buffer) - 1, fp) < 0)
    {
        DNQ_ERROR(DNQ_MOD_GPIO, "fread direction file error! err=%s\n", \
            strerror(errno));
        fclose(fp);
        return -1;
    }

    *(U32*)dir = -1;
    if(strcmp(buffer, "in") == 0)
        *(U32*)dir = GPIO_IN;
    else if (strcmp(buffer, "out") == 0)
        *(U32*)dir = GPIO_OUT;

	fclose(fp);
    return 0;
}

S32 dnq_gpio_read_bit(U32 gpio_port, U32 *value)
{
    S32 ret;
    U8  str[128];
    U8  buffer[16] = {0};
    FILE *fp = NULL;
    
    gpio_assert(gpio_port);
    
    sprintf(str,"/sys/class/gpio/gpio%d/value", gpio_port);
	if ((fp = fopen(str, "rb+")) == NULL) 
    {
		DNQ_ERROR(DNQ_MOD_GPIO, "Cannot open value file! err=%s\n", \
            strerror(errno));
        return -1;
	}
    if(fread(buffer, sizeof(char), sizeof(buffer) - 1, fp) < 0)
    {
        DNQ_ERROR(DNQ_MOD_GPIO, "fread value file error! err=%s\n", \
            strerror(errno));
        fclose(fp);
        return -1;
    }

    *value = atoi(buffer);
    fclose(fp);
    return 0;
}

S32 dnq_gpio_write_bit(U32 gpio_port, U32 value)
{
    S32 ret;
    U8  str[128];
    U8  buffer[16] = {0};
    FILE *fp = NULL;
    
    gpio_assert(gpio_port);
    
    sprintf(str,"/sys/class/gpio/gpio%d/value", gpio_port);
	if ((fp = fopen(str, "rb+")) == NULL) 
    {
		DNQ_ERROR(DNQ_MOD_GPIO, "Cannot open value file! err=%s\n", \
            strerror(errno));
        return -1;
	}

    if(value == 0)
        sprintf(buffer, "%d", 0);
    else
        sprintf(buffer, "%d", 1);
    if(fwrite(buffer, sizeof(char), 1, fp) < 0)
    {
        DNQ_ERROR(DNQ_MOD_GPIO, "fwrite value file error! err=%s\n", \
            strerror(errno));
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}


FILE *fp;
char io[]={'A','B','C','D','E','F','G','H','I'};
char str[256];
int dnq_gpio_ctrl_enable(void)
{
	int num=0x47;	
	//linux equivalent code "echo 130 > export" to export the port 
	if ((fp = fopen("/sys/class/gpio/export", "w")) == NULL) {
		printf("Cannot open export file.\n");
		exit(1);
	}
	fprintf(fp, "%d", num);
	fclose(fp);
	//  linux equivalent code "echo in > direction" to set the port as an input  
	sprintf(str,"/sys/class/gpio/gpio%d/direction",num);
	if ((fp = fopen(str, "rb+")) == NULL) {
		printf("Cannot open direction file.\n");
		exit(1);
	}
	fprintf(fp, "in");
	fclose(fp);

	// **here comes where I have the problem, reading the value**    
	char buffer[10];
	int value;
	sprintf(str,"/sys/class/gpio/gpio%d/value",num);
	while (1) {		
		if ((fp = fopen(str, "rb")) == NULL) {
			printf("Cannot open value file.\n");
		} else {
			fread(buffer, sizeof(char), sizeof(buffer) - 1, fp);
			value = atoi(buffer);
			printf("GPIO%C%d value: %d\n",io[num/0x20],num%(0x20),value);
			fclose(fp);
		}
	}

	return 0;
}

