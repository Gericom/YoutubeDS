#ifndef __MPEG4_H__
#define __MPEG4_H__

typedef struct PACKED
{
	uint8_t* pData;
	uint16_t width;
	uint16_t height;
	uint8_t* pDstY;
	uint8_t* pDstUV;
	uint8_t* pPrevY;
	uint8_t* pPrevUV;
	uint16_t* pIntraDCTVLCTable;
	uint16_t* pInterDCTVLCTable;
	int16_t* pdc_coef_cache_y;
	int16_t* pdc_coef_cache_uv;
	int16_t* pvector_cache;
	uint8_t vop_time_increment_bits;
	uint8_t vop_fcode_forward;
} mpeg4_dec_struct;

extern "C" void mpeg4_VideoObjectPlane(mpeg4_dec_struct* pInfo);

#endif