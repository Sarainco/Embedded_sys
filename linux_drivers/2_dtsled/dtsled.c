#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
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


#define DTSLED_CNT 1
#define DTSLED_NAME "dtsled"

#define LEDON 1
#define LEDOFF 0

static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

struct dtsled_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
};

struct dtsled_dev dtsled;

void led_switch(u8 sta)
{
    u32 val = 0;
    if(sta == LEDON) {
        val = readl(GPIO1_DR);
        val &= ~(1 << 3);
        writel(val, GPIO1_DR);
    } else if(sta == LEDOFF) {
        val = readl(GPIO1_DR);
        val |= (1 << 3);
        writel(val, GPIO1_DR);
    }

}

static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{

    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char ledstat;

    retvalue = copy_from_user(databuf, buf, cnt);
    if (retvalue < 0) {
        printk("copy_from_user failed\n");
        return -EFAULT;
    }
    ledstat = databuf[0];

    if(ledstat == LEDON) {
        led_switch(LEDON);
    } else if(ledstat == LEDOFF) { 
        led_switch(LEDOFF);
    }


    return 0;
}

static int led_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &dtsled;

    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{

    return 0;
}


static struct file_operations dtsled_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
};

static int __init led_init(void)
{
    u32 val = 0;
    int ret;
    u32 regdata[14];
    const char *str;
    struct property *proper;

    dtsled.nd = of_find_node_by_path("/alphaled");
    if(dtsled.nd == NULL) {
        printk("dtsled node not found\n");
        return -ENODEV;
    } else {
        printk("dtsled node found\n");
    }

    proper = of_find_property(dtsled.nd, "compatible", NULL);
    if(proper == NULL) {
        printk("compatible property not found\n");
        return -ENOENT;
    } else {
        //str = (const char *)proper->value;
        printk("compatible: %s\n", (char *)proper->value);
    }

    ret = of_property_read_string(dtsled.nd, "status", &str);
    if (ret < 0) {
        printk("status property not found\n");
        //return -ENOENT;
    } else {
        printk("status: %s\n", str);
    }

    ret = of_property_read_u32_array(dtsled.nd, "reg", regdata, 10);
    if (ret < 0) {
        printk("reg property not found\n");
    } else {
        int i = 0;
        for(i = 0; i < 10; i++) {
            printk("reg[%d]: 0x%x\n", i, regdata[i]);
        }
    }

    /* 初始化 LED */
    #if 0
    /* 1、寄存器地址映射 */
    IMX6U_CCM_CCGR1 = ioremap(regdata[0], regdata[1]);
    SW_MUX_GPIO1_IO03 = ioremap(regdata[2], regdata[3]);
    SW_PAD_GPIO1_IO03 = ioremap(regdata[4], regdata[5]);
    GPIO1_DR = ioremap(regdata[6], regdata[7]);
    GPIO1_GDIR = ioremap(regdata[8], regdata[9]);
    #else
    IMX6U_CCM_CCGR1 = of_iomap(dtsled.nd, 0);
    SW_MUX_GPIO1_IO03 = of_iomap(dtsled.nd, 1);
    SW_PAD_GPIO1_IO03 = of_iomap(dtsled.nd, 2);
    GPIO1_DR = of_iomap(dtsled.nd, 3);
    GPIO1_GDIR = of_iomap(dtsled.nd, 4);
    #endif

    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3 << 26);
    val |= (3 << 26); // 011 0000000000000000000000000000
    writel(val, IMX6U_CCM_CCGR1);

    writel(5, SW_MUX_GPIO1_IO03);

    writel(0x10b0, SW_PAD_GPIO1_IO03);

    val = readl(GPIO1_GDIR);
    val &= ~(1 << 3);
    val |= (1 << 3);
    writel(val, GPIO1_GDIR);

    /* 2、LED 初始为关闭 */  
    val = readl(GPIO1_DR);
    val |= (1 << 3);
    writel(val, GPIO1_DR);

    if(dtsled.major) {
        dtsled.devid = MKDEV(dtsled.major, 0);
        register_chrdev_region(dtsled.devid, DTSLED_CNT, DTSLED_NAME);
    } else {
        alloc_chrdev_region(&dtsled.devid, 0, DTSLED_CNT, DTSLED_NAME);
        dtsled.major = MAJOR(dtsled.devid);
        dtsled.minor = MINOR(dtsled.devid);
    }

    printk("dtsled major: %d minor: %d\r\n", dtsled.major, dtsled.minor);

    dtsled.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev, &dtsled_fops);

    cdev_add(&dtsled.cdev, dtsled.devid, DTSLED_CNT);

    dtsled.class = class_create(THIS_MODULE, DTSLED_NAME);
    if (IS_ERR(dtsled.class)) {
        printk("dtsled class create failed\n");
        return PTR_ERR(dtsled.class);
    }

    dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, DTSLED_NAME);
    if (IS_ERR(dtsled.device)) {
        printk("dtsled device create failed\n");
        //class_destroy(dtsled.class);
        return PTR_ERR(dtsled.device);
    }

    return 0;
}

static void __exit led_exit(void)
{
    /* 取消映射 */
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

    cdev_del(&dtsled.cdev);
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT);

    device_destroy(dtsled.class, dtsled.devid);
    class_destroy(dtsled.class);
    printk("dtsled exit\n");

}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sarainco");