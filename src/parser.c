#include <stdio.h> 
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

struct op_log {
        uint16_t op_type:3;    /* (0-7) 0x0: NO 0x1: PIO 0x2: MMIO 0x3: DMA 0x4: INT(start) 0x5: INT(end) 0x6: next pattern */
        uint16_t op_size:2;    /* (0-3) 0x0: b (1byte) 0x1: l (2byte) 0x2: w (4byte) */
        uint16_t op_rw:1;      /* (0-1) 0x0: r 0x1: w */
        uint16_t is_irq:1;     /* (0-1) 0x0: not interrupted 0x1: interrupted */
        uint16_t is_pci:1;
        uint16_t is_alloc:1;
        uint16_t is_free:1;
        uint16_t reserved:6;  /* Reserved */

        uint16_t bar_idx;
        uint32_t val;
        uint32_t addr;
        uint32_t vendor;
        uint32_t device;
};

typedef struct _DMA_block{
        uint32_t base;                   /* base address of allocated page */
        uint32_t vendor;                 /* vendor id for device identification */
        uint32_t device;                 /* device id for device identification */
        LIST_ENTRY(_DMA_block) ptr;      /* list node */
}DMA_blk;

DMA_blk *create_DMA_blk(uint32_t base, uint32_t vendor, uint32_t device);
LIST_HEAD(DMA_list,_DMA_block) DMA_blks;

#define PAGE_SIZE 1UL << 12
#define DATA_SIZE PAGE_SIZE<<8

#define DMASTART 0x10000000
#define DMASTOP  0x1FF00000

#define OPTYPE_NONE 0x0
#define OPTYPE_PIO 0x1
#define OPTYPE_MMIO 0x2
#define OPTYPE_DMA 0x3
#define OPTYPE_NEXT 0x6

#define OPRW_READ 0x0
#define OPRW_WRITE 0x1

void printf_width(int op_size);
void printf_io(struct op_log *log);

int main(int argc, char** argv)
{
        int size = sizeof(struct op_log);
        struct op_log *log;
        unsigned int offset = 0;
        long lSize;
        char *buffer;
        size_t remain;
        unsigned int vid = -1;
        unsigned int did = -1;
        int dma_exists = 0;

        FILE *fp = fopen(argv[1], "r");
        if(fp == NULL) {
                perror("fopen");
                exit(1);
        }
        fseek(fp, 0, SEEK_END);
        lSize = ftell(fp);
        rewind(fp);
        buffer = (char*)malloc(sizeof(char) * lSize);
        if (buffer == NULL) {
                fputs("Memory error", stderr);
                exit(2);
        }
        remain = fread(buffer, 1, lSize, fp);
        if(remain != lSize) {
                fputs("Reading error", stderr);
                exit(3);
        }
        fread(buffer, sizeof(buffer), 1, fp);

        /*
         * 1st iteration :
         * alloc all DMA spaces in advance
         */
        LIST_INIT(&DMA_blks);
        log = (void*)(buffer + offset);
        for(; log->op_type != OPTYPE_NONE; offset+=size){

                /* if operation is 'DMA_read' or 'DMA_write' */
                if((log->op_type == OPTYPE_DMA) && (log->is_alloc == 0) && (log->is_free == 0)){
                        dma_exists = 1;
                        int blk_exists = 0;
                        int base = (log->addr) & (~0xfff);

                        /* check if target address of the operation is already allocated */
                        DMA_blk *blk;
                        LIST_FOREACH(blk, &DMA_blks, ptr){
                                if(blk->base == base){
                                        blk_exists = 1;
                                        break;
                                }
                        }

                        /* if target address of the operation is not allocated */
                        if(!blk_exists){
                                DMA_blk *new_blk = create_DMA_blk(base, log->vendor, log->device);
                                LIST_INSERT_HEAD(&DMA_blks, new_blk, ptr);

                                /* get `struct pci_dev` using vendor & device id */
                                printf("pdev = pci_get_device(0x%x, 0x%x, NULL);\n", log->vendor, log->device);

                                /*
                                 * - allocate one page(4KB) for that device
                                 * - get virtual address(cpu_addr) from return value
                                 * - get bus address(dma_handle) by pointer argument
                                 */
                                printf("cpu_addr = dma_alloc_coherent(&(pdev->dev),0x1000, &dma_handle, GFP_KERNEL);\n");

                                /* record info. at dynamic kernel list */
                                printf("add_DMA_blk(cpu_addr, 0x%x, dma_handle);\n",base);
                                printf("\n");
                        }
                }
                log = (void*)(buffer + offset);
        }

        /*
         * 2nd iteration :
         * parse operations
         */
        offset=0;
        log = (void*)(buffer + offset);
        offset += size;
        for(; log->op_type != OPTYPE_NONE; offset+=size) {

                /*
                 * load BAR addresses of pci device
                 *    if target device is not set (vid == -1)
                 *    or if target device has changed (vid != log->vendor)
                 */
                if((log->is_pci > 0) && (log->op_type != OPTYPE_NEXT)){
                        if((vid == -1) || (vid != log->vendor)){
                                vid = log->vendor;
                                did = log->device;
                                printf("dev = load_bar(0x%x, 0x%x);\n",vid,did);
                                printf("memcpy(bar_addr, dev->cfg.BAR, sizeof(bar_addr));\n");
                        }
                }

                /*
                 * parse operation
                 */
                if((log->op_type == OPTYPE_PIO) || (log->op_type == OPTYPE_MMIO) || (log->op_type == OPTYPE_DMA))
                        printf_io(log);
                else if(log->op_type == OPTYPE_NEXT)
                        printf("\n");

                log = (void*)(buffer + offset);
        }
        fclose(fp);

        /*
         * if allocated any memory dynamically for DMA,
         * free all of them
         */
        if(dma_exists){
                DMA_blk *blk;
                LIST_FOREACH(blk, &DMA_blks, ptr){
                        printf("pdev = pci_get_device(0x%x, 0x%x,NULL);\n", blk->vendor, blk->device);
                        printf("blk = get_DMA_blk(0x%x);\n", blk->base);
                        printf("dma_free_coherent(&(pdev->dev), 0x1000, blk->cpu_addr, blk->dma_handle);\n");
                }
        }
        return 0;
}

/*
 * Functions
 */

/*
 * create list for DMA allocation info
 */
DMA_blk *create_DMA_blk(uint32_t base, uint32_t vendor, uint32_t device)
{
        DMA_blk *blk;
        if((blk = (DMA_blk *)malloc(sizeof(DMA_blk))) != NULL)
        {
                blk->base = base;
                blk->vendor = vendor;
                blk->device = device;
        }
        return blk;
}

/*
 * print width of I/O operation
 */
void printf_width(int op_size)
{
        if(op_size == 0)
                printf("b");
        else if (op_size == 1)
                printf("w");
        else if (op_size == 2)
                printf("l");
}

/*
 * print I/O operation
 */
void printf_io(struct op_log *log)
{
        static uint32_t last_log_addr = -1; /* memoization for dynamic DMA address resolving */

        /*
         * condition flow layout
         *
         * I. MMIO non-pci => skip
         *
         * II-A. DMA
         * II-B. none-DMA
         *    II-B-1. dynamic target address
         *    II-B-2. constant target address
         */

        if((log->op_type == OPTYPE_MMIO) && (log->is_pci == 0))
                /*
                 * I. MMIO non-pci
                 * why this case even exists..?
                 * just followed ir0nc0w's way : ignore it.
                 */
                return;

        if((log->op_type == OPTYPE_DMA) && (log->is_alloc == 0) && (log->is_free == 0)){
                /* II-A. DMA */

                /* if DMA zone where target address is located has changed */
                if((last_log_addr == -1) || (last_log_addr != ((log->addr) & (~0xFFF)))){
                        printf("blk = get_DMA_blk(0x%x);\n",(log->addr) & (~0xFFF));
                        last_log_addr = (log->addr) & (~0xFFF);
                }

                if(log->op_rw == OPRW_READ){
                        printf("read"); /* operation */
                        printf_width(log->op_size); /* width */
                        printf("(blk->cpu_addr);\n"); /* operand */
                } else{
                        printf("write"); /* operation */
                        printf_width(log->op_size); /* width */
                        printf("(0x%x, blk->cpu_addr);\n", log->val); /* operand */
                }
        } else{
                /* II-B. none-DMA */

                if((log->val>=DMASTART) && (log->val<=DMASTOP) && (((log->val) & 0xFFF) == 0x000)){
                        /*
                         * II-B-1. dynamic target address
                         * if target addr is DMASTART <= addr <= DMASTOP
                         * and has all zeros at lsb 3 bits,
                         * maybe it is telling the device info about base address of DMA zone
                         */
                        int is_dma_handle = 0; /* 1: it really is info about DMA zone..! 0: nah */

                        /* search DMA block */
                        DMA_blk *blk;
                        LIST_FOREACH(blk, &DMA_blks, ptr){
                                if(blk->base == log->val){
                                        is_dma_handle = 1;
                                        printf("blk = get_DMA_blk(0x%x);\n", log->val);
                                }
                        }

                        /* operation */
                        if(log->op_type == OPTYPE_PIO)
                                printf("out");
                        if(log->op_type == OPTYPE_MMIO)
                                printf("write");

                        /* width */
                        printf_width(log->op_size);

                        /* operand1 */
                        if(is_dma_handle)
                                printf("(blk->dma_handle,");
                        else
                                printf("(0x%x,",log->val);

                        /* operand2 */
                        if(log->is_pci){
                                if(log->op_type == OPTYPE_PIO)
                                        printf("(resource_size_t)");
                                else
                                        printf("(void __iomem*)");

                                printf("bar_addr[%d]+0x%x);\n", log->bar_idx, log->addr);
                        }
                        else
                                printf("0x%x);\n", log->addr);

                } else{
                        /*
                         * II-B-2. constant target address
                         */

                        /* operation */
                        if((log->op_type == OPTYPE_PIO) && (log->op_rw == OPRW_READ))
                                printf("in");
                        if((log->op_type == OPTYPE_PIO) && (log->op_rw == OPRW_WRITE))
                                printf("out");
                        if((log->op_type == OPTYPE_MMIO) && (log->op_rw == OPRW_READ))
                                printf("read");
                        if((log->op_type == OPTYPE_MMIO) && (log->op_rw == OPRW_WRITE))
                                printf("write");

                        /* width */
                        printf_width(log->op_size);

                        /* operand */
                        if(log->is_pci){
                                if((log->op_type == OPTYPE_PIO) && (log->op_rw == OPRW_READ))
                                        printf("((resource_size_t)bar_addr[%d]+0x%x);\n", log->bar_idx, log->addr);
                                if((log->op_type == OPTYPE_PIO) && (log->op_rw == OPRW_WRITE))
                                        printf("(0x%x,(resource_size_t)bar_addr[%d]+0x%x);\n", log->val, log->bar_idx, log->addr);
                                if((log->op_type == OPTYPE_MMIO) && (log->op_rw == OPRW_READ))
                                        printf("((void __iomem*)bar_addr[%d]+0x%x);\n", log->bar_idx, log->addr);
                                if((log->op_type == OPTYPE_MMIO) && (log->op_rw == OPRW_WRITE))
                                        printf("(0x%x,(void __iomem*)bar_addr[%d]+0x%x);\n", log->val, log->bar_idx, log->addr);
                        } else{
                                if(log->op_rw == OPRW_READ)
                                        printf("(0x%x);\n", log->addr);
                                if(log->op_rw == OPRW_WRITE)
                                        printf("(0x%x,0x%x);\n", log->val, log->addr);
                        }
                }
        }
}
