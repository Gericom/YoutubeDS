#ifndef __UTIL_H__
#define __UTIL_H__

#define MKTAG(a0,a1,a2,a3) ((uint32_t)((a3) | ((a2) << 8) | ((a1) << 16) | ((a0) << 24)))

static inline uint32_t SWAP_CONSTANT_32(uint32_t a)
{
	return ((uint32_t)((((a) >> 24) & 0x00FF) | 
	          (((a) >>  8) & 0xFF00) | 
	          (((a) & 0xFF00) <<  8) | 
	          (((a) & 0x00FF) << 24) ));
}

#define READ_SAFE_UINT32_BE(a)	((((uint8_t*)(a))[0] << 24) | (((uint8_t*)(a))[1] << 16) | (((uint8_t*)(a))[2] << 8) | (((uint8_t*)(a))[3] << 0))

char* Util_CopyString(const char* string);
void Util_ConvertToObj(uint8_t* src, int width, int height, int stride, uint16_t* dst);

typedef struct
{
	uint16_t BG2PA;
	uint16_t BG2PB;
	uint16_t BG2PC;
	uint16_t BG2PD;
	uint32_t BG2X;
	uint32_t BG2Y;
	uint16_t BG3PA;
	uint16_t BG3PB;
	uint16_t BG3PC;
	uint16_t BG3PD;
	uint32_t BG3X;
	uint32_t BG3Y;
} BG23AffineInfo;

void Util_SetupStrideFixAffine(BG23AffineInfo* pAffineInfo, int srcStride, int dstStride, int xOffset, int yOffset, int xScale, int yScale);

#endif