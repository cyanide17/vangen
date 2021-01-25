#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/list.h>

#define MMIO 0
#define PMIO 1

static LIST_HEAD(dev_list_head);

struct dev_cfg{
    unsigned int        vendor;
    unsigned int        device;
    resource_size_t     BAR[6];
    int                 BARtype[6]; // 0:MMIO 1:PMIO
    unsigned long       BARsize[6];
    void __iomem*       virtBAR[6];
    int                 BAR_isvalid[6];
};

struct dev{
    struct list_head    node;
    struct dev_cfg      cfg;
};


struct dev* check_dev_list(unsigned int vendor, unsigned int device)
{
    struct dev *dev;
    list_for_each_entry(dev, &dev_list_head, node) {
        if(dev->cfg.vendor == vendor &&
           dev->cfg.device == device) {
            return dev;
        }
    }
    return NULL;
}

void print_dev_list(void)
{
    struct dev *dev;
    int i;
    printk(KERN_INFO "printing all dev\n");
    list_for_each_entry(dev, &dev_list_head, node) {
        printk(KERN_INFO "== (%x:%x) ==\n",dev->cfg.vendor,dev->cfg.device);
        for(i=0;i<6;i++){
            if(dev->cfg.BAR_isvalid[i]==true){
                printk(KERN_INFO "| BAR[%d]=%x\n",i,dev->cfg.BAR[i]);
                if(dev->cfg.BARtype[i]==MMIO){
                    printk(KERN_INFO "| type:MMIO\n");
                    printk(KERN_INFO "| virtBAR[%d]=%p\n",i,dev->cfg.virtBAR[i]);
                }else{
                    printk(KERN_INFO "| type:PMIO\n");
                }
                printk(KERN_INFO "| BARsize[%d]=%lx\n",i,dev->cfg.BARsize[i]);
                printk(KERN_INFO "|-\n");
            }
        }
    }
}

void add_dev_list(struct dev* dev,int bar_idx)
{
    struct dev *new_dev;
    if(dev->cfg.BARtype[bar_idx] == MMIO){
        printk("adding (%x,%x), MMIO BAR[%d] = 0x%x with size 0x%lx\n",dev->cfg.vendor,dev->cfg.device,bar_idx,dev->cfg.BAR[bar_idx],dev->cfg.BARsize[bar_idx]);
    }else{
        printk("adding (%x,%x), PMIO BAR[%d] = 0x%x with size 0x%lx\n",dev->cfg.vendor,dev->cfg.device,bar_idx,dev->cfg.BAR[bar_idx],dev->cfg.BARsize[bar_idx]);
    }

    new_dev = check_dev_list(dev->cfg.vendor, dev->cfg.device);
    if(new_dev == NULL) {
        new_dev = (struct dev*)kzalloc(sizeof(struct dev), GFP_KERNEL);
        if(!new_dev){
            printk(KERN_ERR "memory allocation failed!\n");
            return;
        }
        new_dev->cfg.vendor = dev->cfg.vendor;
        new_dev->cfg.device = dev->cfg.device;
        INIT_LIST_HEAD(&new_dev->node);
        list_add_tail(&new_dev->node, &dev_list_head);
    }

    new_dev->cfg.BAR[bar_idx] = dev->cfg.BAR[bar_idx];
    new_dev->cfg.BARsize[bar_idx] = dev->cfg.BARsize[bar_idx];
    if(dev->cfg.BARtype[bar_idx] == MMIO){
        new_dev->cfg.virtBAR[bar_idx] = ioremap(dev->cfg.BAR[bar_idx],dev->cfg.BARsize[bar_idx]);
        new_dev->cfg.BARtype[bar_idx] = MMIO;
    }else{
        new_dev->cfg.BARtype[bar_idx] = PMIO;
    }
    new_dev->cfg.BAR_isvalid[bar_idx] = true;

}

void remove_dev_list(void)
{
    struct dev *dev;
    struct list_head *node;
    struct list_head *nnode;
    list_for_each_safe(node, nnode, &dev_list_head) {
        dev = list_entry(node,struct dev,node);
        printk(KERN_INFO "removing (%x,%x)\n",dev->cfg.vendor,dev->cfg.device);
        list_del(node);
        kfree(dev);
    }
}

void enum_dev(void){
    unsigned int pcicfg_idx;
    int b,d,f; // bus, device, function
    int bar_idx;
    unsigned int tmpval;
    struct dev *dev;

    // struct dev initialization
    dev = (struct dev*)kzalloc(sizeof(struct dev), GFP_KERNEL);
    if(!dev){
        printk(KERN_ERR "memory allocation failed!\n");
        return;
    }

    // scan all pci configuration space
    for(b=0;b<256;b++){
        for(d=0;d<32;d++){
            for(f=0;f<8;f++){
                pcicfg_idx = 0x80000000|b<<16|d<<11|f<<8; /* set pci config address */

                outl(pcicfg_idx,0xCF8);
                dev->cfg.vendor = inl(0xCFC)&0xFFFF; /* read vendor id */
                dev->cfg.device = (inl(0xCFC)>>16)&0xFFFF; /* read device id */

                if(dev->cfg.device==0xFFFF) /* check device existence */
                    break;
                outl(pcicfg_idx+0x0C,0xCf8);
                if(((inl(0xCFC)>>16)&0xFF)==0x00){ /* check header type (endpoint device only) */

                    // reading BARs
                    for(bar_idx=0;bar_idx<6;bar_idx++){
                        outl(pcicfg_idx+0x10+4*bar_idx,0xCF8);
                        if(inl(0xCFC)!=0){ /* if valid BAR */
                            tmpval = inl(0xCFC);

                            // 1. address space size
                            outl(~0x0,0xCFC); /* write 0xFs*/
                            dev->cfg.BARsize[bar_idx] = inl(0xCFC);
                            outl(tmpval,0xCFC); /* restore original BAR */
                            dev->cfg.BARsize[bar_idx] = (~(dev->cfg.BARsize[bar_idx]&(~0xF)))+1; /* calculating size */

                            // 2. base address & type
                            if((tmpval&0x1)==0){
                                dev->cfg.BAR[bar_idx] = tmpval&(~0xF);
                                dev->cfg.BARtype[bar_idx] = MMIO;
                            }else{
                                dev->cfg.BAR[bar_idx] = tmpval&(~0x3);
                                dev->cfg.BARtype[bar_idx] = PMIO;
                            }

                            add_dev_list(dev,bar_idx);
                        }
                    }
                }
            }
        }
    }
    kfree(dev);
}

void print_pci_config(int bus,int dev,int func){
    unsigned int pcicfg_idx;
    int b,d,f;
    int i,j,val;
    unsigned int regi;

    b = bus;
    d = dev;
    f = func;

    pcicfg_idx = 0x80000000+(b*0x100000+d*0x800+f*0x100);

    printk("| ============\n");
    printk("| %d:%d:%d \n",b,d,f);

    for(i=0;i<64;i+=4){
        outl(pcicfg_idx+i,0xCF8);
        regi = inl(0xCFC);
        printk(KERN_CONT "| ");

        if(i<0x10)
            printk(KERN_CONT "0");
        printk(KERN_CONT "%x: ",i);

        for(j=3;j>=0;j--){
            val = (regi>>(8*j))&0xFF;

            if(val<0x10)
                printk(KERN_CONT "0");
            printk(KERN_CONT "%x ",val);
        }
        printk(KERN_CONT "\n");
    }

}

void print_IF(void){
    int IF = -1;
    IF = (0x200&native_save_fl())>>9;
    printk("IF:%d\n",IF);
}




static int __init init_enum(void){
    printk(KERN_ALERT "mod module inserted!\n");
    enum_dev();
    print_dev_list();
    return 0;
}

static void __exit exit_enum(void){
    printk(KERN_ALERT "mod module exited!\n");
    remove_dev_list();
}

module_init(init_enum);
module_exit(exit_enum);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("skim");
