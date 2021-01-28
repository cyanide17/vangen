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
#include <linux/dma-mapping.h>

#define MMIO 0
#define PMIO 1

#define INFO 0
#define ERR 1

static LIST_HEAD(dev_list_head);
static LIST_HEAD(DMA_blks_head);

struct dev_cfg{
        unsigned int        vendor;           /* vendor id */
        unsigned int        device;           /* device id */
        int                 BAR[6];           /* Base Address Registers */
        int                 BARtype[6];       /* type of address; 0: MMIO 1: PMIO */
        unsigned long       BARsize[6];       /* size of address space */
        int                 BARisvalid[6];    /* whether the value in BAR is valid or not */
};
struct dev{
        struct dev_cfg      cfg;              /* configuration info about device */
        struct list_head    node;             /* list node */
};

typedef struct _DMA_block{
        void                *cpu_addr;        /* virtual base address of allocated DMA zone */
        unsigned int        log_addr;         /* target address recorded in log */
        dma_addr_t          dma_handle;       /* bus base address of allocated DMA zone */
        struct              list_head node;   /* list node */
}DMA_blk;


/* search "pnp_global" at System.map and insert the value */
struct list_head* list_pnp_head = (struct list_head*)(0xc1e46be4/* insert here */);


/*
 * add DMA_blk struct to a list.
 */
int add_DMA_blk(void *cpu_addr, unsigned int log_addr, dma_addr_t dma_handle)
{
        DMA_blk *new_blk = (DMA_blk *)kzalloc(sizeof(DMA_blk), GFP_KERNEL);
        if(!new_blk){
                printk(KERN_ERR "###| [ERR ] memory allocation failed!\n");
                return 1;
        }
        new_blk->cpu_addr = cpu_addr;
        new_blk->log_addr = log_addr;
        new_blk->dma_handle = dma_handle;

        INIT_LIST_HEAD(&new_blk->node);
        list_add_tail(&new_blk->node, &DMA_blks_head);

        return 0;
}

/*
 * print log addr of all DMA_blk structs.
 */
void print_DMA_blks(void)
{
        DMA_blk *blk;
        printk(KERN_INFO "printing all dma blks\n");
        list_for_each_entry(blk, &DMA_blks_head, node) {
                printk(KERN_INFO "0x%x\n",blk->log_addr);
        }
}

/*
 * get DMA_blk with specified log_addr from the list.
 * return NULL if not exists.
 */
DMA_blk *get_DMA_blk(unsigned int log_addr)
{
        DMA_blk *blk;
        list_for_each_entry(blk, &DMA_blks_head, node) {
                if(blk->log_addr == log_addr)
                {
                        return blk;
                }
        }
        return NULL;
}

/*
 * get struct dev with specified vendor id and device id.
 * return NULL if not exists.
 */
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

/*
 * add dev struct to a list.
 */
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
                new_dev->cfg.BAR[bar_idx] = (int)ioremap(dev->cfg.BAR[bar_idx],dev->cfg.BARsize[bar_idx]);
                new_dev->cfg.BARtype[bar_idx] = MMIO;
        }else{
                new_dev->cfg.BARtype[bar_idx] = PMIO;
        }
        new_dev->cfg.BARisvalid[bar_idx] = true;
        return 0;
}

/*
 * remove all dev structs and free the memories
 */
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
                        if((dev->cfg.BARisvalid[i]==true)&&(dev->cfg.BARtype[i]==MMIO)){
                                printk(KERN_INFO "###| [%s] unmapping BAR[%d]:%d\n",__FUNCTION__,i,dev->cfg.BAR[i]);
                                iounmap((void __iomem*)dev->cfg.BAR[i]);
                        }
                }
                list_del(node);
                kfree(dev);
        }

}


/*
 * print fields of all struct devs
 */
void print_dev_list(void)
{
        struct dev *dev;
        int i;
        printk(KERN_INFO "printing all dev\n");
        list_for_each_entry(dev, &dev_list_head, node) {
                printk(KERN_INFO "== (%x:%x) ==\n",dev->cfg.vendor,dev->cfg.device);
                for(i=0;i<6;i++){
                        if(dev->cfg.BARisvalid[i]==true){
                                printk(KERN_INFO "| BAR[%d]=%x\n",i,dev->cfg.BAR[i]);
                                if(dev->cfg.BARtype[i]==MMIO){
                                        printk(KERN_INFO "| type: MMIO\n");
                                }else{
                                        printk(KERN_INFO "| type: PMIO\n");
                                }
                                printk(KERN_INFO "| BARsize[%d]=%lx\n",i,dev->cfg.BARsize[i]);
                                printk(KERN_INFO "|-\n");
                        }
                }
        }
}

/*
 * probe PCI configuration spaces and read their address spaces' base & size from BAR.
 */
int enum_dev(void)
{
        unsigned int pcicfg_idx;
        int b,d,f; /* bus, device, function */
        int bar_idx;
        unsigned int tmpval;
        struct dev *dev;

        /* struct dev initialization */
        dev = (struct dev*)kzalloc(sizeof(struct dev), GFP_KERNEL);
        if(!dev){
                printk(KERN_ERR "memory allocation failed!\n");
                return 1;
        }

        /* scan all pci configuration space */
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

                                        /* reading BARs */
                                        for(bar_idx=0;bar_idx<6;bar_idx++){
                                                outl(pcicfg_idx+0x10+4*bar_idx,0xCF8);
                                                if(inl(0xCFC)!=0){ /* if valid BAR */
                                                        tmpval = inl(0xCFC);

                                                        /* 1. address space size */
                                                        outl(~0x0,0xCFC); /* write 0xFs*/
                                                        dev->cfg.BARsize[bar_idx] = inl(0xCFC);
                                                        outl(tmpval,0xCFC); /* restore original BAR */
                                                        dev->cfg.BARsize[bar_idx] = (~(dev->cfg.BARsize[bar_idx]&(~0xF)))+1; /* calculating size */

                                                        /* 2. base address & type */
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


/*
 * print PCI configuration table with specified b, d, f (bus, device, function) as index.
 */
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

/*
 * return pointer to struct dev that have specified vendor id & device id
 * return NULL if not exists
 */
struct dev* load_bar(unsigned int vendor, unsigned int device){
        struct dev *dev = check_dev_list(vendor,device);
        if(dev==NULL){
                printk(KERN_ERR "[ERR ] load_bar tried for not existing device\n");
        }
        return dev;
}

/*
 * function where parsed I/O operations will be merged into.
 */
int repro(void)
{
        void __iomem* bar_addr[6];
        struct dev *dev;

        void *cpu_addr;
        dma_addr_t dma_handle;
        struct pci_dev *pdev;
        DMA_blk *blk;

        printk(KERN_INFO "[%s] REPRO START\n",__FUNCTION__);

        // @


        printk(KERN_INFO "[%s] REPRO END\n",__FUNCTION__);
        return 0;
}



/*
 * disable hardware interupts from all devices.
 * unregister all device drivers.
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
        /* there is some buggy 
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
        printk(KERN_ALERT "###| [%s] vangen_mod inserted!\n",__FUNCTION__);

        enum_dev();
        suppress_drivers();
        if(repro() < 0)
          return -1;
        native_irq_enable();

        return 0;
}

static void __exit exit_mymod(void)
{
        remove_dev_list();
        printk(KERN_ALERT "###| [%s] vangen_mod exited!\n",__FUNCTION__);
}

module_init(init_mymod);
module_exit(exit_mymod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("skim");
