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
        struct list_head    node;
        struct dev_cfg      cfg;
};

typedef struct _DMA_block
{
        void *cpu_addr;
        unsigned int log_addr;
        dma_addr_t bus_addr;
        struct list_head node;
}DMA_blk;

int add_DMA_blk(void *cpu_addr, unsigned int log_addr, dma_addr_t bus_addr)
{
        DMA_blk *new_blk = (DMA_blk *)kzalloc(sizeof(DMA_blk), GFP_KERNEL);
        if(!new_blk){
                printk(KERN_ERR "###| [ERR ] memory allocation failed!\n");
                return 1;
        }
        new_blk->cpu_addr = cpu_addr;
        new_blk->log_addr = log_addr;
        new_blk->bus_addr = bus_addr;

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
vendor = 0x8086;
device = 0x10d3;


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
writel(0x4380000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
writel(0x21, bar_addr[0]+0x14);
readl(bar_addr[0]+0x14);
readl(bar_addr[0]+0x14);

writel(0x25, bar_addr[0]+0x14);
readl(bar_addr[0]+0x14);
readl(bar_addr[0]+0x14);
readl(bar_addr[0]+0x18);

writel(0x10400000, bar_addr[0]+0x18);
writel(0x20, bar_addr[0]+0x1000);
readl(bar_addr[0]+0x1000);
readl(bar_addr[0]+0x0);

writel(0x18140245, bar_addr[0]+0x0);
readl(bar_addr[0]+0x8);

writel(0xffffffff, bar_addr[0]+0xd8);
writel(0x0, bar_addr[0]+0x100);
readl(bar_addr[0]+0x400);

writel(0x0, bar_addr[0]+0x400);
readl(bar_addr[0]+0x8);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);
readl(bar_addr[0]+0x0);

writel(0x1c140245, bar_addr[0]+0x0);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0x10);

writel(0xffffffff, bar_addr[0]+0xd8);
readl(bar_addr[0]+0xc0);
readl(bar_addr[0]+0x18);

writel(0x10400000, bar_addr[0]+0x18);
writel(0x0, bar_addr[0]+0x5800);
readl(bar_addr[0]+0x3828);

writel(0x1410000, bar_addr[0]+0x3828);
readl(bar_addr[0]+0x3928);

writel(0x400000, bar_addr[0]+0x3928);
readl(bar_addr[0]+0x3840);

writel(0x4000403, bar_addr[0]+0x3840);
readl(bar_addr[0]+0x3940);
readl(bar_addr[0]+0x0);

writel(0x18140245, bar_addr[0]+0x0);
readl(bar_addr[0]+0x18);

writel(0x10400000, bar_addr[0]+0x18);
readl(bar_addr[0]+0x5b00);

writel(0xec00200, bar_addr[0]+0x5b00);
readl(bar_addr[0]+0x5b64);

writel(0x1, bar_addr[0]+0x5b64);
writel(0x11, bar_addr[0]+0x14);
readl(bar_addr[0]+0x14);
readl(bar_addr[0]+0x14);
readl(bar_addr[0]+0xe00);

writel(0x0, bar_addr[0]+0x5600);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5604);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5608);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x560c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5610);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5614);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5618);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x561c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5620);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5624);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5628);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x562c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5630);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5634);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5638);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x563c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5640);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5644);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5648);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x564c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5650);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5654);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5658);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x565c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5660);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5664);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5668);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x566c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5670);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5674);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5678);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x567c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5680);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5684);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5688);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x568c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5690);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5694);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5698);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x569c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56a0);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56a4);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56a8);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56ac);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56b0);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56b4);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56b8);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56bc);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56c0);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56c4);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56c8);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56cc);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56d0);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56d4);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56d8);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56dc);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56e0);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56e4);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56e8);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56ec);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56f0);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56f4);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56f8);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x56fc);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5700);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5704);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5708);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x570c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5710);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5714);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5718);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x571c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5720);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5724);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5728);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x572c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5730);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5734);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5738);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x573c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5740);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5744);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5748);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x574c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5750);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5754);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5758);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x575c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5760);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5764);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5768);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x576c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5770);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5774);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5778);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x577c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5780);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5784);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5788);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x578c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5790);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5794);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5798);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x579c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57a0);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57a4);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57a8);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57ac);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57b0);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57b4);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57b8);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57bc);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57c0);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57c4);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57c8);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57cc);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57d0);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57d4);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57d8);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57dc);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57e0);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57e4);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57e8);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57ec);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57f0);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57f4);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57f8);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x57fc);
readl(bar_addr[0]+0x8);

writel(0x12005452, bar_addr[0]+0x5400);
readl(bar_addr[0]+0x8);

writel(0x80005634, bar_addr[0]+0x5404);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5408);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x540c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5410);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5414);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5418);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x541c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5420);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5424);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5428);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x542c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5430);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5434);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5438);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x543c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5440);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5444);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5448);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x544c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5450);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5454);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5458);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x545c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5460);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5464);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5468);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x546c);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5470);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5474);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x5200);
writel(0x0, bar_addr[0]+0x5204);
writel(0x0, bar_addr[0]+0x5208);
writel(0x0, bar_addr[0]+0x520c);
writel(0x0, bar_addr[0]+0x5210);
writel(0x0, bar_addr[0]+0x5214);
writel(0x0, bar_addr[0]+0x5218);
writel(0x0, bar_addr[0]+0x521c);
writel(0x0, bar_addr[0]+0x5220);
writel(0x0, bar_addr[0]+0x5224);
writel(0x0, bar_addr[0]+0x5228);
writel(0x0, bar_addr[0]+0x522c);
writel(0x0, bar_addr[0]+0x5230);
writel(0x0, bar_addr[0]+0x5234);
writel(0x0, bar_addr[0]+0x5238);
writel(0x0, bar_addr[0]+0x523c);
writel(0x0, bar_addr[0]+0x5240);
writel(0x0, bar_addr[0]+0x5244);
writel(0x0, bar_addr[0]+0x5248);
writel(0x0, bar_addr[0]+0x524c);
writel(0x0, bar_addr[0]+0x5250);
writel(0x0, bar_addr[0]+0x5254);
writel(0x0, bar_addr[0]+0x5258);
writel(0x0, bar_addr[0]+0x525c);
writel(0x0, bar_addr[0]+0x5260);
writel(0x0, bar_addr[0]+0x5264);
writel(0x0, bar_addr[0]+0x5268);
writel(0x0, bar_addr[0]+0x526c);
writel(0x0, bar_addr[0]+0x5270);
writel(0x0, bar_addr[0]+0x5274);
writel(0x0, bar_addr[0]+0x5278);
writel(0x0, bar_addr[0]+0x527c);
writel(0x0, bar_addr[0]+0x5280);
writel(0x0, bar_addr[0]+0x5284);
writel(0x0, bar_addr[0]+0x5288);
writel(0x0, bar_addr[0]+0x528c);
writel(0x0, bar_addr[0]+0x5290);
writel(0x0, bar_addr[0]+0x5294);
writel(0x0, bar_addr[0]+0x5298);
writel(0x0, bar_addr[0]+0x529c);
writel(0x0, bar_addr[0]+0x52a0);
writel(0x0, bar_addr[0]+0x52a4);
writel(0x0, bar_addr[0]+0x52a8);
writel(0x0, bar_addr[0]+0x52ac);
writel(0x0, bar_addr[0]+0x52b0);
writel(0x0, bar_addr[0]+0x52b4);
writel(0x0, bar_addr[0]+0x52b8);
writel(0x0, bar_addr[0]+0x52bc);
writel(0x0, bar_addr[0]+0x52c0);
writel(0x0, bar_addr[0]+0x52c4);
writel(0x0, bar_addr[0]+0x52c8);
writel(0x0, bar_addr[0]+0x52cc);
writel(0x0, bar_addr[0]+0x52d0);
writel(0x0, bar_addr[0]+0x52d4);
writel(0x0, bar_addr[0]+0x52d8);
writel(0x0, bar_addr[0]+0x52dc);
writel(0x0, bar_addr[0]+0x52e0);
writel(0x0, bar_addr[0]+0x52e4);
writel(0x0, bar_addr[0]+0x52e8);
writel(0x0, bar_addr[0]+0x52ec);
writel(0x0, bar_addr[0]+0x52f0);
writel(0x0, bar_addr[0]+0x52f4);
writel(0x0, bar_addr[0]+0x52f8);
writel(0x0, bar_addr[0]+0x52fc);
writel(0x0, bar_addr[0]+0x5300);
writel(0x0, bar_addr[0]+0x5304);
writel(0x0, bar_addr[0]+0x5308);
writel(0x0, bar_addr[0]+0x530c);
writel(0x0, bar_addr[0]+0x5310);
writel(0x0, bar_addr[0]+0x5314);
writel(0x0, bar_addr[0]+0x5318);
writel(0x0, bar_addr[0]+0x531c);
writel(0x0, bar_addr[0]+0x5320);
writel(0x0, bar_addr[0]+0x5324);
writel(0x0, bar_addr[0]+0x5328);
writel(0x0, bar_addr[0]+0x532c);
writel(0x0, bar_addr[0]+0x5330);
writel(0x0, bar_addr[0]+0x5334);
writel(0x0, bar_addr[0]+0x5338);
writel(0x0, bar_addr[0]+0x533c);
writel(0x0, bar_addr[0]+0x5340);
writel(0x0, bar_addr[0]+0x5344);
writel(0x0, bar_addr[0]+0x5348);
writel(0x0, bar_addr[0]+0x534c);
writel(0x0, bar_addr[0]+0x5350);
writel(0x0, bar_addr[0]+0x5354);
writel(0x0, bar_addr[0]+0x5358);
writel(0x0, bar_addr[0]+0x535c);
writel(0x0, bar_addr[0]+0x5360);
writel(0x0, bar_addr[0]+0x5364);
writel(0x0, bar_addr[0]+0x5368);
writel(0x0, bar_addr[0]+0x536c);
writel(0x0, bar_addr[0]+0x5370);
writel(0x0, bar_addr[0]+0x5374);
writel(0x0, bar_addr[0]+0x5378);
writel(0x0, bar_addr[0]+0x537c);
writel(0x0, bar_addr[0]+0x5380);
writel(0x0, bar_addr[0]+0x5384);
writel(0x0, bar_addr[0]+0x5388);
writel(0x0, bar_addr[0]+0x538c);
writel(0x0, bar_addr[0]+0x5390);
writel(0x0, bar_addr[0]+0x5394);
writel(0x0, bar_addr[0]+0x5398);
writel(0x0, bar_addr[0]+0x539c);
writel(0x0, bar_addr[0]+0x53a0);
writel(0x0, bar_addr[0]+0x53a4);
writel(0x0, bar_addr[0]+0x53a8);
writel(0x0, bar_addr[0]+0x53ac);
writel(0x0, bar_addr[0]+0x53b0);
writel(0x0, bar_addr[0]+0x53b4);
writel(0x0, bar_addr[0]+0x53b8);
writel(0x0, bar_addr[0]+0x53bc);
writel(0x0, bar_addr[0]+0x53c0);
writel(0x0, bar_addr[0]+0x53c4);
writel(0x0, bar_addr[0]+0x53c8);
writel(0x0, bar_addr[0]+0x53cc);
writel(0x0, bar_addr[0]+0x53d0);
writel(0x0, bar_addr[0]+0x53d4);
writel(0x0, bar_addr[0]+0x53d8);
writel(0x0, bar_addr[0]+0x53dc);
writel(0x0, bar_addr[0]+0x53e0);
writel(0x0, bar_addr[0]+0x53e4);
writel(0x0, bar_addr[0]+0x53e8);
writel(0x0, bar_addr[0]+0x53ec);
writel(0x0, bar_addr[0]+0x53f0);
writel(0x0, bar_addr[0]+0x53f4);
writel(0x0, bar_addr[0]+0x53f8);
writel(0x0, bar_addr[0]+0x53fc);
readl(bar_addr[0]+0x5820);
readl(bar_addr[0]+0x0);

writel(0x18140245, bar_addr[0]+0x0);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x8300000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x4303360, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8200000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4209940, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x4303b60, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x43d0003, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x43e0000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8200000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4209940, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8240000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8290000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4240de1, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4290e00, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8200000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4201b40, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
writel(0x8808, bar_addr[0]+0x30);
writel(0x100, bar_addr[0]+0x2c);
writel(0xc28001, bar_addr[0]+0x28);
writel(0x680, bar_addr[0]+0x170);
writel(0x80007328, bar_addr[0]+0x2160);
writel(0x7330, bar_addr[0]+0x2168);
readl(bar_addr[0]+0x3828);

writel(0x1410000, bar_addr[0]+0x3828);
readl(bar_addr[0]+0x5b00);

writel(0xec00200, bar_addr[0]+0x5b00);
readl(bar_addr[0]+0x4000);
readl(bar_addr[0]+0x4008);
readl(bar_addr[0]+0x4010);
readl(bar_addr[0]+0x4014);
readl(bar_addr[0]+0x4018);
readl(bar_addr[0]+0x401c);
readl(bar_addr[0]+0x4020);
readl(bar_addr[0]+0x4028);
readl(bar_addr[0]+0x4030);
readl(bar_addr[0]+0x4038);
readl(bar_addr[0]+0x4040);
readl(bar_addr[0]+0x4048);
readl(bar_addr[0]+0x404c);
readl(bar_addr[0]+0x4050);
readl(bar_addr[0]+0x4054);
readl(bar_addr[0]+0x4058);
readl(bar_addr[0]+0x4074);
readl(bar_addr[0]+0x4078);
readl(bar_addr[0]+0x407c);
readl(bar_addr[0]+0x4080);
readl(bar_addr[0]+0x4088);
readl(bar_addr[0]+0x408c);
readl(bar_addr[0]+0x4090);
readl(bar_addr[0]+0x4094);
readl(bar_addr[0]+0x40a0);
readl(bar_addr[0]+0x40a4);
readl(bar_addr[0]+0x40a8);
readl(bar_addr[0]+0x40ac);
readl(bar_addr[0]+0x40b0);
readl(bar_addr[0]+0x40c0);
readl(bar_addr[0]+0x40c4);
readl(bar_addr[0]+0x40c8);
readl(bar_addr[0]+0x40cc);
readl(bar_addr[0]+0x40d0);
readl(bar_addr[0]+0x40d4);
readl(bar_addr[0]+0x40f0);
readl(bar_addr[0]+0x40f4);
readl(bar_addr[0]+0x405c);
readl(bar_addr[0]+0x4060);
readl(bar_addr[0]+0x4064);
readl(bar_addr[0]+0x4068);
readl(bar_addr[0]+0x406c);
readl(bar_addr[0]+0x4070);
readl(bar_addr[0]+0x40d8);
readl(bar_addr[0]+0x40dc);
readl(bar_addr[0]+0x40e0);
readl(bar_addr[0]+0x40e4);
readl(bar_addr[0]+0x40e8);
readl(bar_addr[0]+0x40ec);
readl(bar_addr[0]+0x4004);
readl(bar_addr[0]+0x400c);
readl(bar_addr[0]+0x4034);
readl(bar_addr[0]+0x403c);
readl(bar_addr[0]+0x40f8);
readl(bar_addr[0]+0x40fc);
readl(bar_addr[0]+0x40b4);
readl(bar_addr[0]+0x40b8);
readl(bar_addr[0]+0x40bc);
readl(bar_addr[0]+0x4100);
readl(bar_addr[0]+0x4124);
readl(bar_addr[0]+0x4104);
readl(bar_addr[0]+0x4108);
readl(bar_addr[0]+0x410c);
readl(bar_addr[0]+0x4110);
readl(bar_addr[0]+0x4118);
readl(bar_addr[0]+0x411c);
readl(bar_addr[0]+0x4120);

writel(0x8100, bar_addr[0]+0x38);
writel(0x0, bar_addr[0]+0x458);
writel(0x1a00000, bar_addr[0]+0xb608);
readl(bar_addr[0]+0xb600);
readl(bar_addr[0]+0xb604);
readl(bar_addr[0]+0xb608);
readl(bar_addr[0]+0xb600);
readl(bar_addr[0]+0xb604);
readl(bar_addr[0]+0xb614);

writel(0x0, bar_addr[0]+0xb614);
readl(bar_addr[0]+0xb614);
readl(bar_addr[0]+0xb620);

writel(0x0, bar_addr[0]+0xb620);
readl(bar_addr[0]+0xb620);

writel(0x0, bar_addr[0]+0xb634);
writel(0x0, bar_addr[0]+0xb638);
readl(bar_addr[0]+0x8);
readl(bar_addr[0]+0xb628);
readl(bar_addr[0]+0xb61c);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x8390000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x4390000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8200000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4201140, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0x5820);
readl(bar_addr[0]+0x0);

writel(0x18140245, bar_addr[0]+0x0);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x8300000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x4303360, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8200000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4209140, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x4303b60, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x43d0003, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x43e0000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8200000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4209140, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8240000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8290000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4240de1, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4290e00, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8200000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4201340, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
writel(0x8808, bar_addr[0]+0x30);
writel(0x100, bar_addr[0]+0x2c);
writel(0xc28001, bar_addr[0]+0x28);
writel(0x680, bar_addr[0]+0x170);
writel(0x80007328, bar_addr[0]+0x2160);
writel(0x7330, bar_addr[0]+0x2168);
readl(bar_addr[0]+0x100);

writel(0x0, bar_addr[0]+0x53fc);
writel(0x0, bar_addr[0]+0x53f8);
writel(0x0, bar_addr[0]+0x53f4);
writel(0x0, bar_addr[0]+0x53f0);
writel(0x0, bar_addr[0]+0x53ec);
writel(0x0, bar_addr[0]+0x53e8);
writel(0x0, bar_addr[0]+0x53e4);
writel(0x0, bar_addr[0]+0x53e0);
writel(0x0, bar_addr[0]+0x53dc);
writel(0x0, bar_addr[0]+0x53d8);
writel(0x0, bar_addr[0]+0x53d4);
writel(0x0, bar_addr[0]+0x53d0);
writel(0x0, bar_addr[0]+0x53cc);
writel(0x0, bar_addr[0]+0x53c8);
writel(0x0, bar_addr[0]+0x53c4);
writel(0x0, bar_addr[0]+0x53c0);
writel(0x0, bar_addr[0]+0x53bc);
writel(0x0, bar_addr[0]+0x53b8);
writel(0x0, bar_addr[0]+0x53b4);
writel(0x0, bar_addr[0]+0x53b0);
writel(0x0, bar_addr[0]+0x53ac);
writel(0x0, bar_addr[0]+0x53a8);
writel(0x0, bar_addr[0]+0x53a4);
writel(0x0, bar_addr[0]+0x53a0);
writel(0x0, bar_addr[0]+0x539c);
writel(0x0, bar_addr[0]+0x5398);
writel(0x0, bar_addr[0]+0x5394);
writel(0x0, bar_addr[0]+0x5390);
writel(0x0, bar_addr[0]+0x538c);
writel(0x0, bar_addr[0]+0x5388);
writel(0x0, bar_addr[0]+0x5384);
writel(0x0, bar_addr[0]+0x5380);
writel(0x0, bar_addr[0]+0x537c);
writel(0x0, bar_addr[0]+0x5378);
writel(0x0, bar_addr[0]+0x5374);
writel(0x0, bar_addr[0]+0x5370);
writel(0x0, bar_addr[0]+0x536c);
writel(0x0, bar_addr[0]+0x5368);
writel(0x0, bar_addr[0]+0x5364);
writel(0x0, bar_addr[0]+0x5360);
writel(0x0, bar_addr[0]+0x535c);
writel(0x0, bar_addr[0]+0x5358);
writel(0x0, bar_addr[0]+0x5354);
writel(0x0, bar_addr[0]+0x5350);
writel(0x0, bar_addr[0]+0x534c);
writel(0x0, bar_addr[0]+0x5348);
writel(0x0, bar_addr[0]+0x5344);
writel(0x0, bar_addr[0]+0x5340);
writel(0x0, bar_addr[0]+0x533c);
writel(0x0, bar_addr[0]+0x5338);
writel(0x0, bar_addr[0]+0x5334);
writel(0x0, bar_addr[0]+0x5330);
writel(0x0, bar_addr[0]+0x532c);
writel(0x0, bar_addr[0]+0x5328);
writel(0x0, bar_addr[0]+0x5324);
writel(0x0, bar_addr[0]+0x5320);
writel(0x0, bar_addr[0]+0x531c);
writel(0x0, bar_addr[0]+0x5318);
writel(0x0, bar_addr[0]+0x5314);
writel(0x0, bar_addr[0]+0x5310);
writel(0x0, bar_addr[0]+0x530c);
writel(0x0, bar_addr[0]+0x5308);
writel(0x0, bar_addr[0]+0x5304);
writel(0x0, bar_addr[0]+0x5300);
writel(0x0, bar_addr[0]+0x52fc);
writel(0x0, bar_addr[0]+0x52f8);
writel(0x0, bar_addr[0]+0x52f4);
writel(0x0, bar_addr[0]+0x52f0);
writel(0x0, bar_addr[0]+0x52ec);
writel(0x0, bar_addr[0]+0x52e8);
writel(0x0, bar_addr[0]+0x52e4);
writel(0x0, bar_addr[0]+0x52e0);
writel(0x0, bar_addr[0]+0x52dc);
writel(0x0, bar_addr[0]+0x52d8);
writel(0x0, bar_addr[0]+0x52d4);
writel(0x0, bar_addr[0]+0x52d0);
writel(0x0, bar_addr[0]+0x52cc);
writel(0x0, bar_addr[0]+0x52c8);
writel(0x0, bar_addr[0]+0x52c4);
writel(0x0, bar_addr[0]+0x52c0);
writel(0x0, bar_addr[0]+0x52bc);
writel(0x0, bar_addr[0]+0x52b8);
writel(0x0, bar_addr[0]+0x52b4);
writel(0x0, bar_addr[0]+0x52b0);
writel(0x0, bar_addr[0]+0x52ac);
writel(0x0, bar_addr[0]+0x52a8);
writel(0x0, bar_addr[0]+0x52a4);
writel(0x0, bar_addr[0]+0x52a0);
writel(0x0, bar_addr[0]+0x529c);
writel(0x0, bar_addr[0]+0x5298);
writel(0x0, bar_addr[0]+0x5294);
writel(0x0, bar_addr[0]+0x5290);
writel(0x0, bar_addr[0]+0x528c);
writel(0x0, bar_addr[0]+0x5288);
writel(0x0, bar_addr[0]+0x5284);
writel(0x0, bar_addr[0]+0x5280);
writel(0x0, bar_addr[0]+0x527c);
writel(0x0, bar_addr[0]+0x5278);
writel(0x0, bar_addr[0]+0x5274);
writel(0x0, bar_addr[0]+0x5270);
writel(0x0, bar_addr[0]+0x526c);
writel(0x0, bar_addr[0]+0x5268);
writel(0x0, bar_addr[0]+0x5264);
writel(0x0, bar_addr[0]+0x5260);
writel(0x0, bar_addr[0]+0x525c);
writel(0x0, bar_addr[0]+0x5258);
writel(0x0, bar_addr[0]+0x5254);
writel(0x0, bar_addr[0]+0x5250);
writel(0x0, bar_addr[0]+0x524c);
writel(0x0, bar_addr[0]+0x5248);
writel(0x0, bar_addr[0]+0x5244);
writel(0x0, bar_addr[0]+0x5240);
writel(0x0, bar_addr[0]+0x523c);
writel(0x0, bar_addr[0]+0x5238);
writel(0x0, bar_addr[0]+0x5234);
writel(0x0, bar_addr[0]+0x5230);
writel(0x0, bar_addr[0]+0x522c);
writel(0x0, bar_addr[0]+0x5228);
writel(0x0, bar_addr[0]+0x5224);
writel(0x0, bar_addr[0]+0x5220);
writel(0x0, bar_addr[0]+0x521c);
writel(0x0, bar_addr[0]+0x5218);
writel(0x0, bar_addr[0]+0x5214);
writel(0x0, bar_addr[0]+0x5210);
writel(0x0, bar_addr[0]+0x520c);
writel(0x0, bar_addr[0]+0x5208);
writel(0x0, bar_addr[0]+0x5204);
writel(0x10000, bar_addr[0]+0x5200);
readl(bar_addr[0]+0x8);
readl(bar_addr[0]+0x100);

writel(0x40000, bar_addr[0]+0x100);
writel(0x0, bar_addr[0]+0x5474);
writel(0x0, bar_addr[0]+0x5470);
writel(0x0, bar_addr[0]+0x546c);
writel(0x0, bar_addr[0]+0x5468);
writel(0x0, bar_addr[0]+0x5464);
writel(0x0, bar_addr[0]+0x5460);
writel(0x0, bar_addr[0]+0x545c);
writel(0x0, bar_addr[0]+0x5458);
writel(0x0, bar_addr[0]+0x5454);
writel(0x0, bar_addr[0]+0x5450);
writel(0x0, bar_addr[0]+0x544c);
writel(0x0, bar_addr[0]+0x5448);
writel(0x0, bar_addr[0]+0x5444);
writel(0x0, bar_addr[0]+0x5440);
writel(0x0, bar_addr[0]+0x543c);
writel(0x0, bar_addr[0]+0x5438);
writel(0x0, bar_addr[0]+0x5434);
writel(0x0, bar_addr[0]+0x5430);
writel(0x0, bar_addr[0]+0x542c);
writel(0x0, bar_addr[0]+0x5428);
writel(0x0, bar_addr[0]+0x5424);
writel(0x0, bar_addr[0]+0x5420);
writel(0x0, bar_addr[0]+0x541c);
writel(0x0, bar_addr[0]+0x5418);
writel(0x0, bar_addr[0]+0x5414);
writel(0x0, bar_addr[0]+0x5410);
writel(0x0, bar_addr[0]+0x540c);
writel(0x0, bar_addr[0]+0x5408);
readl(bar_addr[0]+0x8);

writel(0x0, bar_addr[0]+0x100);
readl(bar_addr[0]+0x0);

writel(0x58140245, bar_addr[0]+0x0);
readl(bar_addr[0]+0x5600);

writel(0x1, bar_addr[0]+0x5600);
readl(bar_addr[0]+0x8);
readl(bar_addr[0]+0x5600);

writel(0x1, bar_addr[0]+0x5600);
readl(bar_addr[0]+0x8);

writel(0x1dea2000, bar_addr[0]+0x3800);
writel(0x0, bar_addr[0]+0x3804);
writel(0x1000, bar_addr[0]+0x3808);
writel(0x0, bar_addr[0]+0x3810);
writel(0x0, bar_addr[0]+0x3818);
writel(0x0, bar_addr[0]+0x3810);
writel(0x0, bar_addr[0]+0x3818);
writel(0x8, bar_addr[0]+0x3820);
writel(0x20, bar_addr[0]+0x382c);
readl(bar_addr[0]+0x3828);

writel(0x141011f, bar_addr[0]+0x3828);
readl(bar_addr[0]+0x3828);

writel(0x141011f, bar_addr[0]+0x3928);
readl(bar_addr[0]+0x400);

writel(0x10000f8, bar_addr[0]+0x400);
readl(bar_addr[0]+0x400);

writel(0x103f0f8, bar_addr[0]+0x400);
readl(bar_addr[0]+0x8);

writel(0xb028ebe2, bar_addr[0]+0x5c80);
writel(0xa85f3951, bar_addr[0]+0x5c84);
writel(0x1d61e42d, bar_addr[0]+0x5c88);
writel(0x661e0ab, bar_addr[0]+0x5c8c);
writel(0x5542ca31, bar_addr[0]+0x5c90);
writel(0xa1cd6213, bar_addr[0]+0x5c94);
writel(0xf31fd576, bar_addr[0]+0x5c98);
writel(0x71cfc95a, bar_addr[0]+0x5c9c);
writel(0xd394d0cb, bar_addr[0]+0x5ca0);
writel(0xf22e4b83, bar_addr[0]+0x5ca4);
writel(0x0, bar_addr[0]+0x5c00);
writel(0x0, bar_addr[0]+0x5c04);
writel(0x0, bar_addr[0]+0x5c08);
writel(0x0, bar_addr[0]+0x5c0c);
writel(0x0, bar_addr[0]+0x5c10);
writel(0x0, bar_addr[0]+0x5c14);
writel(0x0, bar_addr[0]+0x5c18);
writel(0x0, bar_addr[0]+0x5c1c);
writel(0x0, bar_addr[0]+0x5c20);
writel(0x0, bar_addr[0]+0x5c24);
writel(0x0, bar_addr[0]+0x5c28);
writel(0x0, bar_addr[0]+0x5c2c);
writel(0x0, bar_addr[0]+0x5c30);
writel(0x0, bar_addr[0]+0x5c34);
writel(0x0, bar_addr[0]+0x5c38);
writel(0x0, bar_addr[0]+0x5c3c);
writel(0x0, bar_addr[0]+0x5c40);
writel(0x0, bar_addr[0]+0x5c44);
writel(0x0, bar_addr[0]+0x5c48);
writel(0x0, bar_addr[0]+0x5c4c);
writel(0x0, bar_addr[0]+0x5c50);
writel(0x0, bar_addr[0]+0x5c54);
writel(0x0, bar_addr[0]+0x5c58);
writel(0x0, bar_addr[0]+0x5c5c);
writel(0x0, bar_addr[0]+0x5c60);
writel(0x0, bar_addr[0]+0x5c64);
writel(0x0, bar_addr[0]+0x5c68);
writel(0x0, bar_addr[0]+0x5c6c);
writel(0x0, bar_addr[0]+0x5c70);
writel(0x0, bar_addr[0]+0x5c74);
writel(0x0, bar_addr[0]+0x5c78);
writel(0x0, bar_addr[0]+0x5c7c);
readl(bar_addr[0]+0x5000);

writel(0x2300, bar_addr[0]+0x5000);
writel(0x370000, bar_addr[0]+0x5818);
readl(bar_addr[0]+0x100);
readl(bar_addr[0]+0x5008);

writel(0x8000, bar_addr[0]+0x5008);
writel(0x4008002, bar_addr[0]+0x100);
readl(bar_addr[0]+0x100);
readl(bar_addr[0]+0x8);

writel(0x1040420, bar_addr[0]+0x2828);
writel(0x1040420, bar_addr[0]+0x2928);
writel(0x20, bar_addr[0]+0x2820);
writel(0x20, bar_addr[0]+0x282c);
writel(0xc3, bar_addr[0]+0xe8);
writel(0xc3, bar_addr[0]+0xec);
writel(0xc3, bar_addr[0]+0xf0);
readl(bar_addr[0]+0x18);

writel(0xffffffff, bar_addr[0]+0xe0);
writel(0x18400000, bar_addr[0]+0x18);
readl(bar_addr[0]+0x8);

writel(0x1de3c000, bar_addr[0]+0x2800);
writel(0x0, bar_addr[0]+0x2804);
writel(0x1000, bar_addr[0]+0x2808);
writel(0x0, bar_addr[0]+0x2810);
writel(0x0, bar_addr[0]+0x2818);
writel(0x0, bar_addr[0]+0x2810);
writel(0x0, bar_addr[0]+0x2818);
readl(bar_addr[0]+0x5000);

writel(0x2300, bar_addr[0]+0x5000);
writel(0x4008002, bar_addr[0]+0x100);
writel(0x0, bar_addr[0]+0x2818);
writel(0x10, bar_addr[0]+0x2818);
writel(0x20, bar_addr[0]+0x2818);
writel(0x30, bar_addr[0]+0x2818);
writel(0x40, bar_addr[0]+0x2818);
writel(0x50, bar_addr[0]+0x2818);
writel(0x60, bar_addr[0]+0x2818);
writel(0x70, bar_addr[0]+0x2818);
writel(0x80, bar_addr[0]+0x2818);
writel(0x90, bar_addr[0]+0x2818);
writel(0xa0, bar_addr[0]+0x2818);
writel(0xb0, bar_addr[0]+0x2818);
writel(0xc0, bar_addr[0]+0x2818);
writel(0xd0, bar_addr[0]+0x2818);
writel(0xe0, bar_addr[0]+0x2818);
writel(0xf0, bar_addr[0]+0x2818);
writel(0xfee01004, bar_addr[3]+0x0);
writel(0x0, bar_addr[3]+0x4);
writel(0x4027, bar_addr[3]+0x8);
writel(0x0, bar_addr[3]+0xc);
readl(bar_addr[3]+0x0);

writel(0xfee01004, bar_addr[3]+0x0);
writel(0x0, bar_addr[3]+0x4);
writel(0x4027, bar_addr[3]+0x8);
writel(0xfee01004, bar_addr[3]+0x10);
writel(0x0, bar_addr[3]+0x14);
writel(0x4028, bar_addr[3]+0x18);
writel(0x0, bar_addr[3]+0x1c);
readl(bar_addr[3]+0x0);

writel(0xfee01004, bar_addr[3]+0x10);
writel(0x0, bar_addr[3]+0x14);
writel(0x4028, bar_addr[3]+0x18);
writel(0xfee01004, bar_addr[3]+0x20);
writel(0x0, bar_addr[3]+0x24);
writel(0x4029, bar_addr[3]+0x28);
writel(0x0, bar_addr[3]+0x2c);
readl(bar_addr[3]+0x0);

writel(0xfee01004, bar_addr[3]+0x20);
writel(0x0, bar_addr[3]+0x24);
writel(0x4029, bar_addr[3]+0x28);
readl(bar_addr[0]+0x5008);

writel(0x9000, bar_addr[0]+0x5008);
writel(0xc3, bar_addr[0]+0xe8);
writel(0xc3, bar_addr[0]+0xec);
writel(0xc3, bar_addr[0]+0xf0);
writel(0x800a0908, bar_addr[0]+0xe4);
readl(bar_addr[0]+0x18);

writel(0x91400000, bar_addr[0]+0x18);
readl(bar_addr[0]+0x8);

writel(0x500000, bar_addr[0]+0xdc);
writel(0x1570244, bar_addr[0]+0xd0);
readl(bar_addr[0]+0x8);

writel(0x1000004, bar_addr[0]+0xc8);
readl(bar_addr[0]+0xc0);

writel(0x1070244, bar_addr[0]+0xd0);
readl(bar_addr[0]+0x100);

writel(0x0, bar_addr[0]+0x53fc);
writel(0x0, bar_addr[0]+0x53f8);
writel(0x0, bar_addr[0]+0x53f4);
writel(0x0, bar_addr[0]+0x53f0);
writel(0x0, bar_addr[0]+0x53ec);
writel(0x0, bar_addr[0]+0x53e8);
writel(0x0, bar_addr[0]+0x53e4);
writel(0x0, bar_addr[0]+0x53e0);
writel(0x0, bar_addr[0]+0x53dc);
writel(0x0, bar_addr[0]+0x53d8);
writel(0x0, bar_addr[0]+0x53d4);
writel(0x0, bar_addr[0]+0x53d0);
writel(0x0, bar_addr[0]+0x53cc);
writel(0x0, bar_addr[0]+0x53c8);
writel(0x0, bar_addr[0]+0x53c4);
writel(0x0, bar_addr[0]+0x53c0);
writel(0x0, bar_addr[0]+0x53bc);
writel(0x0, bar_addr[0]+0x53b8);
writel(0x0, bar_addr[0]+0x53b4);
writel(0x0, bar_addr[0]+0x53b0);
writel(0x0, bar_addr[0]+0x53ac);
writel(0x0, bar_addr[0]+0x53a8);
writel(0x0, bar_addr[0]+0x53a4);
writel(0x0, bar_addr[0]+0x53a0);
writel(0x0, bar_addr[0]+0x539c);
writel(0x0, bar_addr[0]+0x5398);
writel(0x0, bar_addr[0]+0x5394);
writel(0x0, bar_addr[0]+0x5390);
writel(0x0, bar_addr[0]+0x538c);
writel(0x0, bar_addr[0]+0x5388);
writel(0x0, bar_addr[0]+0x5384);
writel(0x0, bar_addr[0]+0x5380);
writel(0x0, bar_addr[0]+0x537c);
writel(0x0, bar_addr[0]+0x5378);
writel(0x0, bar_addr[0]+0x5374);
writel(0x0, bar_addr[0]+0x5370);
writel(0x0, bar_addr[0]+0x536c);
writel(0x0, bar_addr[0]+0x5368);
writel(0x0, bar_addr[0]+0x5364);
writel(0x0, bar_addr[0]+0x5360);
writel(0x0, bar_addr[0]+0x535c);
writel(0x0, bar_addr[0]+0x5358);
writel(0x0, bar_addr[0]+0x5354);
writel(0x0, bar_addr[0]+0x5350);
writel(0x0, bar_addr[0]+0x534c);
writel(0x0, bar_addr[0]+0x5348);
writel(0x0, bar_addr[0]+0x5344);
writel(0x0, bar_addr[0]+0x5340);
writel(0x0, bar_addr[0]+0x533c);
writel(0x0, bar_addr[0]+0x5338);
writel(0x0, bar_addr[0]+0x5334);
writel(0x0, bar_addr[0]+0x5330);
writel(0x0, bar_addr[0]+0x532c);
writel(0x0, bar_addr[0]+0x5328);
writel(0x0, bar_addr[0]+0x5324);
writel(0x0, bar_addr[0]+0x5320);
writel(0x0, bar_addr[0]+0x531c);
writel(0x0, bar_addr[0]+0x5318);
writel(0x0, bar_addr[0]+0x5314);
writel(0x0, bar_addr[0]+0x5310);
writel(0x0, bar_addr[0]+0x530c);
writel(0x0, bar_addr[0]+0x5308);
writel(0x0, bar_addr[0]+0x5304);
writel(0x0, bar_addr[0]+0x5300);
writel(0x0, bar_addr[0]+0x52fc);
writel(0x0, bar_addr[0]+0x52f8);
writel(0x0, bar_addr[0]+0x52f4);
writel(0x0, bar_addr[0]+0x52f0);
writel(0x0, bar_addr[0]+0x52ec);
writel(0x0, bar_addr[0]+0x52e8);
writel(0x0, bar_addr[0]+0x52e4);
writel(0x0, bar_addr[0]+0x52e0);
writel(0x0, bar_addr[0]+0x52dc);
writel(0x0, bar_addr[0]+0x52d8);
writel(0x0, bar_addr[0]+0x52d4);
writel(0x0, bar_addr[0]+0x52d0);
writel(0x0, bar_addr[0]+0x52cc);
writel(0x0, bar_addr[0]+0x52c8);
writel(0x0, bar_addr[0]+0x52c4);
writel(0x0, bar_addr[0]+0x52c0);
writel(0x0, bar_addr[0]+0x52bc);
writel(0x0, bar_addr[0]+0x52b8);
writel(0x0, bar_addr[0]+0x52b4);
writel(0x0, bar_addr[0]+0x52b0);
writel(0x0, bar_addr[0]+0x52ac);
writel(0x0, bar_addr[0]+0x52a8);
writel(0x0, bar_addr[0]+0x52a4);
writel(0x0, bar_addr[0]+0x52a0);
writel(0x0, bar_addr[0]+0x529c);
writel(0x0, bar_addr[0]+0x5298);
writel(0x0, bar_addr[0]+0x5294);
writel(0x0, bar_addr[0]+0x5290);
writel(0x0, bar_addr[0]+0x528c);
writel(0x0, bar_addr[0]+0x5288);
writel(0x0, bar_addr[0]+0x5284);
writel(0x0, bar_addr[0]+0x5280);
writel(0x0, bar_addr[0]+0x527c);
writel(0x0, bar_addr[0]+0x5278);
writel(0x0, bar_addr[0]+0x5274);
writel(0x0, bar_addr[0]+0x5270);
writel(0x0, bar_addr[0]+0x526c);
writel(0x0, bar_addr[0]+0x5268);
writel(0x0, bar_addr[0]+0x5264);
writel(0x0, bar_addr[0]+0x5260);
writel(0x0, bar_addr[0]+0x525c);
writel(0x0, bar_addr[0]+0x5258);
writel(0x0, bar_addr[0]+0x5254);
writel(0x0, bar_addr[0]+0x5250);
writel(0x0, bar_addr[0]+0x524c);
writel(0x0, bar_addr[0]+0x5248);
writel(0x0, bar_addr[0]+0x5244);
writel(0x0, bar_addr[0]+0x5240);
writel(0x0, bar_addr[0]+0x523c);
writel(0x0, bar_addr[0]+0x5238);
writel(0x0, bar_addr[0]+0x5234);
writel(0x0, bar_addr[0]+0x5230);
writel(0x0, bar_addr[0]+0x522c);
writel(0x0, bar_addr[0]+0x5228);
writel(0x0, bar_addr[0]+0x5224);
writel(0x0, bar_addr[0]+0x5220);
writel(0x0, bar_addr[0]+0x521c);
writel(0x0, bar_addr[0]+0x5218);
writel(0x0, bar_addr[0]+0x5214);
writel(0x0, bar_addr[0]+0x5210);
writel(0x0, bar_addr[0]+0x520c);
writel(0x0, bar_addr[0]+0x5208);
writel(0x0, bar_addr[0]+0x5204);
writel(0x10000, bar_addr[0]+0x5200);
readl(bar_addr[0]+0x8);
readl(bar_addr[0]+0x100);

writel(0x4048002, bar_addr[0]+0x100);
writel(0x0, bar_addr[0]+0x5474);
writel(0x0, bar_addr[0]+0x5470);
writel(0x0, bar_addr[0]+0x546c);
writel(0x0, bar_addr[0]+0x5468);
writel(0x0, bar_addr[0]+0x5464);
writel(0x0, bar_addr[0]+0x5460);
writel(0x0, bar_addr[0]+0x545c);
writel(0x0, bar_addr[0]+0x5458);
writel(0x0, bar_addr[0]+0x5454);
writel(0x0, bar_addr[0]+0x5450);
writel(0x0, bar_addr[0]+0x544c);
writel(0x0, bar_addr[0]+0x5448);
writel(0x0, bar_addr[0]+0x5444);
writel(0x0, bar_addr[0]+0x5440);
writel(0x0, bar_addr[0]+0x543c);
writel(0x0, bar_addr[0]+0x5438);
writel(0x0, bar_addr[0]+0x5434);
writel(0x0, bar_addr[0]+0x5430);
writel(0x0, bar_addr[0]+0x542c);
writel(0x0, bar_addr[0]+0x5428);
writel(0x0, bar_addr[0]+0x5424);
writel(0x0, bar_addr[0]+0x5420);
writel(0x0, bar_addr[0]+0x541c);
writel(0x0, bar_addr[0]+0x5418);
writel(0x0, bar_addr[0]+0x5414);
writel(0x0, bar_addr[0]+0x5410);
writel(0x0, bar_addr[0]+0x540c);
writel(0x0, bar_addr[0]+0x5408);
readl(bar_addr[0]+0x8);

writel(0x4008002, bar_addr[0]+0x100);
readl(bar_addr[0]+0x0);

writel(0x58140245, bar_addr[0]+0x0);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
writel(0x3d, bar_addr[0]+0x14);
readl(bar_addr[0]+0x14);
readl(bar_addr[0]+0x14);

writel(0x100000, bar_addr[0]+0xc8);
writel(0x80000008, bar_addr[0]+0x3820);
writel(0x80000020, bar_addr[0]+0x2820);
readl(bar_addr[0]+0x8);

writel(0x80000008, bar_addr[0]+0x3820);
writel(0x80000020, bar_addr[0]+0x2820);
readl(bar_addr[0]+0x8);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x8350000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);

writel(0x100000, bar_addr[0]+0xd0);
readl(bar_addr[0]+0x100);

writel(0x0, bar_addr[0]+0x53fc);
writel(0x0, bar_addr[0]+0x53f8);
writel(0x0, bar_addr[0]+0x53f4);
writel(0x0, bar_addr[0]+0x53f0);
writel(0x0, bar_addr[0]+0x53ec);
writel(0x0, bar_addr[0]+0x53e8);
writel(0x0, bar_addr[0]+0x53e4);
writel(0x0, bar_addr[0]+0x53e0);
writel(0x0, bar_addr[0]+0x53dc);
writel(0x0, bar_addr[0]+0x53d8);
writel(0x0, bar_addr[0]+0x53d4);
writel(0x0, bar_addr[0]+0x53d0);
writel(0x0, bar_addr[0]+0x53cc);
writel(0x0, bar_addr[0]+0x53c8);
writel(0x0, bar_addr[0]+0x53c4);
writel(0x0, bar_addr[0]+0x53c0);
writel(0x0, bar_addr[0]+0x53bc);
writel(0x0, bar_addr[0]+0x53b8);
writel(0x0, bar_addr[0]+0x53b4);
writel(0x0, bar_addr[0]+0x53b0);
writel(0x0, bar_addr[0]+0x53ac);
writel(0x0, bar_addr[0]+0x53a8);
writel(0x0, bar_addr[0]+0x53a4);
writel(0x0, bar_addr[0]+0x53a0);
writel(0x0, bar_addr[0]+0x539c);
writel(0x0, bar_addr[0]+0x5398);
writel(0x0, bar_addr[0]+0x5394);
writel(0x0, bar_addr[0]+0x5390);
writel(0x0, bar_addr[0]+0x538c);
writel(0x0, bar_addr[0]+0x5388);
writel(0x0, bar_addr[0]+0x5384);
writel(0x0, bar_addr[0]+0x5380);
writel(0x0, bar_addr[0]+0x537c);
writel(0x0, bar_addr[0]+0x5378);
writel(0x0, bar_addr[0]+0x5374);
writel(0x0, bar_addr[0]+0x5370);
writel(0x0, bar_addr[0]+0x536c);
writel(0x0, bar_addr[0]+0x5368);
writel(0x0, bar_addr[0]+0x5364);
writel(0x0, bar_addr[0]+0x5360);
writel(0x0, bar_addr[0]+0x535c);
writel(0x0, bar_addr[0]+0x5358);
writel(0x0, bar_addr[0]+0x5354);
writel(0x0, bar_addr[0]+0x5350);
writel(0x0, bar_addr[0]+0x534c);
writel(0x0, bar_addr[0]+0x5348);
writel(0x0, bar_addr[0]+0x5344);
writel(0x0, bar_addr[0]+0x5340);
writel(0x0, bar_addr[0]+0x533c);
writel(0x0, bar_addr[0]+0x5338);
writel(0x0, bar_addr[0]+0x5334);
writel(0x0, bar_addr[0]+0x5330);
writel(0x0, bar_addr[0]+0x532c);
writel(0x0, bar_addr[0]+0x5328);
writel(0x0, bar_addr[0]+0x5324);
writel(0x0, bar_addr[0]+0x5320);
writel(0x0, bar_addr[0]+0x531c);
writel(0x0, bar_addr[0]+0x5318);
writel(0x0, bar_addr[0]+0x5314);
writel(0x0, bar_addr[0]+0x5310);
writel(0x0, bar_addr[0]+0x530c);
writel(0x0, bar_addr[0]+0x5308);
writel(0x0, bar_addr[0]+0x5304);
writel(0x0, bar_addr[0]+0x5300);
writel(0x0, bar_addr[0]+0x52fc);
writel(0x0, bar_addr[0]+0x52f8);
writel(0x0, bar_addr[0]+0x52f4);
writel(0x0, bar_addr[0]+0x52f0);
writel(0x0, bar_addr[0]+0x52ec);
writel(0x0, bar_addr[0]+0x52e8);
writel(0x0, bar_addr[0]+0x52e4);
writel(0x0, bar_addr[0]+0x52e0);
writel(0x0, bar_addr[0]+0x52dc);
writel(0x0, bar_addr[0]+0x52d8);
writel(0x0, bar_addr[0]+0x52d4);
writel(0x0, bar_addr[0]+0x52d0);
writel(0x0, bar_addr[0]+0x52cc);
writel(0x0, bar_addr[0]+0x52c8);
writel(0x0, bar_addr[0]+0x52c4);
writel(0x0, bar_addr[0]+0x52c0);
writel(0x0, bar_addr[0]+0x52bc);
writel(0x0, bar_addr[0]+0x52b8);
writel(0x0, bar_addr[0]+0x52b4);
writel(0x0, bar_addr[0]+0x52b0);
writel(0x0, bar_addr[0]+0x52ac);
writel(0x0, bar_addr[0]+0x52a8);
writel(0x0, bar_addr[0]+0x52a4);
writel(0x0, bar_addr[0]+0x52a0);
writel(0x0, bar_addr[0]+0x529c);
writel(0x0, bar_addr[0]+0x5298);
writel(0x0, bar_addr[0]+0x5294);
writel(0x0, bar_addr[0]+0x5290);
writel(0x0, bar_addr[0]+0x528c);
writel(0x0, bar_addr[0]+0x5288);
writel(0x0, bar_addr[0]+0x5284);
writel(0x0, bar_addr[0]+0x5280);
writel(0x0, bar_addr[0]+0x527c);
writel(0x0, bar_addr[0]+0x5278);
writel(0x0, bar_addr[0]+0x5274);
writel(0x0, bar_addr[0]+0x5270);
writel(0x0, bar_addr[0]+0x526c);
writel(0x0, bar_addr[0]+0x5268);
writel(0x0, bar_addr[0]+0x5264);
writel(0x0, bar_addr[0]+0x5260);
writel(0x0, bar_addr[0]+0x525c);
writel(0x0, bar_addr[0]+0x5258);
writel(0x0, bar_addr[0]+0x5254);
writel(0x0, bar_addr[0]+0x5250);
writel(0x0, bar_addr[0]+0x524c);
writel(0x0, bar_addr[0]+0x5248);
writel(0x0, bar_addr[0]+0x5244);
writel(0x0, bar_addr[0]+0x5240);
writel(0x0, bar_addr[0]+0x523c);
writel(0x0, bar_addr[0]+0x5238);
writel(0x0, bar_addr[0]+0x5234);
writel(0x0, bar_addr[0]+0x5230);
writel(0x0, bar_addr[0]+0x522c);
writel(0x0, bar_addr[0]+0x5228);
writel(0x0, bar_addr[0]+0x5224);
writel(0x0, bar_addr[0]+0x5220);
writel(0x0, bar_addr[0]+0x521c);
writel(0x0, bar_addr[0]+0x5218);
writel(0x0, bar_addr[0]+0x5214);
writel(0x0, bar_addr[0]+0x5210);
writel(0x0, bar_addr[0]+0x520c);
writel(0x0, bar_addr[0]+0x5208);
writel(0x0, bar_addr[0]+0x5204);
writel(0x10000, bar_addr[0]+0x5200);
readl(bar_addr[0]+0x8);
readl(bar_addr[0]+0x100);

writel(0x4048002, bar_addr[0]+0x100);
writel(0x0, bar_addr[0]+0x5474);
writel(0x0, bar_addr[0]+0x5470);
writel(0x0, bar_addr[0]+0x546c);
writel(0x0, bar_addr[0]+0x5468);
writel(0x0, bar_addr[0]+0x5464);
writel(0x0, bar_addr[0]+0x5460);
writel(0x0, bar_addr[0]+0x545c);
writel(0x0, bar_addr[0]+0x5458);
writel(0x0, bar_addr[0]+0x5454);
writel(0x0, bar_addr[0]+0x5450);
writel(0x0, bar_addr[0]+0x544c);
writel(0x0, bar_addr[0]+0x5448);
writel(0x0, bar_addr[0]+0x5444);
writel(0x0, bar_addr[0]+0x5440);
writel(0x0, bar_addr[0]+0x543c);
writel(0x0, bar_addr[0]+0x5438);
writel(0x0, bar_addr[0]+0x5434);
writel(0x0, bar_addr[0]+0x5430);
writel(0x0, bar_addr[0]+0x542c);
writel(0x0, bar_addr[0]+0x5428);
writel(0x0, bar_addr[0]+0x5424);
writel(0x0, bar_addr[0]+0x5420);
writel(0x0, bar_addr[0]+0x541c);
writel(0x0, bar_addr[0]+0x5418);
writel(0x0, bar_addr[0]+0x5414);
writel(0x0, bar_addr[0]+0x5410);
writel(0x0, bar_addr[0]+0x540c);
writel(0x0, bar_addr[0]+0x5408);
readl(bar_addr[0]+0x8);

writel(0x4008002, bar_addr[0]+0x100);
readl(bar_addr[0]+0x0);

writel(0x58140245, bar_addr[0]+0x0);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x8310000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0x400);

writel(0x103f0f8, bar_addr[0]+0x400);
readl(bar_addr[0]+0x8);
readl(bar_addr[0]+0x0);

writel(0x58140245, bar_addr[0]+0x0);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8240000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8250000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0x8);
readl(bar_addr[0]+0x0);

writel(0x58140245, bar_addr[0]+0x0);
writel(0x3d, bar_addr[0]+0x14);
readl(bar_addr[0]+0x14);
readl(bar_addr[0]+0x14);
readl(bar_addr[0]+0x8);
readl(bar_addr[0]+0x8);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8200000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8240000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8250000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8260000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8290000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x82a0000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x82f0000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0x8);
readl(bar_addr[0]+0x0);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x8310000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0x400);

writel(0x103f0fa, bar_addr[0]+0x400);
readl(bar_addr[0]+0x4000);
readl(bar_addr[0]+0x4074);
readl(bar_addr[0]+0x4088);
readl(bar_addr[0]+0x408c);
readl(bar_addr[0]+0x4078);
readl(bar_addr[0]+0x407c);
readl(bar_addr[0]+0x40ac);
readl(bar_addr[0]+0x4010);
readl(bar_addr[0]+0x4048);
readl(bar_addr[0]+0x404c);
readl(bar_addr[0]+0x4050);
readl(bar_addr[0]+0x4054);
readl(bar_addr[0]+0x4080);
readl(bar_addr[0]+0x4090);
readl(bar_addr[0]+0x4094);
readl(bar_addr[0]+0x40a0);
readl(bar_addr[0]+0x40a4);
readl(bar_addr[0]+0x40f0);
readl(bar_addr[0]+0x40f4);
readl(bar_addr[0]+0x40d4);
readl(bar_addr[0]+0x4004);
readl(bar_addr[0]+0x400c);
readl(bar_addr[0]+0x403c);
readl(bar_addr[0]+0x40f8);
readl(bar_addr[0]+0x40fc);
readl(bar_addr[0]+0x40bc);
readl(bar_addr[0]+0x40b4);
readl(bar_addr[0]+0x40b8);

writel(0x100000, bar_addr[0]+0xc8);
writel(0x3d0, bar_addr[0]+0xe8);
writel(0x80000008, bar_addr[0]+0x3820);
writel(0x80000020, bar_addr[0]+0x2820);
readl(bar_addr[0]+0x8);

writel(0x80000008, bar_addr[0]+0x3820);
writel(0x80000020, bar_addr[0]+0x2820);
readl(bar_addr[0]+0x8);

writel(0x100000, bar_addr[0]+0xd0);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x8350000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0x4000);
readl(bar_addr[0]+0x4074);
readl(bar_addr[0]+0x4088);
readl(bar_addr[0]+0x408c);
readl(bar_addr[0]+0x4078);
readl(bar_addr[0]+0x407c);
readl(bar_addr[0]+0x40ac);
readl(bar_addr[0]+0x4010);
readl(bar_addr[0]+0x4048);
readl(bar_addr[0]+0x404c);
readl(bar_addr[0]+0x4050);
readl(bar_addr[0]+0x4054);
readl(bar_addr[0]+0x4080);
readl(bar_addr[0]+0x4090);
readl(bar_addr[0]+0x4094);
readl(bar_addr[0]+0x40a0);
readl(bar_addr[0]+0x40a4);
readl(bar_addr[0]+0x40f0);
readl(bar_addr[0]+0x40f4);
readl(bar_addr[0]+0x40d4);
readl(bar_addr[0]+0x4004);
readl(bar_addr[0]+0x400c);
readl(bar_addr[0]+0x403c);
readl(bar_addr[0]+0x40f8);
readl(bar_addr[0]+0x40fc);
readl(bar_addr[0]+0x40bc);
readl(bar_addr[0]+0x40b4);
readl(bar_addr[0]+0x40b8);
readl(bar_addr[0]+0x100);

writel(0x0, bar_addr[0]+0x53fc);
writel(0x0, bar_addr[0]+0x53f8);
writel(0x0, bar_addr[0]+0x53f4);
writel(0x0, bar_addr[0]+0x53f0);
writel(0x0, bar_addr[0]+0x53ec);
writel(0x0, bar_addr[0]+0x53e8);
writel(0x0, bar_addr[0]+0x53e4);
writel(0x0, bar_addr[0]+0x53e0);
writel(0x0, bar_addr[0]+0x53dc);
writel(0x0, bar_addr[0]+0x53d8);
writel(0x0, bar_addr[0]+0x53d4);
writel(0x0, bar_addr[0]+0x53d0);
writel(0x0, bar_addr[0]+0x53cc);
writel(0x0, bar_addr[0]+0x53c8);
writel(0x0, bar_addr[0]+0x53c4);
writel(0x0, bar_addr[0]+0x53c0);
writel(0x0, bar_addr[0]+0x53bc);
writel(0x0, bar_addr[0]+0x53b8);
writel(0x0, bar_addr[0]+0x53b4);
writel(0x0, bar_addr[0]+0x53b0);
writel(0x0, bar_addr[0]+0x53ac);
writel(0x0, bar_addr[0]+0x53a8);
writel(0x0, bar_addr[0]+0x53a4);
writel(0x0, bar_addr[0]+0x53a0);
writel(0x0, bar_addr[0]+0x539c);
writel(0x0, bar_addr[0]+0x5398);
writel(0x0, bar_addr[0]+0x5394);
writel(0x0, bar_addr[0]+0x5390);
writel(0x0, bar_addr[0]+0x538c);
writel(0x0, bar_addr[0]+0x5388);
writel(0x0, bar_addr[0]+0x5384);
writel(0x0, bar_addr[0]+0x5380);
writel(0x0, bar_addr[0]+0x537c);
writel(0x0, bar_addr[0]+0x5378);
writel(0x0, bar_addr[0]+0x5374);
writel(0x0, bar_addr[0]+0x5370);
writel(0x0, bar_addr[0]+0x536c);
writel(0x0, bar_addr[0]+0x5368);
writel(0x0, bar_addr[0]+0x5364);
writel(0x0, bar_addr[0]+0x5360);
writel(0x0, bar_addr[0]+0x535c);
writel(0x0, bar_addr[0]+0x5358);
writel(0x0, bar_addr[0]+0x5354);
writel(0x0, bar_addr[0]+0x5350);
writel(0x0, bar_addr[0]+0x534c);
writel(0x0, bar_addr[0]+0x5348);
writel(0x0, bar_addr[0]+0x5344);
writel(0x0, bar_addr[0]+0x5340);
writel(0x0, bar_addr[0]+0x533c);
writel(0x0, bar_addr[0]+0x5338);
writel(0x0, bar_addr[0]+0x5334);
writel(0x0, bar_addr[0]+0x5330);
writel(0x0, bar_addr[0]+0x532c);
writel(0x0, bar_addr[0]+0x5328);
writel(0x0, bar_addr[0]+0x5324);
writel(0x0, bar_addr[0]+0x5320);
writel(0x0, bar_addr[0]+0x531c);
writel(0x0, bar_addr[0]+0x5318);
writel(0x0, bar_addr[0]+0x5314);
writel(0x0, bar_addr[0]+0x5310);
writel(0x0, bar_addr[0]+0x530c);
writel(0x0, bar_addr[0]+0x5308);
writel(0x0, bar_addr[0]+0x5304);
writel(0x0, bar_addr[0]+0x5300);
writel(0x0, bar_addr[0]+0x52fc);
writel(0x0, bar_addr[0]+0x52f8);
writel(0x0, bar_addr[0]+0x52f4);
writel(0x0, bar_addr[0]+0x52f0);
writel(0x0, bar_addr[0]+0x52ec);
writel(0x0, bar_addr[0]+0x52e8);
writel(0x0, bar_addr[0]+0x52e4);
writel(0x0, bar_addr[0]+0x52e0);
writel(0x0, bar_addr[0]+0x52dc);
writel(0x0, bar_addr[0]+0x52d8);
writel(0x0, bar_addr[0]+0x52d4);
writel(0x0, bar_addr[0]+0x52d0);
writel(0x0, bar_addr[0]+0x52cc);
writel(0x0, bar_addr[0]+0x52c8);
writel(0x0, bar_addr[0]+0x52c4);
writel(0x0, bar_addr[0]+0x52c0);
writel(0x0, bar_addr[0]+0x52bc);
writel(0x0, bar_addr[0]+0x52b8);
writel(0x0, bar_addr[0]+0x52b4);
writel(0x0, bar_addr[0]+0x52b0);
writel(0x8, bar_addr[0]+0x52ac);
writel(0x0, bar_addr[0]+0x52a8);
writel(0x0, bar_addr[0]+0x52a4);
writel(0x0, bar_addr[0]+0x52a0);
writel(0x0, bar_addr[0]+0x529c);
writel(0x0, bar_addr[0]+0x5298);
writel(0x0, bar_addr[0]+0x5294);
writel(0x0, bar_addr[0]+0x5290);
writel(0x0, bar_addr[0]+0x528c);
writel(0x0, bar_addr[0]+0x5288);
writel(0x0, bar_addr[0]+0x5284);
writel(0x0, bar_addr[0]+0x5280);
writel(0x0, bar_addr[0]+0x527c);
writel(0x0, bar_addr[0]+0x5278);
writel(0x0, bar_addr[0]+0x5274);
writel(0x0, bar_addr[0]+0x5270);
writel(0x0, bar_addr[0]+0x526c);
writel(0x0, bar_addr[0]+0x5268);
writel(0x0, bar_addr[0]+0x5264);
writel(0x0, bar_addr[0]+0x5260);
writel(0x0, bar_addr[0]+0x525c);
writel(0x0, bar_addr[0]+0x5258);
writel(0x0, bar_addr[0]+0x5254);
writel(0x0, bar_addr[0]+0x5250);
writel(0x0, bar_addr[0]+0x524c);
writel(0x0, bar_addr[0]+0x5248);
writel(0x0, bar_addr[0]+0x5244);
writel(0x0, bar_addr[0]+0x5240);
writel(0x0, bar_addr[0]+0x523c);
writel(0x0, bar_addr[0]+0x5238);
writel(0x0, bar_addr[0]+0x5234);
writel(0x0, bar_addr[0]+0x5230);
writel(0x0, bar_addr[0]+0x522c);
writel(0x0, bar_addr[0]+0x5228);
writel(0x0, bar_addr[0]+0x5224);
writel(0x0, bar_addr[0]+0x5220);
writel(0x0, bar_addr[0]+0x521c);
writel(0x0, bar_addr[0]+0x5218);
writel(0x0, bar_addr[0]+0x5214);
writel(0x0, bar_addr[0]+0x5210);
writel(0x0, bar_addr[0]+0x520c);
writel(0x0, bar_addr[0]+0x5208);
writel(0x0, bar_addr[0]+0x5204);
writel(0x10000, bar_addr[0]+0x5200);
readl(bar_addr[0]+0x8);
readl(bar_addr[0]+0x100);

writel(0x4048002, bar_addr[0]+0x100);
writel(0x0, bar_addr[0]+0x5474);
writel(0x0, bar_addr[0]+0x5470);
writel(0x0, bar_addr[0]+0x546c);
writel(0x0, bar_addr[0]+0x5468);
writel(0x0, bar_addr[0]+0x5464);
writel(0x0, bar_addr[0]+0x5460);
writel(0x0, bar_addr[0]+0x545c);
writel(0x0, bar_addr[0]+0x5458);
writel(0x0, bar_addr[0]+0x5454);
writel(0x0, bar_addr[0]+0x5450);
writel(0x0, bar_addr[0]+0x544c);
writel(0x0, bar_addr[0]+0x5448);
writel(0x0, bar_addr[0]+0x5444);
writel(0x0, bar_addr[0]+0x5440);
writel(0x0, bar_addr[0]+0x543c);
writel(0x0, bar_addr[0]+0x5438);
writel(0x0, bar_addr[0]+0x5434);
writel(0x0, bar_addr[0]+0x5430);
writel(0x0, bar_addr[0]+0x542c);
writel(0x0, bar_addr[0]+0x5428);
writel(0x0, bar_addr[0]+0x5424);
writel(0x0, bar_addr[0]+0x5420);
writel(0x0, bar_addr[0]+0x541c);
writel(0x0, bar_addr[0]+0x5418);
writel(0x0, bar_addr[0]+0x5414);
writel(0x0, bar_addr[0]+0x5410);
writel(0x0, bar_addr[0]+0x540c);
writel(0x0, bar_addr[0]+0x5408);
readl(bar_addr[0]+0x8);

writel(0x4008002, bar_addr[0]+0x100);
readl(bar_addr[0]+0x0);

writel(0x58140245, bar_addr[0]+0x0);
writel(0x1, bar_addr[0]+0x3818);

writel(0x400000, bar_addr[0]+0xd0);
writel(0x2, bar_addr[0]+0x3818);

writel(0x400000, bar_addr[0]+0xd0);
writel(0x3, bar_addr[0]+0x3818);

writel(0x400000, bar_addr[0]+0xd0);
writel(0x4, bar_addr[0]+0x3818);

writel(0x400000, bar_addr[0]+0xd0);
writel(0x5, bar_addr[0]+0x3818);

writel(0x1b2, bar_addr[0]+0xe8);
writel(0x400000, bar_addr[0]+0xd0);

writel(0x100000, bar_addr[0]+0xd0);
readl(bar_addr[0]+0x4000);
readl(bar_addr[0]+0x4074);
readl(bar_addr[0]+0x4088);
readl(bar_addr[0]+0x408c);
readl(bar_addr[0]+0x4078);
readl(bar_addr[0]+0x407c);
readl(bar_addr[0]+0x40ac);
readl(bar_addr[0]+0x4010);
readl(bar_addr[0]+0x4048);
readl(bar_addr[0]+0x404c);
readl(bar_addr[0]+0x4050);
readl(bar_addr[0]+0x4054);
readl(bar_addr[0]+0x4080);
readl(bar_addr[0]+0x4090);
readl(bar_addr[0]+0x4094);
readl(bar_addr[0]+0x40a0);
readl(bar_addr[0]+0x40a4);
readl(bar_addr[0]+0x40f0);
readl(bar_addr[0]+0x40f4);
readl(bar_addr[0]+0x40d4);
readl(bar_addr[0]+0x4004);
readl(bar_addr[0]+0x400c);
readl(bar_addr[0]+0x403c);
readl(bar_addr[0]+0x40f8);
readl(bar_addr[0]+0x40fc);
readl(bar_addr[0]+0x40bc);
readl(bar_addr[0]+0x40b4);
readl(bar_addr[0]+0x40b8);

writel(0x100000, bar_addr[0]+0xc8);
writel(0x117, bar_addr[0]+0xe8);
writel(0x80000008, bar_addr[0]+0x3820);
writel(0x80000020, bar_addr[0]+0x2820);
readl(bar_addr[0]+0x8);

writel(0x80000008, bar_addr[0]+0x3820);
writel(0x80000020, bar_addr[0]+0x2820);
readl(bar_addr[0]+0x8);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x8350000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x8210000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x8300000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x8310000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x8310000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x8310000, bar_addr[0]+0x20);

writel(0x100000, bar_addr[0]+0xd0);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x82a0000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);
writel(0x6, bar_addr[0]+0x3818);

writel(0x400000, bar_addr[0]+0xd0);
writel(0x7, bar_addr[0]+0x3818);

writel(0x400000, bar_addr[0]+0xd0);
writel(0xcd, bar_addr[0]+0xe8);

writel(0x0, bar_addr[0]+0x2818);
writel(0x100000, bar_addr[0]+0xd0);
writel(0x8, bar_addr[0]+0x3818);

writel(0xc3, bar_addr[0]+0xe8);
writel(0x400000, bar_addr[0]+0xd0);

writel(0x100000, bar_addr[0]+0xd0);

writel(0x100000, bar_addr[0]+0xd0);
writel(0x9, bar_addr[0]+0x3818);

writel(0x400000, bar_addr[0]+0xd0);

writel(0x100000, bar_addr[0]+0xd0);
readl(bar_addr[0]+0x4000);
readl(bar_addr[0]+0x4074);
readl(bar_addr[0]+0x4088);
readl(bar_addr[0]+0x408c);
readl(bar_addr[0]+0x4078);
readl(bar_addr[0]+0x407c);
readl(bar_addr[0]+0x40ac);
readl(bar_addr[0]+0x4010);
readl(bar_addr[0]+0x4048);
readl(bar_addr[0]+0x404c);
readl(bar_addr[0]+0x4050);
readl(bar_addr[0]+0x4054);
readl(bar_addr[0]+0x4080);
readl(bar_addr[0]+0x4090);
readl(bar_addr[0]+0x4094);
readl(bar_addr[0]+0x40a0);
readl(bar_addr[0]+0x40a4);
readl(bar_addr[0]+0x40f0);
readl(bar_addr[0]+0x40f4);
readl(bar_addr[0]+0x40d4);
readl(bar_addr[0]+0x4004);
readl(bar_addr[0]+0x400c);
readl(bar_addr[0]+0x403c);
readl(bar_addr[0]+0x40f8);
readl(bar_addr[0]+0x40fc);
readl(bar_addr[0]+0x40bc);
readl(bar_addr[0]+0x40b4);
readl(bar_addr[0]+0x40b8);

writel(0x100000, bar_addr[0]+0xc8);
writel(0x80000008, bar_addr[0]+0x3820);
writel(0x80000020, bar_addr[0]+0x2820);
readl(bar_addr[0]+0x8);

writel(0x80000008, bar_addr[0]+0x3820);
writel(0x80000020, bar_addr[0]+0x2820);
readl(bar_addr[0]+0x8);
readl(bar_addr[0]+0xf00);

writel(0x28, bar_addr[0]+0xf00);
readl(bar_addr[0]+0xf00);

writel(0x4360000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);

writel(0x8350000, bar_addr[0]+0x20);
readl(bar_addr[0]+0x20);
readl(bar_addr[0]+0xf00);

writel(0x8, bar_addr[0]+0xf00);

writel(0x100000, bar_addr[0]+0xd0);

writel(0xa, bar_addr[0]+0x3818);

writel(0x400000, bar_addr[0]+0xd0);
writel(0x100000, bar_addr[0]+0xd0);

writel(0xc, bar_addr[0]+0x3818);

writel(0x400000, bar_addr[0]+0xd0);
writel(0x100000, bar_addr[0]+0xd0);

writel(0xe, bar_addr[0]+0x3818);

writel(0x400000, bar_addr[0]+0xd0);
writel(0x100000, bar_addr[0]+0xd0);
writel(0x11, bar_addr[0]+0x3818);


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

        add_DMA_blk(0,0x1111,0);
        add_DMA_blk(0,0x2222,0);
        add_DMA_blk(0,0x3333,0);

        print_DMA_blks();

        /*
        enum_dev();
        suppress_drivers();
        repro();
        native_irq_enable();
         * */

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
