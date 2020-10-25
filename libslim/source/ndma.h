
#ifndef __SLIM_NDMA_H__
#define __SLIM_NDMA_H__

#include <nds/ndstypes.h>

#define NDMA_SRC(n) (*(vu32*)(0x4004104+(n*0x1C)))
#define NDMA_DEST(n) (*(vu32*)(0x4004108+(n*0x1C)))
#define NDMA_SZ(n) (*(vu32*)(0x4004110+(n*0x1C)))
#define NDMA_CR(n) (*(vu32*)(0x400411C+(n*0x1C)))
#define NDMA_TIMING(n) (*(vu32*)(0x4004114+(n*0x1C)))
#define NDMA_BUSY	    BIT(31)


static inline 
bool ndmaBusy(uint8 ndmaSlot) {
	return (NDMA_CR(ndmaSlot) & NDMA_BUSY) == 0x80000000;
}

/*! \fn void ndmaCopyWordsAsynch(uint8 channel, const void* src, void* dest, uint32 size)
\brief copies from source to destination on one of the 4 available channels in half words.  
This function returns immediately after starting the transfer.
\param channel the dma channel to use (0 - 3).  
\param src the source to copy from
\param dest the destination to copy to
\param size the size in bytes of the data to copy.  Will be truncated to the nearest word (4 bytes)
*/
static inline
void ndmaCopyWordsAsynch(uint8_t ndmaSlot, const void* src, void* dest, uint32_t size) {
	NDMA_SRC(ndmaSlot) = (uint32)src;
	NDMA_DEST(ndmaSlot) = (uint32)dest;
	
	NDMA_SZ(ndmaSlot) = (size >> 2);	
	
    // no clue what this does
    NDMA_TIMING(ndmaSlot) = 0x1;
	NDMA_CR(ndmaSlot) = 0x90070000;
}

static inline
void ndmaCopyWords(uint8_t ndmaSlot, const void* src, void* dest, uint32_t size) {
	ndmaCopyWordsAsynch(ndmaSlot, src, dest, size);
	while (ndmaBusy(ndmaSlot));
}

#endif