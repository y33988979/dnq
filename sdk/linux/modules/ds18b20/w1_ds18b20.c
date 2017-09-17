/*
 * w1_ds2431.c - w1 family 2d (DS2431) driver
 *
 * Copyright (c) 2008 Bernhard Weirich <bernhard.weirich@riedel.net>
 *
 * Heavily inspired by w1_DS2433 driver from Ben Gardner <bgardner@wabtec.com>
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2. See the file COPYING for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/delay.h>

#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <asm/mach-types.h>

#include <mach/regs-serial.h>
#include <mach/irqs.h>
#include <mach/regs-gcr.h>
#include <mach/regs-aic.h>

#include <mach/map.h>
#include <mach/regs-lcd.h>
#include <mach/gpio.h>

 //add by ychen at 20170908
 #include <linux/w1-gpio.h>
 #include <linux/gpio.h>
 #include <mach/regs-gpio.h>

#include "../w1.h"
#include "../w1_int.h"
#include "../w1_family.h"


static int gpio = NUC970_PH15;
module_param(gpio, int, S_IRUGO);

/******************************************************
* add by ychen at 20170908
* w1 wire for ds18b20
*******************************************************/
static void w1_enable_external_pullup(int enable)
{
    unsigned int val;
    val = __raw_readl(REG_GPIOH_PUEN);
    printk("[ychen]: gpioH_15 pullup start! regval=0x%08x\n", val);

    if(enable)
        __raw_writel(val | 0x8000, REG_GPIOH_PUEN);
    else
        __raw_writel(val & ~0x8000, REG_GPIOH_PUEN);

    val = __raw_readl(REG_GPIOH_PUEN);
    printk("[ychen]: gpioH_15 pullup end! regval=0x%08x\n", val);
}

static struct w1_gpio_platform_data ds18b20_gpio_data = {
    .pin = NUC970_PH15,
    .is_open_drain = 0,
    .ext_pullup_enable_pin = -EINVAL,
    //.enable_external_pullup = NULL,
};

static struct platform_device ds18b20_device = {
    .name = "w1-gpio",
    .id = -1,
    .dev = {
        .platform_data = &ds18b20_gpio_data,      
    },
};
/******************************************************/

static int __init nuc970_w1_init(void)
{
    int ret = -1;
    
    //gpio_request();
    //gpio_direction_output(ds18b20_gpio_data.pin, 1);
    //gpio_free(ds18b20_gpio_data.pin);
    ds18b20_gpio_data.pin = gpio;
    ret = platform_device_register(&ds18b20_device);
    printk("[ychen]: ds18b20 init, use gpio[%d]\n", ds18b20_gpio_data.pin);
    return ret;
}

static void __exit nuc970_w1_exit(void)
{
    //gpio_direction_output(ds18b20_gpio_data.pin, 1);
    platform_device_unregister(&ds18b20_device);
    printk("[ychen]: ds18b20 exit... \n");

}


module_init(nuc970_w1_init);
module_exit(nuc970_w1_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuchen <yuchen@jiuzhoutech.com>");
MODULE_DESCRIPTION("w1 family driver for DS18B20, temp sensor");
