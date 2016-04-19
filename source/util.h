#ifndef __UTIL_H__
#define __UTIL_H__

#define MKTAG(a0,a1,a2,a3) ((uint32_t)((a3) | ((a2) << 8) | ((a1) << 16) | ((a0) << 24)))

//#define SWAP_CONSTANT_32(a) \
//	((uint32_t)((((a) >> 24) & 0x00FF) | \
//	          (((a) >>  8) & 0xFF00) | \
//	          (((a) & 0xFF00) <<  8) | \
//	          (((a) & 0x00FF) << 24) ))

static inline uint32_t SWAP_CONSTANT_32(uint32_t a)
{
	return ((uint32_t)((((a) >> 24) & 0x00FF) | 
	          (((a) >>  8) & 0xFF00) | 
	          (((a) & 0xFF00) <<  8) | 
	          (((a) & 0x00FF) << 24) ));
}

//#define SWAP_CONSTANT_16(a) \
//	((uint16_t)((((a) >>  8) & 0x00FF) | \
//	          (((a) <<  8) & 0xFF00) )) 

#define READ_SAFE_UINT32_BE(a)	((((uint8_t*)(a))[0] << 24) | (((uint8_t*)(a))[1] << 16) | (((uint8_t*)(a))[2] << 8) | (((uint8_t*)(a))[3] << 0))

#endif