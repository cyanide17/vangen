#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

typedef struct _DMA_block{
	uint32_t base;
	LIST_ENTRY(_DMA_block) ptr;
}DMA_blk;
DMA_blk *create_DMA_blk(uint32_t base);
LIST_HEAD(DMA_list,_DMA_block) DMA_blks;

int main(int argc, char** argv)
{
	DMA_blk *blk1 = create_DMA_blk(0x1234);
	DMA_blk *blk2 = create_DMA_blk(0x5678);
	DMA_blk *blk3 = create_DMA_blk(0xabcd);
	LIST_INIT(&DMA_blks);

	LIST_INSERT_HEAD(&DMA_blks,blk1,ptr);
	LIST_INSERT_HEAD(&DMA_blks,blk2,ptr);
	LIST_INSERT_HEAD(&DMA_blks,blk3,ptr);

	DMA_blk *blk;
	/*
	LIST_FOREACH(blk, &DMA_blks, ptr){
		printf("0x%x\n",blk->base);
		if(blk->base == 0x5678)
		{
			printf("found\n");
			break;
		}
	}
	**
	 */

	printf("\n");

}

DMA_blk *create_DMA_blk(uint32_t base)
{
	DMA_blk *blk;
	if((blk = (DMA_blk *)malloc(sizeof(DMA_blk))) != NULL)
	{
		blk->base = base;
	}


	return blk;
}
