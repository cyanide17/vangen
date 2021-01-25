#include <stdio.h> 
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

struct op_log {
  uint16_t op_type:3;    /* (0-7) 0x0: NO 0x1: PIO 0x2: MMIO 0x4: INT(start) 0x5: INT(end) 0x6: next pattern */
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
#define PAGE_SIZE 1UL << 12
#define DATA_SIZE PAGE_SIZE<<8


int main(int argc, char** argv)
{
	int size = sizeof(struct op_log);
	struct op_log *log;
	unsigned int offset = 0;
	int op_size;
	long lSize;
	char *buffer;
	size_t remain;

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

	log = (void*)(buffer + offset);
	offset += size;

        if(log->is_pci>0){
            printf("vendor = 0x%x;\n",log->vendor);
            printf("device = 0x%x;\n",log->device);
        }
        printf("@\n");

	for(; log->op_type != 0x0; offset+=size) {
		if(log->op_size == 0)
			op_size = 1;
		else if(log->op_size == 1)
			op_size = 2;
		else if(log->op_size == 2)
			op_size = 4;

                switch(log->op_type) {
                    case 0x1:
                        if(log->op_rw == 0x0) {
                            if(op_size == 1)
                                printf("inb");
                            else if (op_size == 2)
                                printf("inw");
                            else if (op_size == 4)
                                printf("inl");
                            if(log->is_pci > 0) {
                                printf("(bar_addr[%d]+0x%x);\n", log->bar_idx, log->addr);
                            } else {
                                printf("(0x%x);\n", log->addr);
                            }
                        } else {
                            if(op_size == 1)
                                printf("outb");
                            else if (op_size == 2)
                                printf("outw");
                            else if (op_size == 4)
                                printf("outl");
                            if(log->is_pci > 0) {
                                printf("(0x%x,bar_addr[%d]+0x%x);\n", log->val, log->bar_idx, log->addr);
                            } else {
                                printf("(0x%x,0x%x);\n",log->val, log->addr);
                            }
                        }	
                        break;
                    case 0x2:
                        /*
                        if(log->op_rw == 0x0) {
                            if(op_size == 1)
                                printf("readb");
                            else if (op_size == 2)
                                printf("readw");
                            else if (op_size == 4)
                                printf("readl");
                            if(log->is_pci > 0) {
                                printf("(bar_addr[%d]+0x%x);\n", log->bar_idx, log->addr);
                            } else {
                                //printf("(wrong)\n");
                            }
                        } else {
                            if(op_size == 1)
                                printf("writeb");
                            else if (op_size == 2)
                                printf("writew");
                            else if (op_size == 4)
                                printf("writel");
                            if(log->is_pci > 0) {
                                printf("(0x%x, bar_addr[%d]+0x%x);\n", log->val, log->bar_idx, log->addr);
                            } else {
                                //printf("(wrong)\n");
                            }
                        }
                        */
                        if(log->is_pci>0){
                            if(log->op_rw == 0x0) {
                                if(op_size == 1)
                                    printf("readb");
                                else if (op_size == 2)
                                    printf("readw");
                                else if (op_size == 4)
                                    printf("readl");
                                printf("(bar_addr[%d]+0x%x);\n", log->bar_idx, log->addr);
                            } else {
                                if(op_size == 1)
                                    printf("writeb");
                                else if (op_size == 2)
                                    printf("writew");
                                else if (op_size == 4)
                                    printf("writel");
                                printf("(0x%x, bar_addr[%d]+0x%x);\n", log->val, log->bar_idx, log->addr);
                            }

                        }
                        break;
                    case 0x6:{
                                 printf("\n");
                             }
                             break;
                    default:
                             break;

                }
                log = (void*)(buffer + offset);
        }

        fclose(fp);

        return 0;
}
