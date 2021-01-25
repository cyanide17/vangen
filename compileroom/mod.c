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
        unsigned int        vendor;
        unsigned int        device;
        resource_size_t     BAR[6];
        int                 BARtype[6]; // 0:MMIO 1:PMIO
        unsigned long       BARsize[6];
        void __iomem*       virtBAR[6];
        int                 BAR_isvalid[6];
};

struct dev{
        struct dev_cfg      cfg;
        struct list_head    node;
};

typedef struct _DMA_block
{
        void *cpu_addr;
        unsigned int log_addr;
        dma_addr_t dma_handle;
        struct list_head node;
}DMA_blk;

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

void print_DMA_blks(void)
{
        DMA_blk *blk;
        printk(KERN_INFO "printing all dma blks\n");
        list_for_each_entry(blk, &DMA_blks_head, node) {
                printk(KERN_INFO "0x%x\n",blk->log_addr);
        }
}

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

struct dev* load_bar(unsigned int vendor, unsigned int device){
        struct dev *dev = check_dev_list(vendor,device);
        if(dev==NULL){
                printk(KERN_ERR "[ERR ] load_bar tried for not existing device\n");
        }
        return dev;
}

int repro(void)
{
        printk(KERN_INFO "[%s] REPRO START\n",__FUNCTION__);
        void __iomem* bar_addr[6];
        struct dev *dev;

        void *cpu_addr;
        dma_addr_t dma_handle;
        struct pci_dev *pdev;
        DMA_blk *blk;

        // 
pdev = pci_get_device(0x1022,0x2020,NULL);
cpu_addr = dma_alloc_coherent(&(pdev->dev),0x1000, &dma_handle, GFP_KERNEL);
add_DMA_blk(cpu_addr,0x1e94b000,dma_handle);


dev = load_bar(0x1022,0x2020);
memcpy(bar_addr,dev->cfg.virtBAR,sizeof(bar_addr));
blk = get_DMA_blk(0x1e94b000);
writeb(0xab, blk->cpu_addr);
writeb(0x20, blk->cpu_addr);
writeb(0x4e, blk->cpu_addr);
writeb(0xc, blk->cpu_addr);
writeb(0x24, blk->cpu_addr);
writeb(0x3d, blk->cpu_addr);
outb(0x1,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0xc0,(int)bar_addr[0]+0x8);
outb(0x7a,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0xdf,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x24,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x24,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
outb(0x60,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x18);
outb(0x4,(int)bar_addr[0]+0x40);
outb(0x80,(int)bar_addr[0]+0x40);
outb(0x24,(int)bar_addr[0]+0x0);
outb(0x96,(int)bar_addr[0]+0x4);
outb(0xa3,(int)bar_addr[0]+0x38);
outl(0x767d3ca4,(int)bar_addr[0]+0x44);
outl(0xdc27559,(int)bar_addr[0]+0x48);
outb(0x48,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x1c);
outb(0xe9,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
outb(0x80,(int)bar_addr[0]+0x40);
outb(0x82,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x1c);
outb(0x3,(int)bar_addr[0]+0xc);
outb(0x25,(int)bar_addr[0]+0xc);
outb(0x5a,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x8);
inb((int)bar_addr[0]+0x8);
outb(0x7f,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);

writeb(0x42, blk->cpu_addr);
writeb(0x20, blk->cpu_addr);
writeb(0xd3, blk->cpu_addr);
writeb(0x1e, blk->cpu_addr);
writeb(0xaa, blk->cpu_addr);
outb(0xc,(int)bar_addr[0]+0x10);
outb(0x1b,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0xed,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
readb(blk->cpu_addr);
outb(0x50,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x75,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x51,(int)bar_addr[0]+0x8);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x18);
outb(0xfd,(int)bar_addr[0]+0x40);
outb(0xc0,(int)bar_addr[0]+0x40);
outb(0x24,(int)bar_addr[0]+0x0);
outb(0xa2,(int)bar_addr[0]+0x4);
outb(0x2,(int)bar_addr[0]+0x38);
outl(0x60024,(int)bar_addr[0]+0x44);
outl(0x1f97e024,(int)bar_addr[0]+0x48);
outb(0x80,(int)bar_addr[0]+0xc);
outb(0xa7,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x1c);
outb(0xb6,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
outb(0x3,(int)bar_addr[0]+0x40);
outb(0xd5,(int)bar_addr[0]+0x40);
outb(0xb1,(int)bar_addr[0]+0xc);
outb(0x41,(int)bar_addr[0]+0xc);
outb(0x11,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x8);
inb((int)bar_addr[0]+0x8);
outb(0x2,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x64,(int)bar_addr[0]+0xc);

writeb(0xea, blk->cpu_addr);
outb(0x1,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0xba,(int)bar_addr[0]+0x8);
outb(0x51,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x18);
outb(0x46,(int)bar_addr[0]+0x40);
outb(0x1b,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0xb6,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x8,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0xa,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x4,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x4,(int)bar_addr[0]+0x8);
outb(0x30,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x2,(int)bar_addr[0]+0x40);
outb(0x80,(int)bar_addr[0]+0x40);
outb(0xd7,(int)bar_addr[0]+0x0);
outb(0x2,(int)bar_addr[0]+0x4);
outb(0x8,(int)bar_addr[0]+0x38);
outl(0x700024,(int)bar_addr[0]+0x44);
outl(0x1f76e024,(int)bar_addr[0]+0x48);
outb(0xdd,(int)bar_addr[0]+0xc);
outb(0xd1,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x1c);
outb(0x6f,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
outb(0xd6,(int)bar_addr[0]+0x40);
outb(0x0,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x1c);
outb(0x60,(int)bar_addr[0]+0xc);
outb(0x81,(int)bar_addr[0]+0xc);
outb(0x3b,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x8);
inb((int)bar_addr[0]+0x8);
outb(0x32,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x0,(int)bar_addr[0]+0xc);

outb(0xa4,(int)bar_addr[0]+0x10);
outb(0x9,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0xc0,(int)bar_addr[0]+0x8);
outb(0x51,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x18);
outb(0x80,(int)bar_addr[0]+0x40);
outb(0x9,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0x2d,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
readb(blk->cpu_addr);
outb(0x10,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x20,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x24,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0xed,(int)bar_addr[0]+0x8);
outb(0x10,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0xce,(int)bar_addr[0]+0x40);
outb(0x24,(int)bar_addr[0]+0x0);
outb(0xa1,(int)bar_addr[0]+0x4);
outb(0x40,(int)bar_addr[0]+0x38);
outl(0x5611e3f5,(int)bar_addr[0]+0x44);
outl(0x1f76e024,(int)bar_addr[0]+0x48);
outb(0xbb,(int)bar_addr[0]+0xc);
outb(0x83,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x1c);
outb(0x1,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
outb(0x4,(int)bar_addr[0]+0x40);
outb(0x8,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x1c);
outb(0xef,(int)bar_addr[0]+0xc);
outb(0x1,(int)bar_addr[0]+0xc);
outb(0x13,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x8);
inb((int)bar_addr[0]+0x8);
outb(0x52,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x18);
outb(0x64,(int)bar_addr[0]+0xc);

writeb(0xc0, blk->cpu_addr);
outb(0x14,(int)bar_addr[0]+0x10);
outb(0x3,(int)bar_addr[0]+0xc);
outb(0x80,(int)bar_addr[0]+0x8);
outb(0x43,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x18);
outb(0x0,(int)bar_addr[0]+0x40);
outb(0x1,(int)bar_addr[0]+0xc);
outb(0x19,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x4b,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x3a,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x2c,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0xff,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x28,(int)bar_addr[0]+0x8);
outb(0x50,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x14);
outb(0x80,(int)bar_addr[0]+0x40);
outb(0x88,(int)bar_addr[0]+0x40);
outb(0x6e,(int)bar_addr[0]+0x0);
outb(0x0,(int)bar_addr[0]+0x4);
outb(0x7b,(int)bar_addr[0]+0x38);
outl(0xf0ff,(int)bar_addr[0]+0x44);
outl(0x1e981000,(int)bar_addr[0]+0x48);
outb(0x19,(int)bar_addr[0]+0xc);
outb(0x87,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x1c);
outb(0x81,(int)bar_addr[0]+0x40);
outb(0x18,(int)bar_addr[0]+0x40);
outb(0x40,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x1c);
outb(0x76,(int)bar_addr[0]+0xc);
outb(0x37,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x12,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x64,(int)bar_addr[0]+0xc);

writeb(0xba, blk->cpu_addr);
outb(0x10,(int)bar_addr[0]+0x10);
outb(0xc,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0x56,(int)bar_addr[0]+0x8);
outb(0x4b,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x18);
outb(0x5e,(int)bar_addr[0]+0x40);
readb(blk->cpu_addr);
outb(0x1f,(int)bar_addr[0]+0x8);
outb(0x0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
outb(0xc2,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x4,(int)bar_addr[0]+0x40);
outb(0x80,(int)bar_addr[0]+0x40);
outb(0xdf,(int)bar_addr[0]+0x0);
outb(0x0,(int)bar_addr[0]+0x4);
outb(0x6a,(int)bar_addr[0]+0x38);
outl(0xff,(int)bar_addr[0]+0x44);
outl(0x1e905018,(int)bar_addr[0]+0x48);
outb(0x10,(int)bar_addr[0]+0xc);
outb(0xdd,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x1c);
outb(0x81,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
outb(0x8,(int)bar_addr[0]+0x40);
outb(0xb7,(int)bar_addr[0]+0xc);
outb(0x1,(int)bar_addr[0]+0xc);
outb(0x51,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x8);
inb((int)bar_addr[0]+0x8);
outb(0x12,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x44,(int)bar_addr[0]+0xc);

writeb(0xc4, blk->cpu_addr);
outb(0xbe,(int)bar_addr[0]+0x10);
outb(0x21,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0xc0,(int)bar_addr[0]+0x8);
outb(0x43,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x18);
outb(0x8f,(int)bar_addr[0]+0x40);
outb(0x5,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0x1a,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x1e,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0xfd,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x40,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
outb(0x18,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x2,(int)bar_addr[0]+0x40);
outb(0x80,(int)bar_addr[0]+0x40);
outb(0x40,(int)bar_addr[0]+0x0);
outb(0x0,(int)bar_addr[0]+0x4);
outb(0x0,(int)bar_addr[0]+0x38);
outl(0x8047,(int)bar_addr[0]+0x44);
outl(0x1e8b6502,(int)bar_addr[0]+0x48);
outb(0x4a,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0xc1,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
outb(0x80,(int)bar_addr[0]+0x40);
outb(0x20,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x1c);
outb(0x87,(int)bar_addr[0]+0xc);
outb(0x4d,(int)bar_addr[0]+0xc);
outb(0x4c,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x8);
inb((int)bar_addr[0]+0x8);
outb(0x12,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x44,(int)bar_addr[0]+0xc);

writeb(0xd0, blk->cpu_addr);
writeb(0x12, blk->cpu_addr);
writeb(0x0, blk->cpu_addr);
writeb(0x10, blk->cpu_addr);
writeb(0x40, blk->cpu_addr);
writeb(0x24, blk->cpu_addr);
outb(0x42,(int)bar_addr[0]+0x10);
outb(0x1,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0xd0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x1a,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x20,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x92,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x84,(int)bar_addr[0]+0x8);
outb(0x62,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x18);
outb(0x80,(int)bar_addr[0]+0x40);
outb(0x98,(int)bar_addr[0]+0x40);
outb(0x24,(int)bar_addr[0]+0x0);
outb(0x0,(int)bar_addr[0]+0x4);
outb(0x0,(int)bar_addr[0]+0x38);
outl(0x24,(int)bar_addr[0]+0x44);
outl(0x1f76e000,(int)bar_addr[0]+0x48);
outb(0x90,(int)bar_addr[0]+0xc);
outb(0x83,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x1c);
outb(0x81,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
outb(0xc8,(int)bar_addr[0]+0x40);
outb(0x0,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x1c);
outb(0x1,(int)bar_addr[0]+0xc);
outb(0x1,(int)bar_addr[0]+0xc);
outb(0x1,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x8);
inb((int)bar_addr[0]+0x8);
outb(0x12,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x44,(int)bar_addr[0]+0xc);

writeb(0xc2, blk->cpu_addr);
writeb(0x16, blk->cpu_addr);
writeb(0xe0, blk->cpu_addr);
writeb(0x10, blk->cpu_addr);
writeb(0x10, blk->cpu_addr);
writeb(0x36, blk->cpu_addr);
writeb(0xdc, blk->cpu_addr);
outb(0xc0,(int)bar_addr[0]+0x10);
outb(0x1,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0x80,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x10,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x8b,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x2,(int)bar_addr[0]+0x8);
outb(0x0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x24,(int)bar_addr[0]+0x8);
outb(0xc0,(int)bar_addr[0]+0x8);
outb(0x62,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x18);
outb(0xcb,(int)bar_addr[0]+0x40);
outb(0xa0,(int)bar_addr[0]+0x40);
outb(0x4,(int)bar_addr[0]+0x0);
outb(0xa,(int)bar_addr[0]+0x4);
outb(0xc1,(int)bar_addr[0]+0x38);
outl(0x62976c3,(int)bar_addr[0]+0x44);
outl(0x176e024,(int)bar_addr[0]+0x48);
outb(0x6b,(int)bar_addr[0]+0xc);
outb(0x83,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x1c);
outb(0x81,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
outb(0x0,(int)bar_addr[0]+0x40);
outb(0x0,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x1c);
outb(0x1,(int)bar_addr[0]+0xc);
outb(0x41,(int)bar_addr[0]+0xc);
outb(0x1,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x72,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
outb(0x44,(int)bar_addr[0]+0xc);

writeb(0xd0, blk->cpu_addr);
writeb(0xab, blk->cpu_addr);
writeb(0x0, blk->cpu_addr);
writeb(0x40, blk->cpu_addr);
writeb(0x0, blk->cpu_addr);
writeb(0x24, blk->cpu_addr);
writeb(0x9d, blk->cpu_addr);
outb(0x41,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0xc0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x12,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0xee,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
outb(0x42,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x18);
outb(0x0,(int)bar_addr[0]+0x40);
outb(0x80,(int)bar_addr[0]+0x40);
outb(0x24,(int)bar_addr[0]+0x0);
outb(0x0,(int)bar_addr[0]+0x4);
outb(0x0,(int)bar_addr[0]+0x38);
outl(0x2c,(int)bar_addr[0]+0x44);
outb(0x98,(int)bar_addr[0]+0xc);
outb(0x83,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x1c);
outb(0xc1,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
outb(0x90,(int)bar_addr[0]+0x40);
outb(0x7d,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x1c);
outb(0x3,(int)bar_addr[0]+0xc);
outb(0x1,(int)bar_addr[0]+0xc);
outb(0x31,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x8);
inb((int)bar_addr[0]+0x8);
outb(0x10,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x19,(int)bar_addr[0]+0xc);

writeb(0x2, blk->cpu_addr);
writeb(0x0, blk->cpu_addr);
writeb(0x5b, blk->cpu_addr);
writeb(0x10, blk->cpu_addr);
writeb(0x24, blk->cpu_addr);
writeb(0xea, blk->cpu_addr);
outb(0x74,(int)bar_addr[0]+0x10);
outb(0x5,(int)bar_addr[0]+0xc);
outb(0xb,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0xda,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
readb(blk->cpu_addr);
outb(0x40,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x7c,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x5d,(int)bar_addr[0]+0x8);
outb(0x42,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x18);
outb(0x14,(int)bar_addr[0]+0x40);
outb(0xc0,(int)bar_addr[0]+0x40);
outb(0x24,(int)bar_addr[0]+0x0);
outb(0xd4,(int)bar_addr[0]+0x4);
outb(0x0,(int)bar_addr[0]+0x38);
outl(0x60024,(int)bar_addr[0]+0x44);
outl(0x1f76e027,(int)bar_addr[0]+0x48);
outb(0x90,(int)bar_addr[0]+0xc);
outb(0x7,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x1c);
outb(0x22,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
outb(0x43,(int)bar_addr[0]+0x40);
outb(0x0,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x1c);
outb(0xfd,(int)bar_addr[0]+0xc);
outb(0x81,(int)bar_addr[0]+0xc);
outb(0x11,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x8);
inb((int)bar_addr[0]+0x8);
outb(0x12,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x44,(int)bar_addr[0]+0xc);

writeb(0xc0, blk->cpu_addr);
outb(0x1,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0xc0,(int)bar_addr[0]+0x8);
outb(0x51,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x18);
outb(0x4,(int)bar_addr[0]+0x40);
outb(0x21,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0x12,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x2,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x40,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x24,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
outb(0x30,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x2,(int)bar_addr[0]+0x40);
outb(0x80,(int)bar_addr[0]+0x40);
outb(0xa6,(int)bar_addr[0]+0x0);
outb(0x0,(int)bar_addr[0]+0x4);
outb(0x8,(int)bar_addr[0]+0x38);
outl(0x70f024,(int)bar_addr[0]+0x44);
outb(0x9d,(int)bar_addr[0]+0xc);
outb(0x81,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x1c);
outb(0x81,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
outb(0x80,(int)bar_addr[0]+0x40);
outb(0x0,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x1c);
outb(0x60,(int)bar_addr[0]+0xc);
outb(0x1,(int)bar_addr[0]+0xc);
outb(0x3d,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x8);
outb(0x12,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x84,(int)bar_addr[0]+0xc);

writeb(0x10, blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x10);
outb(0x7b,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0xc0,(int)bar_addr[0]+0x8);
outb(0x43,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x18);
outb(0x0,(int)bar_addr[0]+0x40);
outb(0x2,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0x21,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0xd2,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x20,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x6c,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
outb(0x30,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x14);
outb(0x0,(int)bar_addr[0]+0x40);
outb(0x7,(int)bar_addr[0]+0x40);
outb(0xff,(int)bar_addr[0]+0x0);
outb(0x4,(int)bar_addr[0]+0x4);
outb(0x4b,(int)bar_addr[0]+0x38);
outl(0x21b618a3,(int)bar_addr[0]+0x44);
outl(0x1e981004,(int)bar_addr[0]+0x48);
outb(0x10,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
outb(0x85,(int)bar_addr[0]+0x40);
outb(0x80,(int)bar_addr[0]+0x40);
outb(0x0,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x1c);
outb(0x15,(int)bar_addr[0]+0xc);
outb(0xc7,(int)bar_addr[0]+0xc);
outb(0x15,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x8);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x44,(int)bar_addr[0]+0xc);

writeb(0xc0, blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x10);
outb(0x50,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0xc2,(int)bar_addr[0]+0x8);
outb(0x47,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x18);
outb(0xc9,(int)bar_addr[0]+0x40);
outb(0x1,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0x12,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x1,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x4,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0xfb,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
outb(0x10,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x0,(int)bar_addr[0]+0x40);
outb(0x74,(int)bar_addr[0]+0x40);
outb(0xdf,(int)bar_addr[0]+0x0);
outb(0x0,(int)bar_addr[0]+0x4);
outb(0x0,(int)bar_addr[0]+0x38);
outl(0x2ba81723,(int)bar_addr[0]+0x44);
outl(0x1e989000,(int)bar_addr[0]+0x48);
outb(0x90,(int)bar_addr[0]+0xc);
outb(0xa3,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x1c);
outb(0x81,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
outb(0x88,(int)bar_addr[0]+0x40);
outb(0x0,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x1c);
outb(0x77,(int)bar_addr[0]+0xc);
outb(0x1,(int)bar_addr[0]+0xc);
outb(0x6c,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x8);
inb((int)bar_addr[0]+0x8);
outb(0x16,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x54,(int)bar_addr[0]+0xc);

writeb(0xc0, blk->cpu_addr);
outb(0x10,(int)bar_addr[0]+0x10);
outb(0xa6,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0xc0,(int)bar_addr[0]+0x8);
outb(0x43,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x18);
outb(0x2,(int)bar_addr[0]+0x40);
readb(blk->cpu_addr);
outb(0x12,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x1,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x55,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x10,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x9e,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0xd3,(int)bar_addr[0]+0x8);
outb(0x10,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x0,(int)bar_addr[0]+0x40);
outb(0xcb,(int)bar_addr[0]+0x40);
outb(0xef,(int)bar_addr[0]+0x0);
outb(0x0,(int)bar_addr[0]+0x4);
outb(0x0,(int)bar_addr[0]+0x38);
outl(0x3f63f503,(int)bar_addr[0]+0x44);
outl(0x3e981000,(int)bar_addr[0]+0x48);
outb(0x12,(int)bar_addr[0]+0xc);
outb(0xef,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x1c);
outb(0x89,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
outb(0x80,(int)bar_addr[0]+0x40);
outb(0x63,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x1c);
outb(0x8b,(int)bar_addr[0]+0xc);
outb(0x21,(int)bar_addr[0]+0xc);
outb(0x91,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x8);
inb((int)bar_addr[0]+0x8);
outb(0xad,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);

writeb(0xc0, blk->cpu_addr);
writeb(0x12, blk->cpu_addr);
writeb(0x0, blk->cpu_addr);
writeb(0xf3, blk->cpu_addr);
writeb(0x0, blk->cpu_addr);
writeb(0x24, blk->cpu_addr);
writeb(0x0, blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x10);
outb(0x1,(int)bar_addr[0]+0xc);
readb(blk->cpu_addr);
outb(0xc0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x12,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x8,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0xb0,(int)bar_addr[0]+0x8);
readb(blk->cpu_addr);
outb(0x0,(int)bar_addr[0]+0x8);
outb(0x42,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x18);
outb(0x2,(int)bar_addr[0]+0x40);
outb(0x80,(int)bar_addr[0]+0x40);
outb(0xde,(int)bar_addr[0]+0x0);
outb(0x20,(int)bar_addr[0]+0x4);
outb(0x0,(int)bar_addr[0]+0x38);
outl(0x24,(int)bar_addr[0]+0x44);
outl(0x1f7ae000,(int)bar_addr[0]+0x48);
outb(0x90,(int)bar_addr[0]+0xc);
outb(0x83,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x1c);
outb(0x81,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x54);
outb(0x4a,(int)bar_addr[0]+0x40);
outb(0x0,(int)bar_addr[0]+0x40);
inb((int)bar_addr[0]+0x1c);
outb(0x1,(int)bar_addr[0]+0xc);
outb(0x41,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
inb((int)bar_addr[0]+0x8);
inb((int)bar_addr[0]+0x8);
outb(0x12,(int)bar_addr[0]+0xc);
inb((int)bar_addr[0]+0x54);
inb((int)bar_addr[0]+0x10);
inb((int)bar_addr[0]+0x18);
inb((int)bar_addr[0]+0x14);
outb(0x44,(int)bar_addr[0]+0xc);
pdev = pci_get_device(0x1022,0x2020,NULL);
blk = get_DMA_blk(0x1e94b000);
dma_free_coherent(&(pdev->dev), 0x1000, blk->cpu_addr, blk->dma_handle);


        printk(KERN_INFO "[%s] REPRO END\n",__FUNCTION__);
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
        printk(KERN_ALERT "###| [%s] mymod inserted!\n",__FUNCTION__);

        enum_dev();
        suppress_drivers();
        repro();
        native_irq_enable();

        return 0;
}

static void __exit exit_mymod(void)
{
        remove_dev_list();
        printk(KERN_ALERT "###| [%s] mymod exited!\n",__FUNCTION__);
}

module_init(init_mymod);
module_exit(exit_mymod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("skim");
