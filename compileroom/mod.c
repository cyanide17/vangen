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
        int                 BAR[6];
        int                 BARtype[6]; // 0:MMIO 1:PMIO
        unsigned long       BARsize[6];
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
                new_dev->cfg.BAR[bar_idx] = (int)ioremap(dev->cfg.BAR[bar_idx],dev->cfg.BARsize[bar_idx]);
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
                                printk(KERN_INFO "###| [%s] unmapping BAR[%d]:%d\n",__FUNCTION__,i,dev->cfg.BAR[i]);
                                iounmap((void __iomem*)dev->cfg.BAR[i]);
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
        void __iomem* bar_addr[6];
        struct dev *dev;

        void *cpu_addr;
        dma_addr_t dma_handle;
        struct pci_dev *pdev;
        DMA_blk *blk;

        // 
pdev = pci_get_device(0x8086,0x10d3,NULL);
cpu_addr = dma_alloc_coherent(&(pdev->dev),0x1000, &dma_handle, GFP_KERNEL);
add_DMA_blk(cpu_addr,0x1de3c000,dma_handle);

pdev = pci_get_device(0x8086,0x10d3,NULL);
cpu_addr = dma_alloc_coherent(&(pdev->dev),0x1000, &dma_handle, GFP_KERNEL);
add_DMA_blk(cpu_addr,0x1dea2000,dma_handle);

dev = load_bar(0x8086,0x10d3);
memcpy(bar_addr,dev->cfg.BAR,sizeof(bar_addr));
writel(0x4380000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
writel(0x21,(void __iomem*)bar_addr[0]+0x14);
readl((void __iomem*)bar_addr[0]+0x14);
readl((void __iomem*)bar_addr[0]+0x14);

writel(0x25,(void __iomem*)bar_addr[0]+0x14);
readl((void __iomem*)bar_addr[0]+0x14);
readl((void __iomem*)bar_addr[0]+0x14);
readl((void __iomem*)bar_addr[0]+0x18);

writel(0x10400000,(void __iomem*)bar_addr[0]+0x18);
writel(0x20,(void __iomem*)bar_addr[0]+0x1000);
readl((void __iomem*)bar_addr[0]+0x1000);
readl((void __iomem*)bar_addr[0]+0x0);

writel(0x18140245,(void __iomem*)bar_addr[0]+0x0);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0xffffffff,(void __iomem*)bar_addr[0]+0xd8);
writel(0x0,(void __iomem*)bar_addr[0]+0x100);
readl((void __iomem*)bar_addr[0]+0x400);

writel(0x0,(void __iomem*)bar_addr[0]+0x400);
readl((void __iomem*)bar_addr[0]+0x8);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0x0);

writel(0x1c140245,(void __iomem*)bar_addr[0]+0x0);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0x10);

writel(0xffffffff,(void __iomem*)bar_addr[0]+0xd8);
readl((void __iomem*)bar_addr[0]+0xc0);
readl((void __iomem*)bar_addr[0]+0x18);

writel(0x10400000,(void __iomem*)bar_addr[0]+0x18);
writel(0x0,(void __iomem*)bar_addr[0]+0x5800);
readl((void __iomem*)bar_addr[0]+0x3828);

writel(0x1410000,(void __iomem*)bar_addr[0]+0x3828);
readl((void __iomem*)bar_addr[0]+0x3928);

writel(0x400000,(void __iomem*)bar_addr[0]+0x3928);
readl((void __iomem*)bar_addr[0]+0x3840);

writel(0x4000403,(void __iomem*)bar_addr[0]+0x3840);
readl((void __iomem*)bar_addr[0]+0x3940);
readl((void __iomem*)bar_addr[0]+0x0);

writel(0x18140245,(void __iomem*)bar_addr[0]+0x0);
readl((void __iomem*)bar_addr[0]+0x18);

writel(0x10400000,(void __iomem*)bar_addr[0]+0x18);
readl((void __iomem*)bar_addr[0]+0x5b00);

writel(0xec00200,(void __iomem*)bar_addr[0]+0x5b00);
readl((void __iomem*)bar_addr[0]+0x5b64);

writel(0x1,(void __iomem*)bar_addr[0]+0x5b64);
writel(0x11,(void __iomem*)bar_addr[0]+0x14);
readl((void __iomem*)bar_addr[0]+0x14);
readl((void __iomem*)bar_addr[0]+0x14);
readl((void __iomem*)bar_addr[0]+0xe00);

writel(0x0,(void __iomem*)bar_addr[0]+0x5600);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5604);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5608);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x560c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5610);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5614);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5618);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x561c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5620);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5624);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5628);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x562c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5630);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5634);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5638);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x563c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5640);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5644);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5648);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x564c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5650);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5654);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5658);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x565c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5660);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5664);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5668);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x566c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5670);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5674);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5678);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x567c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5680);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5684);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5688);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x568c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5690);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5694);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5698);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x569c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56a0);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56a4);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56a8);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56ac);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56b0);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56b4);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56b8);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56bc);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56c0);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56c4);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56c8);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56cc);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56d0);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56d4);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56d8);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56dc);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56e0);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56e4);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56e8);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56ec);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56f0);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56f4);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56f8);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x56fc);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5700);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5704);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5708);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x570c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5710);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5714);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5718);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x571c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5720);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5724);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5728);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x572c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5730);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5734);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5738);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x573c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5740);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5744);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5748);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x574c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5750);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5754);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5758);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x575c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5760);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5764);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5768);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x576c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5770);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5774);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5778);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x577c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5780);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5784);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5788);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x578c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5790);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5794);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5798);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x579c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57a0);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57a4);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57a8);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57ac);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57b0);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57b4);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57b8);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57bc);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57c0);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57c4);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57c8);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57cc);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57d0);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57d4);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57d8);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57dc);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57e0);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57e4);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57e8);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57ec);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57f0);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57f4);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57f8);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x57fc);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x12005452,(void __iomem*)bar_addr[0]+0x5400);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x80005634,(void __iomem*)bar_addr[0]+0x5404);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5408);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x540c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5410);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5414);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5418);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x541c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5420);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5424);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5428);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x542c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5430);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5434);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5438);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x543c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5440);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5444);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5448);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x544c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5450);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5454);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5458);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x545c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5460);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5464);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5468);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x546c);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5470);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5474);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x5200);
writel(0x0,(void __iomem*)bar_addr[0]+0x5204);
writel(0x0,(void __iomem*)bar_addr[0]+0x5208);
writel(0x0,(void __iomem*)bar_addr[0]+0x520c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5210);
writel(0x0,(void __iomem*)bar_addr[0]+0x5214);
writel(0x0,(void __iomem*)bar_addr[0]+0x5218);
writel(0x0,(void __iomem*)bar_addr[0]+0x521c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5220);
writel(0x0,(void __iomem*)bar_addr[0]+0x5224);
writel(0x0,(void __iomem*)bar_addr[0]+0x5228);
writel(0x0,(void __iomem*)bar_addr[0]+0x522c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5230);
writel(0x0,(void __iomem*)bar_addr[0]+0x5234);
writel(0x0,(void __iomem*)bar_addr[0]+0x5238);
writel(0x0,(void __iomem*)bar_addr[0]+0x523c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5240);
writel(0x0,(void __iomem*)bar_addr[0]+0x5244);
writel(0x0,(void __iomem*)bar_addr[0]+0x5248);
writel(0x0,(void __iomem*)bar_addr[0]+0x524c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5250);
writel(0x0,(void __iomem*)bar_addr[0]+0x5254);
writel(0x0,(void __iomem*)bar_addr[0]+0x5258);
writel(0x0,(void __iomem*)bar_addr[0]+0x525c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5260);
writel(0x0,(void __iomem*)bar_addr[0]+0x5264);
writel(0x0,(void __iomem*)bar_addr[0]+0x5268);
writel(0x0,(void __iomem*)bar_addr[0]+0x526c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5270);
writel(0x0,(void __iomem*)bar_addr[0]+0x5274);
writel(0x0,(void __iomem*)bar_addr[0]+0x5278);
writel(0x0,(void __iomem*)bar_addr[0]+0x527c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5280);
writel(0x0,(void __iomem*)bar_addr[0]+0x5284);
writel(0x0,(void __iomem*)bar_addr[0]+0x5288);
writel(0x0,(void __iomem*)bar_addr[0]+0x528c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5290);
writel(0x0,(void __iomem*)bar_addr[0]+0x5294);
writel(0x0,(void __iomem*)bar_addr[0]+0x5298);
writel(0x0,(void __iomem*)bar_addr[0]+0x529c);
writel(0x0,(void __iomem*)bar_addr[0]+0x52a0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52a4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52a8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52ac);
writel(0x0,(void __iomem*)bar_addr[0]+0x52b0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52b4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52b8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52bc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52c0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52c4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52c8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52cc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52d0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52d4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52d8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52dc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52e0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52e4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52e8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52ec);
writel(0x0,(void __iomem*)bar_addr[0]+0x52f0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52f4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52f8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52fc);
writel(0x0,(void __iomem*)bar_addr[0]+0x5300);
writel(0x0,(void __iomem*)bar_addr[0]+0x5304);
writel(0x0,(void __iomem*)bar_addr[0]+0x5308);
writel(0x0,(void __iomem*)bar_addr[0]+0x530c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5310);
writel(0x0,(void __iomem*)bar_addr[0]+0x5314);
writel(0x0,(void __iomem*)bar_addr[0]+0x5318);
writel(0x0,(void __iomem*)bar_addr[0]+0x531c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5320);
writel(0x0,(void __iomem*)bar_addr[0]+0x5324);
writel(0x0,(void __iomem*)bar_addr[0]+0x5328);
writel(0x0,(void __iomem*)bar_addr[0]+0x532c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5330);
writel(0x0,(void __iomem*)bar_addr[0]+0x5334);
writel(0x0,(void __iomem*)bar_addr[0]+0x5338);
writel(0x0,(void __iomem*)bar_addr[0]+0x533c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5340);
writel(0x0,(void __iomem*)bar_addr[0]+0x5344);
writel(0x0,(void __iomem*)bar_addr[0]+0x5348);
writel(0x0,(void __iomem*)bar_addr[0]+0x534c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5350);
writel(0x0,(void __iomem*)bar_addr[0]+0x5354);
writel(0x0,(void __iomem*)bar_addr[0]+0x5358);
writel(0x0,(void __iomem*)bar_addr[0]+0x535c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5360);
writel(0x0,(void __iomem*)bar_addr[0]+0x5364);
writel(0x0,(void __iomem*)bar_addr[0]+0x5368);
writel(0x0,(void __iomem*)bar_addr[0]+0x536c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5370);
writel(0x0,(void __iomem*)bar_addr[0]+0x5374);
writel(0x0,(void __iomem*)bar_addr[0]+0x5378);
writel(0x0,(void __iomem*)bar_addr[0]+0x537c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5380);
writel(0x0,(void __iomem*)bar_addr[0]+0x5384);
writel(0x0,(void __iomem*)bar_addr[0]+0x5388);
writel(0x0,(void __iomem*)bar_addr[0]+0x538c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5390);
writel(0x0,(void __iomem*)bar_addr[0]+0x5394);
writel(0x0,(void __iomem*)bar_addr[0]+0x5398);
writel(0x0,(void __iomem*)bar_addr[0]+0x539c);
writel(0x0,(void __iomem*)bar_addr[0]+0x53a0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53a4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53a8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53ac);
writel(0x0,(void __iomem*)bar_addr[0]+0x53b0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53b4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53b8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53bc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53c0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53c4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53c8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53cc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53d0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53d4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53d8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53dc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53e0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53e4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53e8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53ec);
writel(0x0,(void __iomem*)bar_addr[0]+0x53f0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53f4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53f8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53fc);
readl((void __iomem*)bar_addr[0]+0x5820);
readl((void __iomem*)bar_addr[0]+0x0);

writel(0x18140245,(void __iomem*)bar_addr[0]+0x0);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x8300000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x4303360,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8200000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4209940,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x4303b60,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x43d0003,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x43e0000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8200000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4209940,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8240000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8290000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4240de1,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4290e00,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8200000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4201b40,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
writel(0x8808,(void __iomem*)bar_addr[0]+0x30);
writel(0x100,(void __iomem*)bar_addr[0]+0x2c);
writel(0xc28001,(void __iomem*)bar_addr[0]+0x28);
writel(0x680,(void __iomem*)bar_addr[0]+0x170);
writel(0x80007328,(void __iomem*)bar_addr[0]+0x2160);
writel(0x7330,(void __iomem*)bar_addr[0]+0x2168);
readl((void __iomem*)bar_addr[0]+0x3828);

writel(0x1410000,(void __iomem*)bar_addr[0]+0x3828);
readl((void __iomem*)bar_addr[0]+0x5b00);

writel(0xec00200,(void __iomem*)bar_addr[0]+0x5b00);
readl((void __iomem*)bar_addr[0]+0x4000);
readl((void __iomem*)bar_addr[0]+0x4008);
readl((void __iomem*)bar_addr[0]+0x4010);
readl((void __iomem*)bar_addr[0]+0x4014);
readl((void __iomem*)bar_addr[0]+0x4018);
readl((void __iomem*)bar_addr[0]+0x401c);
readl((void __iomem*)bar_addr[0]+0x4020);
readl((void __iomem*)bar_addr[0]+0x4028);
readl((void __iomem*)bar_addr[0]+0x4030);
readl((void __iomem*)bar_addr[0]+0x4038);
readl((void __iomem*)bar_addr[0]+0x4040);
readl((void __iomem*)bar_addr[0]+0x4048);
readl((void __iomem*)bar_addr[0]+0x404c);
readl((void __iomem*)bar_addr[0]+0x4050);
readl((void __iomem*)bar_addr[0]+0x4054);
readl((void __iomem*)bar_addr[0]+0x4058);
readl((void __iomem*)bar_addr[0]+0x4074);
readl((void __iomem*)bar_addr[0]+0x4078);
readl((void __iomem*)bar_addr[0]+0x407c);
readl((void __iomem*)bar_addr[0]+0x4080);
readl((void __iomem*)bar_addr[0]+0x4088);
readl((void __iomem*)bar_addr[0]+0x408c);
readl((void __iomem*)bar_addr[0]+0x4090);
readl((void __iomem*)bar_addr[0]+0x4094);
readl((void __iomem*)bar_addr[0]+0x40a0);
readl((void __iomem*)bar_addr[0]+0x40a4);
readl((void __iomem*)bar_addr[0]+0x40a8);
readl((void __iomem*)bar_addr[0]+0x40ac);
readl((void __iomem*)bar_addr[0]+0x40b0);
readl((void __iomem*)bar_addr[0]+0x40c0);
readl((void __iomem*)bar_addr[0]+0x40c4);
readl((void __iomem*)bar_addr[0]+0x40c8);
readl((void __iomem*)bar_addr[0]+0x40cc);
readl((void __iomem*)bar_addr[0]+0x40d0);
readl((void __iomem*)bar_addr[0]+0x40d4);
readl((void __iomem*)bar_addr[0]+0x40f0);
readl((void __iomem*)bar_addr[0]+0x40f4);
readl((void __iomem*)bar_addr[0]+0x405c);
readl((void __iomem*)bar_addr[0]+0x4060);
readl((void __iomem*)bar_addr[0]+0x4064);
readl((void __iomem*)bar_addr[0]+0x4068);
readl((void __iomem*)bar_addr[0]+0x406c);
readl((void __iomem*)bar_addr[0]+0x4070);
readl((void __iomem*)bar_addr[0]+0x40d8);
readl((void __iomem*)bar_addr[0]+0x40dc);
readl((void __iomem*)bar_addr[0]+0x40e0);
readl((void __iomem*)bar_addr[0]+0x40e4);
readl((void __iomem*)bar_addr[0]+0x40e8);
readl((void __iomem*)bar_addr[0]+0x40ec);
readl((void __iomem*)bar_addr[0]+0x4004);
readl((void __iomem*)bar_addr[0]+0x400c);
readl((void __iomem*)bar_addr[0]+0x4034);
readl((void __iomem*)bar_addr[0]+0x403c);
readl((void __iomem*)bar_addr[0]+0x40f8);
readl((void __iomem*)bar_addr[0]+0x40fc);
readl((void __iomem*)bar_addr[0]+0x40b4);
readl((void __iomem*)bar_addr[0]+0x40b8);
readl((void __iomem*)bar_addr[0]+0x40bc);
readl((void __iomem*)bar_addr[0]+0x4100);
readl((void __iomem*)bar_addr[0]+0x4124);
readl((void __iomem*)bar_addr[0]+0x4104);
readl((void __iomem*)bar_addr[0]+0x4108);
readl((void __iomem*)bar_addr[0]+0x410c);
readl((void __iomem*)bar_addr[0]+0x4110);
readl((void __iomem*)bar_addr[0]+0x4118);
readl((void __iomem*)bar_addr[0]+0x411c);
readl((void __iomem*)bar_addr[0]+0x4120);

writel(0x8100,(void __iomem*)bar_addr[0]+0x38);
writel(0x0,(void __iomem*)bar_addr[0]+0x458);
writel(0x1a00000,(void __iomem*)bar_addr[0]+0xb608);
readl((void __iomem*)bar_addr[0]+0xb600);
readl((void __iomem*)bar_addr[0]+0xb604);
readl((void __iomem*)bar_addr[0]+0xb608);
readl((void __iomem*)bar_addr[0]+0xb600);
readl((void __iomem*)bar_addr[0]+0xb604);
readl((void __iomem*)bar_addr[0]+0xb614);

writel(0x0,(void __iomem*)bar_addr[0]+0xb614);
readl((void __iomem*)bar_addr[0]+0xb614);
readl((void __iomem*)bar_addr[0]+0xb620);

writel(0x0,(void __iomem*)bar_addr[0]+0xb620);
readl((void __iomem*)bar_addr[0]+0xb620);

writel(0x0,(void __iomem*)bar_addr[0]+0xb634);
writel(0x0,(void __iomem*)bar_addr[0]+0xb638);
readl((void __iomem*)bar_addr[0]+0x8);
readl((void __iomem*)bar_addr[0]+0xb628);
readl((void __iomem*)bar_addr[0]+0xb61c);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x8390000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x4390000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8200000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4201140,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0x5820);
readl((void __iomem*)bar_addr[0]+0x0);

writel(0x18140245,(void __iomem*)bar_addr[0]+0x0);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x8300000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x4303360,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8200000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4209140,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x4303b60,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x43d0003,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x43e0000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8200000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4209140,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8240000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8290000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4240de1,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4290e00,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8200000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4201340,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
writel(0x8808,(void __iomem*)bar_addr[0]+0x30);
writel(0x100,(void __iomem*)bar_addr[0]+0x2c);
writel(0xc28001,(void __iomem*)bar_addr[0]+0x28);
writel(0x680,(void __iomem*)bar_addr[0]+0x170);
writel(0x80007328,(void __iomem*)bar_addr[0]+0x2160);
writel(0x7330,(void __iomem*)bar_addr[0]+0x2168);
readl((void __iomem*)bar_addr[0]+0x100);

writel(0x0,(void __iomem*)bar_addr[0]+0x53fc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53f8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53f4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53f0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53ec);
writel(0x0,(void __iomem*)bar_addr[0]+0x53e8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53e4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53e0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53dc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53d8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53d4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53d0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53cc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53c8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53c4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53c0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53bc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53b8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53b4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53b0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53ac);
writel(0x0,(void __iomem*)bar_addr[0]+0x53a8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53a4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53a0);
writel(0x0,(void __iomem*)bar_addr[0]+0x539c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5398);
writel(0x0,(void __iomem*)bar_addr[0]+0x5394);
writel(0x0,(void __iomem*)bar_addr[0]+0x5390);
writel(0x0,(void __iomem*)bar_addr[0]+0x538c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5388);
writel(0x0,(void __iomem*)bar_addr[0]+0x5384);
writel(0x0,(void __iomem*)bar_addr[0]+0x5380);
writel(0x0,(void __iomem*)bar_addr[0]+0x537c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5378);
writel(0x0,(void __iomem*)bar_addr[0]+0x5374);
writel(0x0,(void __iomem*)bar_addr[0]+0x5370);
writel(0x0,(void __iomem*)bar_addr[0]+0x536c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5368);
writel(0x0,(void __iomem*)bar_addr[0]+0x5364);
writel(0x0,(void __iomem*)bar_addr[0]+0x5360);
writel(0x0,(void __iomem*)bar_addr[0]+0x535c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5358);
writel(0x0,(void __iomem*)bar_addr[0]+0x5354);
writel(0x0,(void __iomem*)bar_addr[0]+0x5350);
writel(0x0,(void __iomem*)bar_addr[0]+0x534c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5348);
writel(0x0,(void __iomem*)bar_addr[0]+0x5344);
writel(0x0,(void __iomem*)bar_addr[0]+0x5340);
writel(0x0,(void __iomem*)bar_addr[0]+0x533c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5338);
writel(0x0,(void __iomem*)bar_addr[0]+0x5334);
writel(0x0,(void __iomem*)bar_addr[0]+0x5330);
writel(0x0,(void __iomem*)bar_addr[0]+0x532c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5328);
writel(0x0,(void __iomem*)bar_addr[0]+0x5324);
writel(0x0,(void __iomem*)bar_addr[0]+0x5320);
writel(0x0,(void __iomem*)bar_addr[0]+0x531c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5318);
writel(0x0,(void __iomem*)bar_addr[0]+0x5314);
writel(0x0,(void __iomem*)bar_addr[0]+0x5310);
writel(0x0,(void __iomem*)bar_addr[0]+0x530c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5308);
writel(0x0,(void __iomem*)bar_addr[0]+0x5304);
writel(0x0,(void __iomem*)bar_addr[0]+0x5300);
writel(0x0,(void __iomem*)bar_addr[0]+0x52fc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52f8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52f4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52f0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52ec);
writel(0x0,(void __iomem*)bar_addr[0]+0x52e8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52e4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52e0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52dc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52d8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52d4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52d0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52cc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52c8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52c4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52c0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52bc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52b8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52b4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52b0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52ac);
writel(0x0,(void __iomem*)bar_addr[0]+0x52a8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52a4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52a0);
writel(0x0,(void __iomem*)bar_addr[0]+0x529c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5298);
writel(0x0,(void __iomem*)bar_addr[0]+0x5294);
writel(0x0,(void __iomem*)bar_addr[0]+0x5290);
writel(0x0,(void __iomem*)bar_addr[0]+0x528c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5288);
writel(0x0,(void __iomem*)bar_addr[0]+0x5284);
writel(0x0,(void __iomem*)bar_addr[0]+0x5280);
writel(0x0,(void __iomem*)bar_addr[0]+0x527c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5278);
writel(0x0,(void __iomem*)bar_addr[0]+0x5274);
writel(0x0,(void __iomem*)bar_addr[0]+0x5270);
writel(0x0,(void __iomem*)bar_addr[0]+0x526c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5268);
writel(0x0,(void __iomem*)bar_addr[0]+0x5264);
writel(0x0,(void __iomem*)bar_addr[0]+0x5260);
writel(0x0,(void __iomem*)bar_addr[0]+0x525c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5258);
writel(0x0,(void __iomem*)bar_addr[0]+0x5254);
writel(0x0,(void __iomem*)bar_addr[0]+0x5250);
writel(0x0,(void __iomem*)bar_addr[0]+0x524c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5248);
writel(0x0,(void __iomem*)bar_addr[0]+0x5244);
writel(0x0,(void __iomem*)bar_addr[0]+0x5240);
writel(0x0,(void __iomem*)bar_addr[0]+0x523c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5238);
writel(0x0,(void __iomem*)bar_addr[0]+0x5234);
writel(0x0,(void __iomem*)bar_addr[0]+0x5230);
writel(0x0,(void __iomem*)bar_addr[0]+0x522c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5228);
writel(0x0,(void __iomem*)bar_addr[0]+0x5224);
writel(0x0,(void __iomem*)bar_addr[0]+0x5220);
writel(0x0,(void __iomem*)bar_addr[0]+0x521c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5218);
writel(0x0,(void __iomem*)bar_addr[0]+0x5214);
writel(0x0,(void __iomem*)bar_addr[0]+0x5210);
writel(0x0,(void __iomem*)bar_addr[0]+0x520c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5208);
writel(0x0,(void __iomem*)bar_addr[0]+0x5204);
writel(0x10000,(void __iomem*)bar_addr[0]+0x5200);
readl((void __iomem*)bar_addr[0]+0x8);
readl((void __iomem*)bar_addr[0]+0x100);

writel(0x40000,(void __iomem*)bar_addr[0]+0x100);
writel(0x0,(void __iomem*)bar_addr[0]+0x5474);
writel(0x0,(void __iomem*)bar_addr[0]+0x5470);
writel(0x0,(void __iomem*)bar_addr[0]+0x546c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5468);
writel(0x0,(void __iomem*)bar_addr[0]+0x5464);
writel(0x0,(void __iomem*)bar_addr[0]+0x5460);
writel(0x0,(void __iomem*)bar_addr[0]+0x545c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5458);
writel(0x0,(void __iomem*)bar_addr[0]+0x5454);
writel(0x0,(void __iomem*)bar_addr[0]+0x5450);
writel(0x0,(void __iomem*)bar_addr[0]+0x544c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5448);
writel(0x0,(void __iomem*)bar_addr[0]+0x5444);
writel(0x0,(void __iomem*)bar_addr[0]+0x5440);
writel(0x0,(void __iomem*)bar_addr[0]+0x543c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5438);
writel(0x0,(void __iomem*)bar_addr[0]+0x5434);
writel(0x0,(void __iomem*)bar_addr[0]+0x5430);
writel(0x0,(void __iomem*)bar_addr[0]+0x542c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5428);
writel(0x0,(void __iomem*)bar_addr[0]+0x5424);
writel(0x0,(void __iomem*)bar_addr[0]+0x5420);
writel(0x0,(void __iomem*)bar_addr[0]+0x541c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5418);
writel(0x0,(void __iomem*)bar_addr[0]+0x5414);
writel(0x0,(void __iomem*)bar_addr[0]+0x5410);
writel(0x0,(void __iomem*)bar_addr[0]+0x540c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5408);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x0,(void __iomem*)bar_addr[0]+0x100);
readl((void __iomem*)bar_addr[0]+0x0);

writel(0x58140245,(void __iomem*)bar_addr[0]+0x0);
readl((void __iomem*)bar_addr[0]+0x5600);

writel(0x1,(void __iomem*)bar_addr[0]+0x5600);
readl((void __iomem*)bar_addr[0]+0x8);
readl((void __iomem*)bar_addr[0]+0x5600);

writel(0x1,(void __iomem*)bar_addr[0]+0x5600);
readl((void __iomem*)bar_addr[0]+0x8);

blk = get_DMA_blk(0x1dea2000);
writel(blk->dma_handle,(void __iomem*)bar_addr[0]+0x3800);
writel(0x0,(void __iomem*)bar_addr[0]+0x3804);
writel(0x1000,(void __iomem*)bar_addr[0]+0x3808);
writel(0x0,(void __iomem*)bar_addr[0]+0x3810);
writel(0x0,(void __iomem*)bar_addr[0]+0x3818);
writel(0x0,(void __iomem*)bar_addr[0]+0x3810);
writel(0x0,(void __iomem*)bar_addr[0]+0x3818);
writel(0x8,(void __iomem*)bar_addr[0]+0x3820);
writel(0x20,(void __iomem*)bar_addr[0]+0x382c);
readl((void __iomem*)bar_addr[0]+0x3828);

writel(0x141011f,(void __iomem*)bar_addr[0]+0x3828);
readl((void __iomem*)bar_addr[0]+0x3828);

writel(0x141011f,(void __iomem*)bar_addr[0]+0x3928);
readl((void __iomem*)bar_addr[0]+0x400);

writel(0x10000f8,(void __iomem*)bar_addr[0]+0x400);
readl((void __iomem*)bar_addr[0]+0x400);

writel(0x103f0f8,(void __iomem*)bar_addr[0]+0x400);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0xb028ebe2,(void __iomem*)bar_addr[0]+0x5c80);
writel(0xa85f3951,(void __iomem*)bar_addr[0]+0x5c84);
writel(0x1d61e42d,(void __iomem*)bar_addr[0]+0x5c88);
writel(0x661e0ab,(void __iomem*)bar_addr[0]+0x5c8c);
writel(0x5542ca31,(void __iomem*)bar_addr[0]+0x5c90);
writel(0xa1cd6213,(void __iomem*)bar_addr[0]+0x5c94);
writel(0xf31fd576,(void __iomem*)bar_addr[0]+0x5c98);
writel(0x71cfc95a,(void __iomem*)bar_addr[0]+0x5c9c);
writel(0xd394d0cb,(void __iomem*)bar_addr[0]+0x5ca0);
writel(0xf22e4b83,(void __iomem*)bar_addr[0]+0x5ca4);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c00);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c04);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c08);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c0c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c10);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c14);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c18);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c1c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c20);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c24);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c28);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c2c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c30);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c34);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c38);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c3c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c40);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c44);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c48);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c4c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c50);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c54);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c58);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c5c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c60);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c64);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c68);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c6c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c70);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c74);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c78);
writel(0x0,(void __iomem*)bar_addr[0]+0x5c7c);
readl((void __iomem*)bar_addr[0]+0x5000);

writel(0x2300,(void __iomem*)bar_addr[0]+0x5000);
writel(0x370000,(void __iomem*)bar_addr[0]+0x5818);
readl((void __iomem*)bar_addr[0]+0x100);
readl((void __iomem*)bar_addr[0]+0x5008);

writel(0x8000,(void __iomem*)bar_addr[0]+0x5008);
writel(0x4008002,(void __iomem*)bar_addr[0]+0x100);
readl((void __iomem*)bar_addr[0]+0x100);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x1040420,(void __iomem*)bar_addr[0]+0x2828);
writel(0x1040420,(void __iomem*)bar_addr[0]+0x2928);
writel(0x20,(void __iomem*)bar_addr[0]+0x2820);
writel(0x20,(void __iomem*)bar_addr[0]+0x282c);
writel(0xc3,(void __iomem*)bar_addr[0]+0xe8);
writel(0xc3,(void __iomem*)bar_addr[0]+0xec);
writel(0xc3,(void __iomem*)bar_addr[0]+0xf0);
readl((void __iomem*)bar_addr[0]+0x18);

writel(0xffffffff,(void __iomem*)bar_addr[0]+0xe0);
writel(0x18400000,(void __iomem*)bar_addr[0]+0x18);
readl((void __iomem*)bar_addr[0]+0x8);

blk = get_DMA_blk(0x1de3c000);
writel(blk->dma_handle,(void __iomem*)bar_addr[0]+0x2800);
writel(0x0,(void __iomem*)bar_addr[0]+0x2804);
writel(0x1000,(void __iomem*)bar_addr[0]+0x2808);
writel(0x0,(void __iomem*)bar_addr[0]+0x2810);
writel(0x0,(void __iomem*)bar_addr[0]+0x2818);
writel(0x0,(void __iomem*)bar_addr[0]+0x2810);
writel(0x0,(void __iomem*)bar_addr[0]+0x2818);
readl((void __iomem*)bar_addr[0]+0x5000);

writel(0x2300,(void __iomem*)bar_addr[0]+0x5000);
writel(0x4008002,(void __iomem*)bar_addr[0]+0x100);
blk = get_DMA_blk(0x1de3c000);
writel(0x1dc23840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x0,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1dc23040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1dff8040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1dff8840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1dff9040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1dff9840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1dffa040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1dffa840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1dffb040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1dffb840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de84040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de84840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de85040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de85840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de86040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de86840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de87040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x10,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1de87840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de70040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de70840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de71040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de71840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de72040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de72840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de73040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de73840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4d0040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4d0840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4d1040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4d1840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4d2040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4d2840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4d3040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x20,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1e4d3840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de44040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de44840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de45040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de45840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de46040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de46840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de47040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de47840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c480040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c480840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c481040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c481840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c482040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c482840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c483040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x30,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1c483840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de68040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de68840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de69040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de69840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de6a040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de6a840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de6b040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de6b840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4a8040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4a8840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4a9040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4a9840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4aa040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4aa840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4ab040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x40,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1e4ab840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4ac040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4ac840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4ad040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4ad840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4ae040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4ae840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4af040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1e4af840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de60040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de60840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de61040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de61840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de62040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de62840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de63040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x50,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1de63840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de64040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de64840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de65040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de65840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de66040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de66840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de67040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de67840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c488040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c488840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c489040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c489840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c48a040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c48a840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c48b040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x60,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1c48b840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c48c040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c48c840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c48d040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c48d840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c48e040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c48e840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c48f040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c48f840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de48040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de48840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de49040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de49840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de4a040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de4a840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de4b040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x70,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1de4b840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de4c040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de4c840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de4d040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de4d840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de4e040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de4e840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de4f040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1de4f840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c470040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c470840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c471040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c471840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c472040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c472840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c473040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x80,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1c473840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c474040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c474840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c475040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c475840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c476040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c476840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c477040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c477840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c478040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c478840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c479040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c479840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c47a040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c47a840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c47b040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x90,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1c47b840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c47c040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c47c840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c47d040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c47d840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c47e040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c47e840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c47f040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c47f840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a0040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a0840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a1040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a1840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a2040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a2840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a3040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0xa0,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1c4a3840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a4040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a4840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a5040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a5840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a6040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a6840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a7040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a7840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a8040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a8840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a9040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4a9840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4aa040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4aa840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4ab040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0xb0,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1c4ab840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4ac040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4ac840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4ad040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4ad840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4ae040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4ae840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4af040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4af840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b0040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b0840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b1040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b1840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b2040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b2840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b3040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0xc0,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1c4b3840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b4040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b4840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b5040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b5840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b6040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b6840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b7040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b7840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b8040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b8840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b9040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4b9840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4ba040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4ba840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4bb040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0xd0,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1c4bb840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4bc040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4bc840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4bd040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4bd840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4be040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4be840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4bf040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4bf840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c0040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c0840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c1040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c1840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c2040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c2840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c3040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0xe0,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1c4c3840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c4040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c4840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c5040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c5840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c6040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c6840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c7040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c7840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c8040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c8840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c9040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4c9840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4ca040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4ca840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4cb040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0xf0,(void __iomem*)bar_addr[0]+0x2818);
writel(0x1c4cb840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4cc040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4cc840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4cd040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4cd840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4ce040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4ce840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4cf040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4cf840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4d0040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4d0840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4d1040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4d1840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c4d2040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0xfee01004,(void __iomem*)bar_addr[3]+0x0);
writel(0x0,(void __iomem*)bar_addr[3]+0x4);
writel(0x4027,(void __iomem*)bar_addr[3]+0x8);
writel(0x0,(void __iomem*)bar_addr[3]+0xc);
readl((void __iomem*)bar_addr[3]+0x0);

writel(0xfee01004,(void __iomem*)bar_addr[3]+0x0);
writel(0x0,(void __iomem*)bar_addr[3]+0x4);
writel(0x4027,(void __iomem*)bar_addr[3]+0x8);
writel(0xfee01004,(void __iomem*)bar_addr[3]+0x10);
writel(0x0,(void __iomem*)bar_addr[3]+0x14);
writel(0x4028,(void __iomem*)bar_addr[3]+0x18);
writel(0x0,(void __iomem*)bar_addr[3]+0x1c);
readl((void __iomem*)bar_addr[3]+0x0);

writel(0xfee01004,(void __iomem*)bar_addr[3]+0x10);
writel(0x0,(void __iomem*)bar_addr[3]+0x14);
writel(0x4028,(void __iomem*)bar_addr[3]+0x18);
writel(0xfee01004,(void __iomem*)bar_addr[3]+0x20);
writel(0x0,(void __iomem*)bar_addr[3]+0x24);
writel(0x4029,(void __iomem*)bar_addr[3]+0x28);
writel(0x0,(void __iomem*)bar_addr[3]+0x2c);
readl((void __iomem*)bar_addr[3]+0x0);

writel(0xfee01004,(void __iomem*)bar_addr[3]+0x20);
writel(0x0,(void __iomem*)bar_addr[3]+0x24);
writel(0x4029,(void __iomem*)bar_addr[3]+0x28);
readl((void __iomem*)bar_addr[0]+0x5008);

writel(0x9000,(void __iomem*)bar_addr[0]+0x5008);
writel(0xc3,(void __iomem*)bar_addr[0]+0xe8);
writel(0xc3,(void __iomem*)bar_addr[0]+0xec);
writel(0xc3,(void __iomem*)bar_addr[0]+0xf0);
writel(0x800a0908,(void __iomem*)bar_addr[0]+0xe4);
readl((void __iomem*)bar_addr[0]+0x18);

writel(0x91400000,(void __iomem*)bar_addr[0]+0x18);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x500000,(void __iomem*)bar_addr[0]+0xdc);
writel(0x1570244,(void __iomem*)bar_addr[0]+0xd0);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x1000004,(void __iomem*)bar_addr[0]+0xc8);
readl((void __iomem*)bar_addr[0]+0xc0);

writel(0x1070244,(void __iomem*)bar_addr[0]+0xd0);
readl((void __iomem*)bar_addr[0]+0x100);

writel(0x0,(void __iomem*)bar_addr[0]+0x53fc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53f8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53f4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53f0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53ec);
writel(0x0,(void __iomem*)bar_addr[0]+0x53e8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53e4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53e0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53dc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53d8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53d4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53d0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53cc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53c8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53c4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53c0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53bc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53b8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53b4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53b0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53ac);
writel(0x0,(void __iomem*)bar_addr[0]+0x53a8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53a4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53a0);
writel(0x0,(void __iomem*)bar_addr[0]+0x539c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5398);
writel(0x0,(void __iomem*)bar_addr[0]+0x5394);
writel(0x0,(void __iomem*)bar_addr[0]+0x5390);
writel(0x0,(void __iomem*)bar_addr[0]+0x538c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5388);
writel(0x0,(void __iomem*)bar_addr[0]+0x5384);
writel(0x0,(void __iomem*)bar_addr[0]+0x5380);
writel(0x0,(void __iomem*)bar_addr[0]+0x537c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5378);
writel(0x0,(void __iomem*)bar_addr[0]+0x5374);
writel(0x0,(void __iomem*)bar_addr[0]+0x5370);
writel(0x0,(void __iomem*)bar_addr[0]+0x536c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5368);
writel(0x0,(void __iomem*)bar_addr[0]+0x5364);
writel(0x0,(void __iomem*)bar_addr[0]+0x5360);
writel(0x0,(void __iomem*)bar_addr[0]+0x535c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5358);
writel(0x0,(void __iomem*)bar_addr[0]+0x5354);
writel(0x0,(void __iomem*)bar_addr[0]+0x5350);
writel(0x0,(void __iomem*)bar_addr[0]+0x534c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5348);
writel(0x0,(void __iomem*)bar_addr[0]+0x5344);
writel(0x0,(void __iomem*)bar_addr[0]+0x5340);
writel(0x0,(void __iomem*)bar_addr[0]+0x533c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5338);
writel(0x0,(void __iomem*)bar_addr[0]+0x5334);
writel(0x0,(void __iomem*)bar_addr[0]+0x5330);
writel(0x0,(void __iomem*)bar_addr[0]+0x532c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5328);
writel(0x0,(void __iomem*)bar_addr[0]+0x5324);
writel(0x0,(void __iomem*)bar_addr[0]+0x5320);
writel(0x0,(void __iomem*)bar_addr[0]+0x531c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5318);
writel(0x0,(void __iomem*)bar_addr[0]+0x5314);
writel(0x0,(void __iomem*)bar_addr[0]+0x5310);
writel(0x0,(void __iomem*)bar_addr[0]+0x530c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5308);
writel(0x0,(void __iomem*)bar_addr[0]+0x5304);
writel(0x0,(void __iomem*)bar_addr[0]+0x5300);
writel(0x0,(void __iomem*)bar_addr[0]+0x52fc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52f8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52f4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52f0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52ec);
writel(0x0,(void __iomem*)bar_addr[0]+0x52e8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52e4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52e0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52dc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52d8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52d4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52d0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52cc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52c8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52c4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52c0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52bc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52b8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52b4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52b0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52ac);
writel(0x0,(void __iomem*)bar_addr[0]+0x52a8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52a4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52a0);
writel(0x0,(void __iomem*)bar_addr[0]+0x529c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5298);
writel(0x0,(void __iomem*)bar_addr[0]+0x5294);
writel(0x0,(void __iomem*)bar_addr[0]+0x5290);
writel(0x0,(void __iomem*)bar_addr[0]+0x528c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5288);
writel(0x0,(void __iomem*)bar_addr[0]+0x5284);
writel(0x0,(void __iomem*)bar_addr[0]+0x5280);
writel(0x0,(void __iomem*)bar_addr[0]+0x527c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5278);
writel(0x0,(void __iomem*)bar_addr[0]+0x5274);
writel(0x0,(void __iomem*)bar_addr[0]+0x5270);
writel(0x0,(void __iomem*)bar_addr[0]+0x526c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5268);
writel(0x0,(void __iomem*)bar_addr[0]+0x5264);
writel(0x0,(void __iomem*)bar_addr[0]+0x5260);
writel(0x0,(void __iomem*)bar_addr[0]+0x525c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5258);
writel(0x0,(void __iomem*)bar_addr[0]+0x5254);
writel(0x0,(void __iomem*)bar_addr[0]+0x5250);
writel(0x0,(void __iomem*)bar_addr[0]+0x524c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5248);
writel(0x0,(void __iomem*)bar_addr[0]+0x5244);
writel(0x0,(void __iomem*)bar_addr[0]+0x5240);
writel(0x0,(void __iomem*)bar_addr[0]+0x523c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5238);
writel(0x0,(void __iomem*)bar_addr[0]+0x5234);
writel(0x0,(void __iomem*)bar_addr[0]+0x5230);
writel(0x0,(void __iomem*)bar_addr[0]+0x522c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5228);
writel(0x0,(void __iomem*)bar_addr[0]+0x5224);
writel(0x0,(void __iomem*)bar_addr[0]+0x5220);
writel(0x0,(void __iomem*)bar_addr[0]+0x521c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5218);
writel(0x0,(void __iomem*)bar_addr[0]+0x5214);
writel(0x0,(void __iomem*)bar_addr[0]+0x5210);
writel(0x0,(void __iomem*)bar_addr[0]+0x520c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5208);
writel(0x0,(void __iomem*)bar_addr[0]+0x5204);
writel(0x10000,(void __iomem*)bar_addr[0]+0x5200);
readl((void __iomem*)bar_addr[0]+0x8);
readl((void __iomem*)bar_addr[0]+0x100);

writel(0x4048002,(void __iomem*)bar_addr[0]+0x100);
writel(0x0,(void __iomem*)bar_addr[0]+0x5474);
writel(0x0,(void __iomem*)bar_addr[0]+0x5470);
writel(0x0,(void __iomem*)bar_addr[0]+0x546c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5468);
writel(0x0,(void __iomem*)bar_addr[0]+0x5464);
writel(0x0,(void __iomem*)bar_addr[0]+0x5460);
writel(0x0,(void __iomem*)bar_addr[0]+0x545c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5458);
writel(0x0,(void __iomem*)bar_addr[0]+0x5454);
writel(0x0,(void __iomem*)bar_addr[0]+0x5450);
writel(0x0,(void __iomem*)bar_addr[0]+0x544c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5448);
writel(0x0,(void __iomem*)bar_addr[0]+0x5444);
writel(0x0,(void __iomem*)bar_addr[0]+0x5440);
writel(0x0,(void __iomem*)bar_addr[0]+0x543c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5438);
writel(0x0,(void __iomem*)bar_addr[0]+0x5434);
writel(0x0,(void __iomem*)bar_addr[0]+0x5430);
writel(0x0,(void __iomem*)bar_addr[0]+0x542c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5428);
writel(0x0,(void __iomem*)bar_addr[0]+0x5424);
writel(0x0,(void __iomem*)bar_addr[0]+0x5420);
writel(0x0,(void __iomem*)bar_addr[0]+0x541c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5418);
writel(0x0,(void __iomem*)bar_addr[0]+0x5414);
writel(0x0,(void __iomem*)bar_addr[0]+0x5410);
writel(0x0,(void __iomem*)bar_addr[0]+0x540c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5408);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x4008002,(void __iomem*)bar_addr[0]+0x100);
readl((void __iomem*)bar_addr[0]+0x0);

writel(0x58140245,(void __iomem*)bar_addr[0]+0x0);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
writel(0x3d,(void __iomem*)bar_addr[0]+0x14);
readl((void __iomem*)bar_addr[0]+0x14);
readl((void __iomem*)bar_addr[0]+0x14);

writel(0x100000,(void __iomem*)bar_addr[0]+0xc8);
writel(0x80000008,(void __iomem*)bar_addr[0]+0x3820);
writel(0x80000020,(void __iomem*)bar_addr[0]+0x2820);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x80000008,(void __iomem*)bar_addr[0]+0x3820);
writel(0x80000020,(void __iomem*)bar_addr[0]+0x2820);
readl((void __iomem*)bar_addr[0]+0x8);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x8350000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl(blk->cpu_addr);

writel(0x100000,(void __iomem*)bar_addr[0]+0xd0);
readl((void __iomem*)bar_addr[0]+0x100);

writel(0x0,(void __iomem*)bar_addr[0]+0x53fc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53f8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53f4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53f0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53ec);
writel(0x0,(void __iomem*)bar_addr[0]+0x53e8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53e4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53e0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53dc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53d8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53d4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53d0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53cc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53c8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53c4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53c0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53bc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53b8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53b4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53b0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53ac);
writel(0x0,(void __iomem*)bar_addr[0]+0x53a8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53a4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53a0);
writel(0x0,(void __iomem*)bar_addr[0]+0x539c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5398);
writel(0x0,(void __iomem*)bar_addr[0]+0x5394);
writel(0x0,(void __iomem*)bar_addr[0]+0x5390);
writel(0x0,(void __iomem*)bar_addr[0]+0x538c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5388);
writel(0x0,(void __iomem*)bar_addr[0]+0x5384);
writel(0x0,(void __iomem*)bar_addr[0]+0x5380);
writel(0x0,(void __iomem*)bar_addr[0]+0x537c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5378);
writel(0x0,(void __iomem*)bar_addr[0]+0x5374);
writel(0x0,(void __iomem*)bar_addr[0]+0x5370);
writel(0x0,(void __iomem*)bar_addr[0]+0x536c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5368);
writel(0x0,(void __iomem*)bar_addr[0]+0x5364);
writel(0x0,(void __iomem*)bar_addr[0]+0x5360);
writel(0x0,(void __iomem*)bar_addr[0]+0x535c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5358);
writel(0x0,(void __iomem*)bar_addr[0]+0x5354);
writel(0x0,(void __iomem*)bar_addr[0]+0x5350);
writel(0x0,(void __iomem*)bar_addr[0]+0x534c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5348);
writel(0x0,(void __iomem*)bar_addr[0]+0x5344);
writel(0x0,(void __iomem*)bar_addr[0]+0x5340);
writel(0x0,(void __iomem*)bar_addr[0]+0x533c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5338);
writel(0x0,(void __iomem*)bar_addr[0]+0x5334);
writel(0x0,(void __iomem*)bar_addr[0]+0x5330);
writel(0x0,(void __iomem*)bar_addr[0]+0x532c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5328);
writel(0x0,(void __iomem*)bar_addr[0]+0x5324);
writel(0x0,(void __iomem*)bar_addr[0]+0x5320);
writel(0x0,(void __iomem*)bar_addr[0]+0x531c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5318);
writel(0x0,(void __iomem*)bar_addr[0]+0x5314);
writel(0x0,(void __iomem*)bar_addr[0]+0x5310);
writel(0x0,(void __iomem*)bar_addr[0]+0x530c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5308);
writel(0x0,(void __iomem*)bar_addr[0]+0x5304);
writel(0x0,(void __iomem*)bar_addr[0]+0x5300);
writel(0x0,(void __iomem*)bar_addr[0]+0x52fc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52f8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52f4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52f0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52ec);
writel(0x0,(void __iomem*)bar_addr[0]+0x52e8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52e4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52e0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52dc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52d8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52d4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52d0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52cc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52c8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52c4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52c0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52bc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52b8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52b4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52b0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52ac);
writel(0x0,(void __iomem*)bar_addr[0]+0x52a8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52a4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52a0);
writel(0x0,(void __iomem*)bar_addr[0]+0x529c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5298);
writel(0x0,(void __iomem*)bar_addr[0]+0x5294);
writel(0x0,(void __iomem*)bar_addr[0]+0x5290);
writel(0x0,(void __iomem*)bar_addr[0]+0x528c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5288);
writel(0x0,(void __iomem*)bar_addr[0]+0x5284);
writel(0x0,(void __iomem*)bar_addr[0]+0x5280);
writel(0x0,(void __iomem*)bar_addr[0]+0x527c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5278);
writel(0x0,(void __iomem*)bar_addr[0]+0x5274);
writel(0x0,(void __iomem*)bar_addr[0]+0x5270);
writel(0x0,(void __iomem*)bar_addr[0]+0x526c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5268);
writel(0x0,(void __iomem*)bar_addr[0]+0x5264);
writel(0x0,(void __iomem*)bar_addr[0]+0x5260);
writel(0x0,(void __iomem*)bar_addr[0]+0x525c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5258);
writel(0x0,(void __iomem*)bar_addr[0]+0x5254);
writel(0x0,(void __iomem*)bar_addr[0]+0x5250);
writel(0x0,(void __iomem*)bar_addr[0]+0x524c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5248);
writel(0x0,(void __iomem*)bar_addr[0]+0x5244);
writel(0x0,(void __iomem*)bar_addr[0]+0x5240);
writel(0x0,(void __iomem*)bar_addr[0]+0x523c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5238);
writel(0x0,(void __iomem*)bar_addr[0]+0x5234);
writel(0x0,(void __iomem*)bar_addr[0]+0x5230);
writel(0x0,(void __iomem*)bar_addr[0]+0x522c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5228);
writel(0x0,(void __iomem*)bar_addr[0]+0x5224);
writel(0x0,(void __iomem*)bar_addr[0]+0x5220);
writel(0x0,(void __iomem*)bar_addr[0]+0x521c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5218);
writel(0x0,(void __iomem*)bar_addr[0]+0x5214);
writel(0x0,(void __iomem*)bar_addr[0]+0x5210);
writel(0x0,(void __iomem*)bar_addr[0]+0x520c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5208);
writel(0x0,(void __iomem*)bar_addr[0]+0x5204);
writel(0x10000,(void __iomem*)bar_addr[0]+0x5200);
readl((void __iomem*)bar_addr[0]+0x8);
readl((void __iomem*)bar_addr[0]+0x100);

writel(0x4048002,(void __iomem*)bar_addr[0]+0x100);
writel(0x0,(void __iomem*)bar_addr[0]+0x5474);
writel(0x0,(void __iomem*)bar_addr[0]+0x5470);
writel(0x0,(void __iomem*)bar_addr[0]+0x546c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5468);
writel(0x0,(void __iomem*)bar_addr[0]+0x5464);
writel(0x0,(void __iomem*)bar_addr[0]+0x5460);
writel(0x0,(void __iomem*)bar_addr[0]+0x545c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5458);
writel(0x0,(void __iomem*)bar_addr[0]+0x5454);
writel(0x0,(void __iomem*)bar_addr[0]+0x5450);
writel(0x0,(void __iomem*)bar_addr[0]+0x544c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5448);
writel(0x0,(void __iomem*)bar_addr[0]+0x5444);
writel(0x0,(void __iomem*)bar_addr[0]+0x5440);
writel(0x0,(void __iomem*)bar_addr[0]+0x543c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5438);
writel(0x0,(void __iomem*)bar_addr[0]+0x5434);
writel(0x0,(void __iomem*)bar_addr[0]+0x5430);
writel(0x0,(void __iomem*)bar_addr[0]+0x542c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5428);
writel(0x0,(void __iomem*)bar_addr[0]+0x5424);
writel(0x0,(void __iomem*)bar_addr[0]+0x5420);
writel(0x0,(void __iomem*)bar_addr[0]+0x541c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5418);
writel(0x0,(void __iomem*)bar_addr[0]+0x5414);
writel(0x0,(void __iomem*)bar_addr[0]+0x5410);
writel(0x0,(void __iomem*)bar_addr[0]+0x540c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5408);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x4008002,(void __iomem*)bar_addr[0]+0x100);
readl((void __iomem*)bar_addr[0]+0x0);

writel(0x58140245,(void __iomem*)bar_addr[0]+0x0);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x8310000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0x400);

writel(0x103f0f8,(void __iomem*)bar_addr[0]+0x400);
readl((void __iomem*)bar_addr[0]+0x8);
readl((void __iomem*)bar_addr[0]+0x0);

writel(0x58140245,(void __iomem*)bar_addr[0]+0x0);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8240000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8250000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0x8);
readl((void __iomem*)bar_addr[0]+0x0);

writel(0x58140245,(void __iomem*)bar_addr[0]+0x0);
writel(0x3d,(void __iomem*)bar_addr[0]+0x14);
readl((void __iomem*)bar_addr[0]+0x14);
readl((void __iomem*)bar_addr[0]+0x14);
readl((void __iomem*)bar_addr[0]+0x8);
readl((void __iomem*)bar_addr[0]+0x8);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8200000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8240000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8250000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8260000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8290000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x82a0000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x82f0000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0x8);
readl((void __iomem*)bar_addr[0]+0x0);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x8310000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0x400);

writel(0x103f0fa,(void __iomem*)bar_addr[0]+0x400);
readl((void __iomem*)bar_addr[0]+0x4000);
readl((void __iomem*)bar_addr[0]+0x4074);
readl((void __iomem*)bar_addr[0]+0x4088);
readl((void __iomem*)bar_addr[0]+0x408c);
readl((void __iomem*)bar_addr[0]+0x4078);
readl((void __iomem*)bar_addr[0]+0x407c);
readl((void __iomem*)bar_addr[0]+0x40ac);
readl((void __iomem*)bar_addr[0]+0x4010);
readl((void __iomem*)bar_addr[0]+0x4048);
readl((void __iomem*)bar_addr[0]+0x404c);
readl((void __iomem*)bar_addr[0]+0x4050);
readl((void __iomem*)bar_addr[0]+0x4054);
readl((void __iomem*)bar_addr[0]+0x4080);
readl((void __iomem*)bar_addr[0]+0x4090);
readl((void __iomem*)bar_addr[0]+0x4094);
readl((void __iomem*)bar_addr[0]+0x40a0);
readl((void __iomem*)bar_addr[0]+0x40a4);
readl((void __iomem*)bar_addr[0]+0x40f0);
readl((void __iomem*)bar_addr[0]+0x40f4);
readl((void __iomem*)bar_addr[0]+0x40d4);
readl((void __iomem*)bar_addr[0]+0x4004);
readl((void __iomem*)bar_addr[0]+0x400c);
readl((void __iomem*)bar_addr[0]+0x403c);
readl((void __iomem*)bar_addr[0]+0x40f8);
readl((void __iomem*)bar_addr[0]+0x40fc);
readl((void __iomem*)bar_addr[0]+0x40bc);
readl((void __iomem*)bar_addr[0]+0x40b4);
readl((void __iomem*)bar_addr[0]+0x40b8);

writel(0x100000,(void __iomem*)bar_addr[0]+0xc8);
writel(0x3d0,(void __iomem*)bar_addr[0]+0xe8);
writel(0x80000008,(void __iomem*)bar_addr[0]+0x3820);
writel(0x80000020,(void __iomem*)bar_addr[0]+0x2820);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x80000008,(void __iomem*)bar_addr[0]+0x3820);
writel(0x80000020,(void __iomem*)bar_addr[0]+0x2820);
readl((void __iomem*)bar_addr[0]+0x8);
readl(blk->cpu_addr);

writel(0x100000,(void __iomem*)bar_addr[0]+0xd0);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x8350000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0x4000);
readl((void __iomem*)bar_addr[0]+0x4074);
readl((void __iomem*)bar_addr[0]+0x4088);
readl((void __iomem*)bar_addr[0]+0x408c);
readl((void __iomem*)bar_addr[0]+0x4078);
readl((void __iomem*)bar_addr[0]+0x407c);
readl((void __iomem*)bar_addr[0]+0x40ac);
readl((void __iomem*)bar_addr[0]+0x4010);
readl((void __iomem*)bar_addr[0]+0x4048);
readl((void __iomem*)bar_addr[0]+0x404c);
readl((void __iomem*)bar_addr[0]+0x4050);
readl((void __iomem*)bar_addr[0]+0x4054);
readl((void __iomem*)bar_addr[0]+0x4080);
readl((void __iomem*)bar_addr[0]+0x4090);
readl((void __iomem*)bar_addr[0]+0x4094);
readl((void __iomem*)bar_addr[0]+0x40a0);
readl((void __iomem*)bar_addr[0]+0x40a4);
readl((void __iomem*)bar_addr[0]+0x40f0);
readl((void __iomem*)bar_addr[0]+0x40f4);
readl((void __iomem*)bar_addr[0]+0x40d4);
readl((void __iomem*)bar_addr[0]+0x4004);
readl((void __iomem*)bar_addr[0]+0x400c);
readl((void __iomem*)bar_addr[0]+0x403c);
readl((void __iomem*)bar_addr[0]+0x40f8);
readl((void __iomem*)bar_addr[0]+0x40fc);
readl((void __iomem*)bar_addr[0]+0x40bc);
readl((void __iomem*)bar_addr[0]+0x40b4);
readl((void __iomem*)bar_addr[0]+0x40b8);
readl((void __iomem*)bar_addr[0]+0x100);

writel(0x0,(void __iomem*)bar_addr[0]+0x53fc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53f8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53f4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53f0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53ec);
writel(0x0,(void __iomem*)bar_addr[0]+0x53e8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53e4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53e0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53dc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53d8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53d4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53d0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53cc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53c8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53c4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53c0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53bc);
writel(0x0,(void __iomem*)bar_addr[0]+0x53b8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53b4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53b0);
writel(0x0,(void __iomem*)bar_addr[0]+0x53ac);
writel(0x0,(void __iomem*)bar_addr[0]+0x53a8);
writel(0x0,(void __iomem*)bar_addr[0]+0x53a4);
writel(0x0,(void __iomem*)bar_addr[0]+0x53a0);
writel(0x0,(void __iomem*)bar_addr[0]+0x539c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5398);
writel(0x0,(void __iomem*)bar_addr[0]+0x5394);
writel(0x0,(void __iomem*)bar_addr[0]+0x5390);
writel(0x0,(void __iomem*)bar_addr[0]+0x538c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5388);
writel(0x0,(void __iomem*)bar_addr[0]+0x5384);
writel(0x0,(void __iomem*)bar_addr[0]+0x5380);
writel(0x0,(void __iomem*)bar_addr[0]+0x537c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5378);
writel(0x0,(void __iomem*)bar_addr[0]+0x5374);
writel(0x0,(void __iomem*)bar_addr[0]+0x5370);
writel(0x0,(void __iomem*)bar_addr[0]+0x536c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5368);
writel(0x0,(void __iomem*)bar_addr[0]+0x5364);
writel(0x0,(void __iomem*)bar_addr[0]+0x5360);
writel(0x0,(void __iomem*)bar_addr[0]+0x535c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5358);
writel(0x0,(void __iomem*)bar_addr[0]+0x5354);
writel(0x0,(void __iomem*)bar_addr[0]+0x5350);
writel(0x0,(void __iomem*)bar_addr[0]+0x534c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5348);
writel(0x0,(void __iomem*)bar_addr[0]+0x5344);
writel(0x0,(void __iomem*)bar_addr[0]+0x5340);
writel(0x0,(void __iomem*)bar_addr[0]+0x533c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5338);
writel(0x0,(void __iomem*)bar_addr[0]+0x5334);
writel(0x0,(void __iomem*)bar_addr[0]+0x5330);
writel(0x0,(void __iomem*)bar_addr[0]+0x532c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5328);
writel(0x0,(void __iomem*)bar_addr[0]+0x5324);
writel(0x0,(void __iomem*)bar_addr[0]+0x5320);
writel(0x0,(void __iomem*)bar_addr[0]+0x531c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5318);
writel(0x0,(void __iomem*)bar_addr[0]+0x5314);
writel(0x0,(void __iomem*)bar_addr[0]+0x5310);
writel(0x0,(void __iomem*)bar_addr[0]+0x530c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5308);
writel(0x0,(void __iomem*)bar_addr[0]+0x5304);
writel(0x0,(void __iomem*)bar_addr[0]+0x5300);
writel(0x0,(void __iomem*)bar_addr[0]+0x52fc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52f8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52f4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52f0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52ec);
writel(0x0,(void __iomem*)bar_addr[0]+0x52e8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52e4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52e0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52dc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52d8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52d4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52d0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52cc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52c8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52c4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52c0);
writel(0x0,(void __iomem*)bar_addr[0]+0x52bc);
writel(0x0,(void __iomem*)bar_addr[0]+0x52b8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52b4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52b0);
writel(0x8,(void __iomem*)bar_addr[0]+0x52ac);
writel(0x0,(void __iomem*)bar_addr[0]+0x52a8);
writel(0x0,(void __iomem*)bar_addr[0]+0x52a4);
writel(0x0,(void __iomem*)bar_addr[0]+0x52a0);
writel(0x0,(void __iomem*)bar_addr[0]+0x529c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5298);
writel(0x0,(void __iomem*)bar_addr[0]+0x5294);
writel(0x0,(void __iomem*)bar_addr[0]+0x5290);
writel(0x0,(void __iomem*)bar_addr[0]+0x528c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5288);
writel(0x0,(void __iomem*)bar_addr[0]+0x5284);
writel(0x0,(void __iomem*)bar_addr[0]+0x5280);
writel(0x0,(void __iomem*)bar_addr[0]+0x527c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5278);
writel(0x0,(void __iomem*)bar_addr[0]+0x5274);
writel(0x0,(void __iomem*)bar_addr[0]+0x5270);
writel(0x0,(void __iomem*)bar_addr[0]+0x526c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5268);
writel(0x0,(void __iomem*)bar_addr[0]+0x5264);
writel(0x0,(void __iomem*)bar_addr[0]+0x5260);
writel(0x0,(void __iomem*)bar_addr[0]+0x525c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5258);
writel(0x0,(void __iomem*)bar_addr[0]+0x5254);
writel(0x0,(void __iomem*)bar_addr[0]+0x5250);
writel(0x0,(void __iomem*)bar_addr[0]+0x524c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5248);
writel(0x0,(void __iomem*)bar_addr[0]+0x5244);
writel(0x0,(void __iomem*)bar_addr[0]+0x5240);
writel(0x0,(void __iomem*)bar_addr[0]+0x523c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5238);
writel(0x0,(void __iomem*)bar_addr[0]+0x5234);
writel(0x0,(void __iomem*)bar_addr[0]+0x5230);
writel(0x0,(void __iomem*)bar_addr[0]+0x522c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5228);
writel(0x0,(void __iomem*)bar_addr[0]+0x5224);
writel(0x0,(void __iomem*)bar_addr[0]+0x5220);
writel(0x0,(void __iomem*)bar_addr[0]+0x521c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5218);
writel(0x0,(void __iomem*)bar_addr[0]+0x5214);
writel(0x0,(void __iomem*)bar_addr[0]+0x5210);
writel(0x0,(void __iomem*)bar_addr[0]+0x520c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5208);
writel(0x0,(void __iomem*)bar_addr[0]+0x5204);
writel(0x10000,(void __iomem*)bar_addr[0]+0x5200);
readl((void __iomem*)bar_addr[0]+0x8);
readl((void __iomem*)bar_addr[0]+0x100);

writel(0x4048002,(void __iomem*)bar_addr[0]+0x100);
writel(0x0,(void __iomem*)bar_addr[0]+0x5474);
writel(0x0,(void __iomem*)bar_addr[0]+0x5470);
writel(0x0,(void __iomem*)bar_addr[0]+0x546c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5468);
writel(0x0,(void __iomem*)bar_addr[0]+0x5464);
writel(0x0,(void __iomem*)bar_addr[0]+0x5460);
writel(0x0,(void __iomem*)bar_addr[0]+0x545c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5458);
writel(0x0,(void __iomem*)bar_addr[0]+0x5454);
writel(0x0,(void __iomem*)bar_addr[0]+0x5450);
writel(0x0,(void __iomem*)bar_addr[0]+0x544c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5448);
writel(0x0,(void __iomem*)bar_addr[0]+0x5444);
writel(0x0,(void __iomem*)bar_addr[0]+0x5440);
writel(0x0,(void __iomem*)bar_addr[0]+0x543c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5438);
writel(0x0,(void __iomem*)bar_addr[0]+0x5434);
writel(0x0,(void __iomem*)bar_addr[0]+0x5430);
writel(0x0,(void __iomem*)bar_addr[0]+0x542c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5428);
writel(0x0,(void __iomem*)bar_addr[0]+0x5424);
writel(0x0,(void __iomem*)bar_addr[0]+0x5420);
writel(0x0,(void __iomem*)bar_addr[0]+0x541c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5418);
writel(0x0,(void __iomem*)bar_addr[0]+0x5414);
writel(0x0,(void __iomem*)bar_addr[0]+0x5410);
writel(0x0,(void __iomem*)bar_addr[0]+0x540c);
writel(0x0,(void __iomem*)bar_addr[0]+0x5408);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x4008002,(void __iomem*)bar_addr[0]+0x100);
readl((void __iomem*)bar_addr[0]+0x0);

writel(0x58140245,(void __iomem*)bar_addr[0]+0x0);
blk = get_DMA_blk(0x1dea2000);
writel(0x0, blk->cpu_addr);
writel(0x1c4d2802, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x200005a, blk->cpu_addr);
writel(0x8b00005a, blk->cpu_addr);
writel(0x1,(void __iomem*)bar_addr[0]+0x3818);
readl(blk->cpu_addr);

writel(0x0, blk->cpu_addr);
writel(0x400000,(void __iomem*)bar_addr[0]+0xd0);
writel(0x0, blk->cpu_addr);
writel(0x1e4c7802, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x2000056, blk->cpu_addr);
writel(0x8b000056, blk->cpu_addr);
writel(0x2,(void __iomem*)bar_addr[0]+0x3818);
readl(blk->cpu_addr);

writel(0x0, blk->cpu_addr);
writel(0x400000,(void __iomem*)bar_addr[0]+0xd0);
writel(0x0, blk->cpu_addr);
writel(0x1c4d2802, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x200005a, blk->cpu_addr);
writel(0x8b00005a, blk->cpu_addr);
writel(0x3,(void __iomem*)bar_addr[0]+0x3818);
readl(blk->cpu_addr);

writel(0x0, blk->cpu_addr);
writel(0x400000,(void __iomem*)bar_addr[0]+0xd0);
writel(0x0, blk->cpu_addr);
writel(0x1c4d2802, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x200005a, blk->cpu_addr);
writel(0x8b00005a, blk->cpu_addr);
writel(0x4,(void __iomem*)bar_addr[0]+0x3818);
readl(blk->cpu_addr);

writel(0x0, blk->cpu_addr);
writel(0x400000,(void __iomem*)bar_addr[0]+0xd0);
writel(0x0, blk->cpu_addr);
writel(0x1e4c7802, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x2000046, blk->cpu_addr);
writel(0x8b000046, blk->cpu_addr);
writel(0x5,(void __iomem*)bar_addr[0]+0x3818);
readl(blk->cpu_addr);

writel(0x1b2,(void __iomem*)bar_addr[0]+0xe8);
writel(0x0, blk->cpu_addr);
writel(0x400000,(void __iomem*)bar_addr[0]+0xd0);
blk = get_DMA_blk(0x1de3c000);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);

writel(0x1c4df740, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x100000,(void __iomem*)bar_addr[0]+0xd0);
readl((void __iomem*)bar_addr[0]+0x4000);
readl((void __iomem*)bar_addr[0]+0x4074);
readl((void __iomem*)bar_addr[0]+0x4088);
readl((void __iomem*)bar_addr[0]+0x408c);
readl((void __iomem*)bar_addr[0]+0x4078);
readl((void __iomem*)bar_addr[0]+0x407c);
readl((void __iomem*)bar_addr[0]+0x40ac);
readl((void __iomem*)bar_addr[0]+0x4010);
readl((void __iomem*)bar_addr[0]+0x4048);
readl((void __iomem*)bar_addr[0]+0x404c);
readl((void __iomem*)bar_addr[0]+0x4050);
readl((void __iomem*)bar_addr[0]+0x4054);
readl((void __iomem*)bar_addr[0]+0x4080);
readl((void __iomem*)bar_addr[0]+0x4090);
readl((void __iomem*)bar_addr[0]+0x4094);
readl((void __iomem*)bar_addr[0]+0x40a0);
readl((void __iomem*)bar_addr[0]+0x40a4);
readl((void __iomem*)bar_addr[0]+0x40f0);
readl((void __iomem*)bar_addr[0]+0x40f4);
readl((void __iomem*)bar_addr[0]+0x40d4);
readl((void __iomem*)bar_addr[0]+0x4004);
readl((void __iomem*)bar_addr[0]+0x400c);
readl((void __iomem*)bar_addr[0]+0x403c);
readl((void __iomem*)bar_addr[0]+0x40f8);
readl((void __iomem*)bar_addr[0]+0x40fc);
readl((void __iomem*)bar_addr[0]+0x40bc);
readl((void __iomem*)bar_addr[0]+0x40b4);
readl((void __iomem*)bar_addr[0]+0x40b8);

writel(0x100000,(void __iomem*)bar_addr[0]+0xc8);
writel(0x117,(void __iomem*)bar_addr[0]+0xe8);
writel(0x80000008,(void __iomem*)bar_addr[0]+0x3820);
writel(0x80000020,(void __iomem*)bar_addr[0]+0x2820);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x80000008,(void __iomem*)bar_addr[0]+0x3820);
writel(0x80000020,(void __iomem*)bar_addr[0]+0x2820);
readl((void __iomem*)bar_addr[0]+0x8);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x8350000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8210000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x8300000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x8310000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x8310000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x8310000,(void __iomem*)bar_addr[0]+0x20);
readl(blk->cpu_addr);

writel(0x100000,(void __iomem*)bar_addr[0]+0xd0);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x82a0000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
blk = get_DMA_blk(0x1dea2000);
writel(0x0, blk->cpu_addr);
writel(0x1e4c7202, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x2000056, blk->cpu_addr);
writel(0x8b000056, blk->cpu_addr);
writel(0x6,(void __iomem*)bar_addr[0]+0x3818);
readl(blk->cpu_addr);

writel(0x0, blk->cpu_addr);
writel(0x400000,(void __iomem*)bar_addr[0]+0xd0);
writel(0x0, blk->cpu_addr);
writel(0x1c4d2802, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x200005a, blk->cpu_addr);
writel(0x8b00005a, blk->cpu_addr);
writel(0x7,(void __iomem*)bar_addr[0]+0x3818);
readl(blk->cpu_addr);

writel(0x0, blk->cpu_addr);
writel(0x400000,(void __iomem*)bar_addr[0]+0xd0);
writel(0xcd,(void __iomem*)bar_addr[0]+0xe8);
blk = get_DMA_blk(0x1de3c000);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);

writel(0x1dc23840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x0,(void __iomem*)bar_addr[0]+0x2818);
writel(0x100000,(void __iomem*)bar_addr[0]+0xd0);
blk = get_DMA_blk(0x1dea2000);
writel(0x0, blk->cpu_addr);
writel(0x1ddf3c02, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x2000156, blk->cpu_addr);
writel(0x8b000156, blk->cpu_addr);
writel(0x8,(void __iomem*)bar_addr[0]+0x3818);
readl(blk->cpu_addr);

writel(0xc3,(void __iomem*)bar_addr[0]+0xe8);
writel(0x0, blk->cpu_addr);
writel(0x400000,(void __iomem*)bar_addr[0]+0xd0);
blk = get_DMA_blk(0x1de3c000);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);

writel(0x1dc23040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1dff8040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x100000,(void __iomem*)bar_addr[0]+0xd0);
readl(blk->cpu_addr);

writel(0x100000,(void __iomem*)bar_addr[0]+0xd0);
blk = get_DMA_blk(0x1dea2000);
writel(0x0, blk->cpu_addr);
writel(0x1ddf3c02, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x2000156, blk->cpu_addr);
writel(0x8b000156, blk->cpu_addr);
writel(0x9,(void __iomem*)bar_addr[0]+0x3818);
readl(blk->cpu_addr);

writel(0x0, blk->cpu_addr);
writel(0x400000,(void __iomem*)bar_addr[0]+0xd0);
blk = get_DMA_blk(0x1de3c000);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);

writel(0x1c4ded00, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x100000,(void __iomem*)bar_addr[0]+0xd0);
readl((void __iomem*)bar_addr[0]+0x4000);
readl((void __iomem*)bar_addr[0]+0x4074);
readl((void __iomem*)bar_addr[0]+0x4088);
readl((void __iomem*)bar_addr[0]+0x408c);
readl((void __iomem*)bar_addr[0]+0x4078);
readl((void __iomem*)bar_addr[0]+0x407c);
readl((void __iomem*)bar_addr[0]+0x40ac);
readl((void __iomem*)bar_addr[0]+0x4010);
readl((void __iomem*)bar_addr[0]+0x4048);
readl((void __iomem*)bar_addr[0]+0x404c);
readl((void __iomem*)bar_addr[0]+0x4050);
readl((void __iomem*)bar_addr[0]+0x4054);
readl((void __iomem*)bar_addr[0]+0x4080);
readl((void __iomem*)bar_addr[0]+0x4090);
readl((void __iomem*)bar_addr[0]+0x4094);
readl((void __iomem*)bar_addr[0]+0x40a0);
readl((void __iomem*)bar_addr[0]+0x40a4);
readl((void __iomem*)bar_addr[0]+0x40f0);
readl((void __iomem*)bar_addr[0]+0x40f4);
readl((void __iomem*)bar_addr[0]+0x40d4);
readl((void __iomem*)bar_addr[0]+0x4004);
readl((void __iomem*)bar_addr[0]+0x400c);
readl((void __iomem*)bar_addr[0]+0x403c);
readl((void __iomem*)bar_addr[0]+0x40f8);
readl((void __iomem*)bar_addr[0]+0x40fc);
readl((void __iomem*)bar_addr[0]+0x40bc);
readl((void __iomem*)bar_addr[0]+0x40b4);
readl((void __iomem*)bar_addr[0]+0x40b8);

writel(0x100000,(void __iomem*)bar_addr[0]+0xc8);
writel(0x80000008,(void __iomem*)bar_addr[0]+0x3820);
writel(0x80000020,(void __iomem*)bar_addr[0]+0x2820);
readl((void __iomem*)bar_addr[0]+0x8);

writel(0x80000008,(void __iomem*)bar_addr[0]+0x3820);
writel(0x80000020,(void __iomem*)bar_addr[0]+0x2820);
readl((void __iomem*)bar_addr[0]+0x8);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x28,(void __iomem*)bar_addr[0]+0xf00);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x4360000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);

writel(0x8350000,(void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0x20);
readl((void __iomem*)bar_addr[0]+0xf00);

writel(0x8,(void __iomem*)bar_addr[0]+0xf00);
readl(blk->cpu_addr);

writel(0x100000,(void __iomem*)bar_addr[0]+0xd0);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);

writel(0x1c4de440, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
blk = get_DMA_blk(0x1dea2000);
writel(0x0, blk->cpu_addr);
writel(0x1e430402, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x200002a, blk->cpu_addr);
writel(0x8b00002a, blk->cpu_addr);
writel(0xa,(void __iomem*)bar_addr[0]+0x3818);
readl(blk->cpu_addr);

writel(0x0, blk->cpu_addr);
writel(0x400000,(void __iomem*)bar_addr[0]+0xd0);
writel(0x100000,(void __iomem*)bar_addr[0]+0xd0);
blk = get_DMA_blk(0x1de3c000);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);

writel(0x1dff9840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
blk = get_DMA_blk(0x1dea2000);
writel(0x0, blk->cpu_addr);
writeb(0x22, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writeb(0x32, blk->cpu_addr);
writew(0x0, blk->cpu_addr);
writel(0x21000000, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c548106, blk->cpu_addr);
writel(0x200, blk->cpu_addr);
writel(0x2210003a, blk->cpu_addr);
writel(0xab10003a, blk->cpu_addr);
writel(0xc,(void __iomem*)bar_addr[0]+0x3818);
readl(blk->cpu_addr);

writel(0x0, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x400000,(void __iomem*)bar_addr[0]+0xd0);
writel(0x100000,(void __iomem*)bar_addr[0]+0xd0);
blk = get_DMA_blk(0x1de3c000);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);
readw(blk->cpu_addr);
readl(blk->cpu_addr);

writel(0x1dffa040, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1dffa840, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
blk = get_DMA_blk(0x1dea2000);
writel(0x0, blk->cpu_addr);
writeb(0x22, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writeb(0x32, blk->cpu_addr);
writew(0x0, blk->cpu_addr);
writel(0x21000000, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c54810a, blk->cpu_addr);
writel(0x200, blk->cpu_addr);
writel(0x22100036, blk->cpu_addr);
writel(0xab100036, blk->cpu_addr);
writel(0xe,(void __iomem*)bar_addr[0]+0x3818);
readl(blk->cpu_addr);

writel(0x0, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x400000,(void __iomem*)bar_addr[0]+0xd0);
writel(0x100000,(void __iomem*)bar_addr[0]+0xd0);
writel(0x0, blk->cpu_addr);
writeb(0x22, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writeb(0x32, blk->cpu_addr);
writew(0x0, blk->cpu_addr);
writel(0x21000000, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c54810a, blk->cpu_addr);
writel(0x200, blk->cpu_addr);
writel(0x22100036, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x1c540000, blk->cpu_addr);
writel(0x200, blk->cpu_addr);
writel(0x22100027, blk->cpu_addr);
writel(0xab100027, blk->cpu_addr);
writel(0x11,(void __iomem*)bar_addr[0]+0x3818);
readl(blk->cpu_addr);

writel(0x0, blk->cpu_addr);
writel(0x0, blk->cpu_addr);
writel(0x0, blk->cpu_addr);

pdev = pci_get_device(0x8086,0x10d3,NULL);
blk = get_DMA_blk(0x1dea2000);
dma_free_coherent(&(pdev->dev), 0x1000, blk->cpu_addr, blk->dma_handle);
pdev = pci_get_device(0x8086,0x10d3,NULL);
blk = get_DMA_blk(0x1de3c000);
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
