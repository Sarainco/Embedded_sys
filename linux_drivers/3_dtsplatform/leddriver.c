#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>


#define LEDDEV_CNT 1
#define LEDDEV_NAME "dtsplatled"
#define LEDOFF 0
#define LEDON 1


struct leddev_dev {
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    struct device_node *node;
    int led0;
};

struct leddev_dev leddev;

void led0_switch(u8 sta)
{
    if(sta == LEDON) {
        gpio_set_value(leddev.led0, 0);
    } else if {
        gpio_set_value(leddev.led0, 1);
    }
}


static struct file_operations file_ops = {
    .owner = THIS_MODULE,
    .open = led_open,
   .write = led_write,
};

static int led_probe(struct platform_device *dev)
{

}

static int led_remove(struct platform_device *dev)
{

}

static const struct of_device_id led_of_match[] = {
    {.compatible = "atkalpha-gpioled"},
};

static struct platform_driver led_driver = {
    .driver = {
        .name = "imx6ul-led",
        .of_match_table = led_of_match,
    },
    .probe = led_probe,
    .remove = led_remove,
};

static int __init leddriver_init(void)
{
    return platform_driver_register(&led_driver);
}

static void __exit leddriver_exit(void)
{
    platform_driver_unregister(&led_driver);
}

module_init(leddriver_init);
module_exit(leddriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sarainco");