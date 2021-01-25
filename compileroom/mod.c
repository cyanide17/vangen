#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/list.h>

#include <linux/pci.h>
#include <linux/pnp.h>
#include <linux/isapnp.h>

#define MMIO 0
#define PMIO 1

#define INFO 0
#define ERR 1

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

int add_dev_list(struct dev* dev,int bar_idx)
{
        struct dev *new_dev;
        if(dev->cfg.BARtype[bar_idx] == MMIO){
                printk("###| [%s] adding (%x,%x), MMIO BAR[%d] = 0x%x with size 0x%lx\n",__FUNCTION__,
                                dev->cfg.vendor,dev->cfg.device,bar_idx,dev->cfg.BAR[bar_idx],dev->cfg.BARsize[bar_idx]);
        }else{
                printk("###| [%s]  adding (%x,%x), PMIO BAR[%d] = 0x%x with size 0x%lx\n",__FUNCTION__,
                                dev->cfg.vendor,dev->cfg.device,bar_idx,dev->cfg.BAR[bar_idx],dev->cfg.BARsize[bar_idx]);
        }

        new_dev = check_dev_list(dev->cfg.vendor, dev->cfg.device);
        if(new_dev == NULL) {
                new_dev = (struct dev*)kzalloc(sizeof(struct dev), GFP_KERNEL);
                if(!new_dev){
                        printk(KERN_ERR "###| [ERR ] memory allocation failed!\n");
                        return 1;
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
        return 0;

}

void remove_dev_list(void)
{
        struct dev *dev;
        struct list_head *node;
        struct list_head *nnode;
        int i;
        list_for_each_safe(node, nnode, &dev_list_head) {
                dev = list_entry(node,struct dev,node);
                printk(KERN_INFO "###| [%s] removing (%x,%x)\n",__FUNCTION__,dev->cfg.vendor,dev->cfg.device);
                for(i=0;i<6;i++){
                        if((dev->cfg.BAR_isvalid[i]==true)&&(dev->cfg.BARtype[i]==MMIO)){
                                printk(KERN_INFO "###| [%s] unmapping virtBAR[%d]:%p\n",__FUNCTION__,i,dev->cfg.virtBAR[i]);
                                iounmap(dev->cfg.virtBAR[i]);
                        }
                }
                list_del(node);
                kfree(dev);
        }

}

int enum_dev(void)
{
        unsigned int pcicfg_idx;
        int b,d,f; // bus, device, function
        int bar_idx;
        unsigned int tmpval;
        struct dev *dev;

        // struct dev initialization
        dev = (struct dev*)kzalloc(sizeof(struct dev), GFP_KERNEL);
        if(!dev){
                printk(KERN_ERR "memory allocation failed!\n");
                return 1;
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

                                                        if(add_dev_list(dev,bar_idx))
                                                                return 1;
                                                }
                                        }
                                }
                        }
                }
        }

        kfree(dev);
        return 0;
}

void print_pci_config(int bus,int dev,int func)
{
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

void print_IF(void)
{
        int IF = -1;
        IF = (0x200&native_save_fl())>>9;
        printk("###| [%s] IF:%d\n",__FUNCTION__,IF);
}

int repro(void)
{
        void __iomem* bar_addr[6];
        struct dev *dev;
        unsigned int vendor;
        unsigned int device;
        vendor = -1;
        device = -1;
        /*
         * vendor = ~
         * device = ~
         */
        // 

vendor = 0x1022;
device = 0x2020;


        if((vendor!=-1)&&(device!=-1)){
                dev = check_dev_list(vendor,device);
                if(dev==NULL){
                        return 1;
                        printk(KERN_ERR "[ERR ] no device\n");
                }
                memcpy(bar_addr,dev->cfg.virtBAR,sizeof(bar_addr));
        }

        /*
         * in, out operations
         */
        // 
outb(0x1,bar_addr[0]+0xc);
outb(0xc0,bar_addr[0]+0x8);
outb(0x7a,bar_addr[0]+0x8);
outb(0xdf,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x24,bar_addr[0]+0x8);
outb(0x24,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x60,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x18);
outb(0x4,bar_addr[0]+0x40);
outb(0x80,bar_addr[0]+0x40);
outb(0x24,bar_addr[0]+0x0);
outb(0x96,bar_addr[0]+0x4);
outb(0xa3,bar_addr[0]+0x38);
outl(0x767d3ca4,bar_addr[0]+0x44);
outl(0xdc27559,bar_addr[0]+0x48);
outb(0x48,bar_addr[0]+0xc);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x1c);
outb(0xe9,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
outb(0x80,bar_addr[0]+0x40);
outb(0x82,bar_addr[0]+0x40);
inb(bar_addr[0]+0x1c);
outb(0x3,bar_addr[0]+0xc);
outb(0x25,bar_addr[0]+0xc);
outb(0x5a,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x8);
inb(bar_addr[0]+0x8);
outb(0x7f,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);

outb(0xc,bar_addr[0]+0x10);
outb(0x1b,bar_addr[0]+0x8);
outb(0xed,bar_addr[0]+0x8);
outb(0x50,bar_addr[0]+0x8);
outb(0x75,bar_addr[0]+0x8);
outb(0x51,bar_addr[0]+0x8);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x18);
outb(0xfd,bar_addr[0]+0x40);
outb(0xc0,bar_addr[0]+0x40);
outb(0x24,bar_addr[0]+0x0);
outb(0xa2,bar_addr[0]+0x4);
outb(0x2,bar_addr[0]+0x38);
outl(0x60024,bar_addr[0]+0x44);
outl(0x1f97e024,bar_addr[0]+0x48);
outb(0x80,bar_addr[0]+0xc);
outb(0xa7,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x1c);
outb(0xb6,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
outb(0x3,bar_addr[0]+0x40);
outb(0xd5,bar_addr[0]+0x40);
outb(0xb1,bar_addr[0]+0xc);
outb(0x41,bar_addr[0]+0xc);
outb(0x11,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x8);
inb(bar_addr[0]+0x8);
outb(0x2,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x64,bar_addr[0]+0xc);

outb(0x1,bar_addr[0]+0xc);
outb(0xba,bar_addr[0]+0x8);
outb(0x51,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x18);
outb(0x46,bar_addr[0]+0x40);
outb(0x1b,bar_addr[0]+0xc);
outb(0xb6,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x8,bar_addr[0]+0x8);
outb(0xa,bar_addr[0]+0x8);
outb(0x4,bar_addr[0]+0x8);
outb(0x4,bar_addr[0]+0x8);
outb(0x30,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x2,bar_addr[0]+0x40);
outb(0x80,bar_addr[0]+0x40);
outb(0xd7,bar_addr[0]+0x0);
outb(0x2,bar_addr[0]+0x4);
outb(0x8,bar_addr[0]+0x38);
outl(0x700024,bar_addr[0]+0x44);
outl(0x1f76e024,bar_addr[0]+0x48);
outb(0xdd,bar_addr[0]+0xc);
outb(0xd1,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x1c);
outb(0x6f,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
outb(0xd6,bar_addr[0]+0x40);
outb(0x0,bar_addr[0]+0x40);
inb(bar_addr[0]+0x1c);
outb(0x60,bar_addr[0]+0xc);
outb(0x81,bar_addr[0]+0xc);
outb(0x3b,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x8);
inb(bar_addr[0]+0x8);
outb(0x32,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x0,bar_addr[0]+0xc);

outb(0xa4,bar_addr[0]+0x10);
outb(0x9,bar_addr[0]+0xc);
outb(0xc0,bar_addr[0]+0x8);
outb(0x51,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x18);
outb(0x80,bar_addr[0]+0x40);
outb(0x9,bar_addr[0]+0xc);
outb(0x2d,bar_addr[0]+0x8);
outb(0x10,bar_addr[0]+0x8);
outb(0x20,bar_addr[0]+0x8);
outb(0x24,bar_addr[0]+0x8);
outb(0xed,bar_addr[0]+0x8);
outb(0x10,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0xce,bar_addr[0]+0x40);
outb(0x24,bar_addr[0]+0x0);
outb(0xa1,bar_addr[0]+0x4);
outb(0x40,bar_addr[0]+0x38);
outl(0x5611e3f5,bar_addr[0]+0x44);
outl(0x1f76e024,bar_addr[0]+0x48);
outb(0xbb,bar_addr[0]+0xc);
outb(0x83,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x1c);
outb(0x1,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
outb(0x4,bar_addr[0]+0x40);
outb(0x8,bar_addr[0]+0x40);
inb(bar_addr[0]+0x1c);
outb(0xef,bar_addr[0]+0xc);
outb(0x1,bar_addr[0]+0xc);
outb(0x13,bar_addr[0]+0xc);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x8);
inb(bar_addr[0]+0x8);
outb(0x52,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x18);
outb(0x64,bar_addr[0]+0xc);

outb(0x14,bar_addr[0]+0x10);
outb(0x3,bar_addr[0]+0xc);
outb(0x80,bar_addr[0]+0x8);
outb(0x43,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x18);
outb(0x0,bar_addr[0]+0x40);
outb(0x1,bar_addr[0]+0xc);
outb(0x19,bar_addr[0]+0x8);
outb(0x4b,bar_addr[0]+0x8);
outb(0x3a,bar_addr[0]+0x8);
outb(0x2c,bar_addr[0]+0x8);
outb(0xff,bar_addr[0]+0x8);
outb(0x28,bar_addr[0]+0x8);
outb(0x50,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x14);
outb(0x80,bar_addr[0]+0x40);
outb(0x88,bar_addr[0]+0x40);
outb(0x6e,bar_addr[0]+0x0);
outb(0x0,bar_addr[0]+0x4);
outb(0x7b,bar_addr[0]+0x38);
outl(0xf0ff,bar_addr[0]+0x44);
outl(0x1e981000,bar_addr[0]+0x48);
outb(0x19,bar_addr[0]+0xc);
outb(0x87,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x1c);
outb(0x81,bar_addr[0]+0x40);
outb(0x18,bar_addr[0]+0x40);
outb(0x40,bar_addr[0]+0x40);
inb(bar_addr[0]+0x1c);
outb(0x76,bar_addr[0]+0xc);
outb(0x37,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x12,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x64,bar_addr[0]+0xc);

outb(0x10,bar_addr[0]+0x10);
outb(0xc,bar_addr[0]+0xc);
outb(0x56,bar_addr[0]+0x8);
outb(0x4b,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x18);
outb(0x5e,bar_addr[0]+0x40);
outb(0x1f,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0xc2,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x4,bar_addr[0]+0x40);
outb(0x80,bar_addr[0]+0x40);
outb(0xdf,bar_addr[0]+0x0);
outb(0x0,bar_addr[0]+0x4);
outb(0x6a,bar_addr[0]+0x38);
outl(0xff,bar_addr[0]+0x44);
outl(0x1e905018,bar_addr[0]+0x48);
outb(0x10,bar_addr[0]+0xc);
outb(0xdd,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x1c);
outb(0x81,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
outb(0x8,bar_addr[0]+0x40);
outb(0xb7,bar_addr[0]+0xc);
outb(0x1,bar_addr[0]+0xc);
outb(0x51,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x8);
inb(bar_addr[0]+0x8);
outb(0x12,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x44,bar_addr[0]+0xc);

outb(0xbe,bar_addr[0]+0x10);
outb(0x21,bar_addr[0]+0xc);
outb(0xc0,bar_addr[0]+0x8);
outb(0x43,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x18);
outb(0x8f,bar_addr[0]+0x40);
outb(0x5,bar_addr[0]+0xc);
outb(0x1a,bar_addr[0]+0x8);
outb(0x1e,bar_addr[0]+0x8);
outb(0xfd,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x40,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x18,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x2,bar_addr[0]+0x40);
outb(0x80,bar_addr[0]+0x40);
outb(0x40,bar_addr[0]+0x0);
outb(0x0,bar_addr[0]+0x4);
outb(0x0,bar_addr[0]+0x38);
outl(0x8047,bar_addr[0]+0x44);
outl(0x1e8b6502,bar_addr[0]+0x48);
outb(0x4a,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0xc1,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
outb(0x80,bar_addr[0]+0x40);
outb(0x20,bar_addr[0]+0x40);
inb(bar_addr[0]+0x1c);
outb(0x87,bar_addr[0]+0xc);
outb(0x4d,bar_addr[0]+0xc);
outb(0x4c,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x8);
inb(bar_addr[0]+0x8);
outb(0x12,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x44,bar_addr[0]+0xc);

outb(0x42,bar_addr[0]+0x10);
outb(0x1,bar_addr[0]+0xc);
outb(0xd0,bar_addr[0]+0x8);
outb(0x1a,bar_addr[0]+0x8);
outb(0x20,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x92,bar_addr[0]+0x8);
outb(0x84,bar_addr[0]+0x8);
outb(0x62,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x18);
outb(0x80,bar_addr[0]+0x40);
outb(0x98,bar_addr[0]+0x40);
outb(0x24,bar_addr[0]+0x0);
outb(0x0,bar_addr[0]+0x4);
outb(0x0,bar_addr[0]+0x38);
outl(0x24,bar_addr[0]+0x44);
outl(0x1f76e000,bar_addr[0]+0x48);
outb(0x90,bar_addr[0]+0xc);
outb(0x83,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x1c);
outb(0x81,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
outb(0xc8,bar_addr[0]+0x40);
outb(0x0,bar_addr[0]+0x40);
inb(bar_addr[0]+0x1c);
outb(0x1,bar_addr[0]+0xc);
outb(0x1,bar_addr[0]+0xc);
outb(0x1,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x8);
inb(bar_addr[0]+0x8);
outb(0x12,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x44,bar_addr[0]+0xc);

outb(0xc0,bar_addr[0]+0x10);
outb(0x1,bar_addr[0]+0xc);
outb(0x80,bar_addr[0]+0x8);
outb(0x10,bar_addr[0]+0x8);
outb(0x8b,bar_addr[0]+0x8);
outb(0x2,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x24,bar_addr[0]+0x8);
outb(0xc0,bar_addr[0]+0x8);
outb(0x62,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x18);
outb(0xcb,bar_addr[0]+0x40);
outb(0xa0,bar_addr[0]+0x40);
outb(0x4,bar_addr[0]+0x0);
outb(0xa,bar_addr[0]+0x4);
outb(0xc1,bar_addr[0]+0x38);
outl(0x62976c3,bar_addr[0]+0x44);
outl(0x176e024,bar_addr[0]+0x48);
outb(0x6b,bar_addr[0]+0xc);
outb(0x83,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x1c);
outb(0x81,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
outb(0x0,bar_addr[0]+0x40);
outb(0x0,bar_addr[0]+0x40);
inb(bar_addr[0]+0x1c);
outb(0x1,bar_addr[0]+0xc);
outb(0x41,bar_addr[0]+0xc);
outb(0x1,bar_addr[0]+0xc);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x72,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
outb(0x44,bar_addr[0]+0xc);

outb(0x41,bar_addr[0]+0xc);
outb(0xc0,bar_addr[0]+0x8);
outb(0x12,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0xee,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x42,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x18);
outb(0x0,bar_addr[0]+0x40);
outb(0x80,bar_addr[0]+0x40);
outb(0x24,bar_addr[0]+0x0);
outb(0x0,bar_addr[0]+0x4);
outb(0x0,bar_addr[0]+0x38);
outl(0x2c,bar_addr[0]+0x44);
outb(0x98,bar_addr[0]+0xc);
outb(0x83,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x1c);
outb(0xc1,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
outb(0x90,bar_addr[0]+0x40);
outb(0x7d,bar_addr[0]+0x40);
inb(bar_addr[0]+0x1c);
outb(0x3,bar_addr[0]+0xc);
outb(0x1,bar_addr[0]+0xc);
outb(0x31,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x8);
inb(bar_addr[0]+0x8);
outb(0x10,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x19,bar_addr[0]+0xc);

outb(0x74,bar_addr[0]+0x10);
outb(0x5,bar_addr[0]+0xc);
outb(0xb,bar_addr[0]+0x8);
outb(0xda,bar_addr[0]+0x8);
outb(0x40,bar_addr[0]+0x8);
outb(0x7c,bar_addr[0]+0x8);
outb(0x5d,bar_addr[0]+0x8);
outb(0x42,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x18);
outb(0x14,bar_addr[0]+0x40);
outb(0xc0,bar_addr[0]+0x40);
outb(0x24,bar_addr[0]+0x0);
outb(0xd4,bar_addr[0]+0x4);
outb(0x0,bar_addr[0]+0x38);
outl(0x60024,bar_addr[0]+0x44);
outl(0x1f76e027,bar_addr[0]+0x48);
outb(0x90,bar_addr[0]+0xc);
outb(0x7,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x1c);
outb(0x22,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
outb(0x43,bar_addr[0]+0x40);
outb(0x0,bar_addr[0]+0x40);
inb(bar_addr[0]+0x1c);
outb(0xfd,bar_addr[0]+0xc);
outb(0x81,bar_addr[0]+0xc);
outb(0x11,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x8);
inb(bar_addr[0]+0x8);
outb(0x12,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x44,bar_addr[0]+0xc);

outb(0x1,bar_addr[0]+0xc);
outb(0xc0,bar_addr[0]+0x8);
outb(0x51,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x18);
outb(0x4,bar_addr[0]+0x40);
outb(0x21,bar_addr[0]+0xc);
outb(0x12,bar_addr[0]+0x8);
outb(0x2,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x40,bar_addr[0]+0x8);
outb(0x24,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x30,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x2,bar_addr[0]+0x40);
outb(0x80,bar_addr[0]+0x40);
outb(0xa6,bar_addr[0]+0x0);
outb(0x0,bar_addr[0]+0x4);
outb(0x8,bar_addr[0]+0x38);
outl(0x70f024,bar_addr[0]+0x44);
outb(0x9d,bar_addr[0]+0xc);
outb(0x81,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x1c);
outb(0x81,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
outb(0x80,bar_addr[0]+0x40);
outb(0x0,bar_addr[0]+0x40);
inb(bar_addr[0]+0x1c);
outb(0x60,bar_addr[0]+0xc);
outb(0x1,bar_addr[0]+0xc);
outb(0x3d,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x8);
outb(0x12,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x84,bar_addr[0]+0xc);

outb(0x0,bar_addr[0]+0x10);
outb(0x7b,bar_addr[0]+0xc);
outb(0xc0,bar_addr[0]+0x8);
outb(0x43,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x18);
outb(0x0,bar_addr[0]+0x40);
outb(0x2,bar_addr[0]+0xc);
outb(0x21,bar_addr[0]+0x8);
outb(0xd2,bar_addr[0]+0x8);
outb(0x20,bar_addr[0]+0x8);
outb(0x6c,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x30,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x14);
outb(0x0,bar_addr[0]+0x40);
outb(0x7,bar_addr[0]+0x40);
outb(0xff,bar_addr[0]+0x0);
outb(0x4,bar_addr[0]+0x4);
outb(0x4b,bar_addr[0]+0x38);
outl(0x21b618a3,bar_addr[0]+0x44);
outl(0x1e981004,bar_addr[0]+0x48);
outb(0x10,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
outb(0x85,bar_addr[0]+0x40);
outb(0x80,bar_addr[0]+0x40);
outb(0x0,bar_addr[0]+0x40);
inb(bar_addr[0]+0x1c);
outb(0x15,bar_addr[0]+0xc);
outb(0xc7,bar_addr[0]+0xc);
outb(0x15,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x8);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x44,bar_addr[0]+0xc);

outb(0x0,bar_addr[0]+0x10);
outb(0x50,bar_addr[0]+0xc);
outb(0xc2,bar_addr[0]+0x8);
outb(0x47,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x18);
outb(0xc9,bar_addr[0]+0x40);
outb(0x1,bar_addr[0]+0xc);
outb(0x12,bar_addr[0]+0x8);
outb(0x1,bar_addr[0]+0x8);
outb(0x4,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0xfb,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x10,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x0,bar_addr[0]+0x40);
outb(0x74,bar_addr[0]+0x40);
outb(0xdf,bar_addr[0]+0x0);
outb(0x0,bar_addr[0]+0x4);
outb(0x0,bar_addr[0]+0x38);
outl(0x2ba81723,bar_addr[0]+0x44);
outl(0x1e989000,bar_addr[0]+0x48);
outb(0x90,bar_addr[0]+0xc);
outb(0xa3,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x1c);
outb(0x81,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
outb(0x88,bar_addr[0]+0x40);
outb(0x0,bar_addr[0]+0x40);
inb(bar_addr[0]+0x1c);
outb(0x77,bar_addr[0]+0xc);
outb(0x1,bar_addr[0]+0xc);
outb(0x6c,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x8);
inb(bar_addr[0]+0x8);
outb(0x16,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x54,bar_addr[0]+0xc);

outb(0x10,bar_addr[0]+0x10);
outb(0xa6,bar_addr[0]+0xc);
outb(0xc0,bar_addr[0]+0x8);
outb(0x43,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x18);
outb(0x2,bar_addr[0]+0x40);
outb(0x12,bar_addr[0]+0x8);
outb(0x1,bar_addr[0]+0x8);
outb(0x55,bar_addr[0]+0x8);
outb(0x10,bar_addr[0]+0x8);
outb(0x9e,bar_addr[0]+0x8);
outb(0xd3,bar_addr[0]+0x8);
outb(0x10,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x0,bar_addr[0]+0x40);
outb(0xcb,bar_addr[0]+0x40);
outb(0xef,bar_addr[0]+0x0);
outb(0x0,bar_addr[0]+0x4);
outb(0x0,bar_addr[0]+0x38);
outl(0x3f63f503,bar_addr[0]+0x44);
outl(0x3e981000,bar_addr[0]+0x48);
outb(0x12,bar_addr[0]+0xc);
outb(0xef,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x1c);
outb(0x89,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
outb(0x80,bar_addr[0]+0x40);
outb(0x63,bar_addr[0]+0x40);
inb(bar_addr[0]+0x1c);
outb(0x8b,bar_addr[0]+0xc);
outb(0x21,bar_addr[0]+0xc);
outb(0x91,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x8);
inb(bar_addr[0]+0x8);
outb(0xad,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);

outb(0x0,bar_addr[0]+0x10);
outb(0x1,bar_addr[0]+0xc);
outb(0xc0,bar_addr[0]+0x8);
outb(0x12,bar_addr[0]+0x8);
outb(0x8,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0xb0,bar_addr[0]+0x8);
outb(0x0,bar_addr[0]+0x8);
outb(0x42,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x18);
outb(0x2,bar_addr[0]+0x40);
outb(0x80,bar_addr[0]+0x40);
outb(0xde,bar_addr[0]+0x0);
outb(0x20,bar_addr[0]+0x4);
outb(0x0,bar_addr[0]+0x38);
outl(0x24,bar_addr[0]+0x44);
outl(0x1f7ae000,bar_addr[0]+0x48);
outb(0x90,bar_addr[0]+0xc);
outb(0x83,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x1c);
outb(0x81,bar_addr[0]+0x40);
inb(bar_addr[0]+0x54);
outb(0x4a,bar_addr[0]+0x40);
outb(0x0,bar_addr[0]+0x40);
inb(bar_addr[0]+0x1c);
outb(0x1,bar_addr[0]+0xc);
outb(0x41,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
inb(bar_addr[0]+0x8);
inb(bar_addr[0]+0x8);
outb(0x12,bar_addr[0]+0xc);
inb(bar_addr[0]+0x54);
inb(bar_addr[0]+0x10);
inb(bar_addr[0]+0x18);
inb(bar_addr[0]+0x14);
outb(0x44,bar_addr[0]+0xc);

        return 0;
}

// [WHY] linux kernel variable.
// search "pnp_global" at System.map
struct list_head* list_pnp_head = (struct list_head*)(0xc1e46be4);


/*
** 1. disable hardware interupts from all devices
** 2. unregister all device drivers
 */
int suppress_drivers(void)
{
        struct pci_dev *pdev;
        struct dev *dev;
        unsigned int vendor;
        unsigned int device;
        /* 1. disable hardware interupts from all devices  */
        printk(KERN_INFO "###| [%s] irq disable\n",__FUNCTION__);
        native_irq_disable();

        //printk(KERN_INFO "###| [INFO] driver detach\n");
        /* 2. unregister all device drivers  */
        /* (only pci_devices for now) */
        list_for_each_entry(dev, &dev_list_head, node) {
                vendor = dev->cfg.vendor;
                device = dev->cfg.device;
                printk(KERN_INFO "###| [%s] device [%x:%x]\n",__FUNCTION__,vendor,device);
                pdev = pci_get_device(vendor,device,NULL);
                if(pdev->driver){
                        printk(KERN_INFO "###| [%s] yes driver exists! named..%s\n",__FUNCTION__,pdev->driver->name);
                        if((device==0x10d3)||(device==0x100e)){
                                printk(KERN_INFO "###| [%s] let network go\n",__FUNCTION__);
                        }else if(device==0x7010){
                                printk(KERN_INFO "###| [%s] let hdd go\n",__FUNCTION__);
                        }else{
                                pci_unregister_driver(pdev->driver);
                                printk(KERN_INFO "###| [%s] but now it's gone uwu\n",__FUNCTION__);
                        }
                }else{
                        printk(KERN_INFO "###| [%s] device with no driver..!\n",__FUNCTION__);
                }
        }


        /* isa devices */
        /*
        struct pnp_dev *pnpdev;
        list_for_each_entry(pnpdev, list_pnp_head, global_list)
        {
       
                if(pnpdev!=NULL){
                        printk(KERN_INFO "###| [%s]: PNP_NAME:[%s]\n",__FUNCTION__,pnpdev->name);
                        if(pnpdev->driver!=NULL){
                                printk(KERN_INFO "###| [%s]: PNP_DRIVER_NAME:[%s]\n",__FUNCTION__,pnpdev->driver->name);
                                pnp_unregister_driver(pnpdev->driver);
                                printk(KERN_INFO "###| [%s]: unregistered!\n",__FUNCTION__);
                        }else{
                                printk(KERN_INFO "###| [%s]: no PNP driver\n",__FUNCTION__);
                        }
                }else{
                        printk(KERN_INFO "###| [%s]: no PNP\n",__FUNCTION__);
                }
        }
        */

        return 0;
}

static int __init init_mymod(void)
{
        printk(KERN_ALERT "###| [INFO] mymod inserted!\n");

        enum_dev();
        suppress_drivers();
        repro();
        native_irq_enable();

        return 0;
}

static void __exit exit_mymod(void)
{
        remove_dev_list();
        printk(KERN_ALERT "mymod exited!\n");
}

module_init(init_mymod);
module_exit(exit_mymod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("skim");
