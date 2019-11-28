#include <nds.h>
#include "util.h"

char* Util_CopyString(const char* string)
{
	int len = strlen(string);
	char* result = (char*)malloc(len + 1);
	memcpy(result, string, len + 1);
	return result;
}

void Util_ConvertToObj(uint8_t* src, int width, int height, int stride, uint16_t* dst)
{
	for (int y = 0; y < height / 8; y++)
	{
		for (int x = 0; x < width / 8; x++)
		{
			for (int y2 = 0; y2 < 8; y2++)
			{
				//write in 32 bit units for vram compatibility
				*((uint32_t*)dst) =
					(1 + (src[0] * 14 + 128) / 256) |
					((1 + (src[1] * 14 + 128) / 256) << 4) |
					((1 + (src[2] * 14 + 128) / 256) << 8) |
					((1 + (src[3] * 14 + 128) / 256) << 12) |
					((1 + (src[4] * 14 + 128) / 256) << 16) |
					((1 + (src[5] * 14 + 128) / 256) << 20) |
					((1 + (src[6] * 14 + 128) / 256) << 24) |
					((1 + (src[7] * 14 + 128) / 256) << 28);
				dst += 2;
				src += stride;
			}
			src -= 8 * stride;
			src += 8;
		}
		src -= width;
		src += 8 * stride;
	}
}

void Util_SetupStrideFixAffine(BG23AffineInfo* pAffineInfo, int srcStride, int dstStride, int xOffset, int yOffset, int xScale, int yScale, int xFrac)
{
	int startoffset = -yOffset * srcStride;
	int x = startoffset & 0xFF;
	int y = startoffset >> 8;
	int realy = 128;//0;
	//setup affine shit
	for(int i = 0; i < 192; i++)
	{
		pAffineInfo[i].BG2PA = xScale;
		pAffineInfo[i].BG2PB = 0;
		pAffineInfo[i].BG2PC = 0;
		pAffineInfo[i].BG2PD = 256;
		pAffineInfo[i].BG3PA = xScale;
		pAffineInfo[i].BG3PB = 0;
		pAffineInfo[i].BG3PC = 0;
		pAffineInfo[i].BG3PD = 256;
		pAffineInfo[i].BG2X = x * 256 - xOffset * 256 + 128 + xFrac;
		pAffineInfo[i].BG2Y = y * 256;
		if(dstStride - x < srcStride)
		{
			pAffineInfo[i].BG3X = (x - dstStride) * 256 - xOffset * 256 + 128 + xFrac;
			pAffineInfo[i].BG3Y = (y + 1) * 256;
		}
		else
		{
			pAffineInfo[i].BG3X = x * 256 - xOffset * 256 + 128 + xFrac;
			pAffineInfo[i].BG3Y = y * 256;
		}
		//some lines need to be repeated
		if((realy & 0xFF) + yScale >= dstStride)
		{
			x += srcStride;
			if(x >= dstStride)
			{
				x -= dstStride;
				y++;
			}
		}
		realy += yScale;
	}
	DC_FlushRange(pAffineInfo, sizeof(BG23AffineInfo) * 192);
}