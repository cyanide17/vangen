#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");

void my_add(void){
    printk("callee add\n");
}

static int __init my_init(void){
    printk(KERN_ALERT "callee inserted!\n");
    my_add();
    return 0;
}
static void __exit my_exit(void){
    printk(KERN_ALERT "callee removed!\n");
}



module_init(my_init);
module_exit(my_exit);
