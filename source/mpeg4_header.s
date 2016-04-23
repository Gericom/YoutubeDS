@We'll use stride 256 to make it more suiteable for displaying on a ds
STRIDE = 256
STRIDE_SHIFT = 8

@r0 has the following structure:
@uint8_t* pData
@uint16_t width
@uint16_t height
@uint8_t* pDstY
@uint8_t* pDstUV
@uint8_t* pPrevY
@uint8_t* pPrevUV
@uint16_t* pIntraDCTVLCTable
@uint16_t* pInterDCTVLCTable
@int16_t* pdc_coef_cache_y
@int16_t* pdc_coef_cache_uv
@int16_t* pvector_cache
@uint8_t vop_time_increment_bits
@uint8_t vop_fcode_forward

mpeg4_dec_struct__pData = 0
mpeg4_dec_struct__width = (mpeg4_dec_struct__pData + 4)
mpeg4_dec_struct__height = (mpeg4_dec_struct__width + 2)
mpeg4_dec_struct__pDstY = (mpeg4_dec_struct__height + 2)
mpeg4_dec_struct__pDstUV = (mpeg4_dec_struct__pDstY + 4)
mpeg4_dec_struct__pPrevY = (mpeg4_dec_struct__pDstUV + 4)
mpeg4_dec_struct__pPrevUV = (mpeg4_dec_struct__pPrevY + 4)
mpeg4_dec_struct__pIntraDCTVLCTable = (mpeg4_dec_struct__pPrevUV + 4)
mpeg4_dec_struct__pInterDCTVLCTable = (mpeg4_dec_struct__pIntraDCTVLCTable + 4)
mpeg4_dec_struct__pdc_coef_cache_y = (mpeg4_dec_struct__pInterDCTVLCTable + 4)
mpeg4_dec_struct__pdc_coef_cache_uv = (mpeg4_dec_struct__pdc_coef_cache_y + 4)
mpeg4_dec_struct__pvector_cache = (mpeg4_dec_struct__pdc_coef_cache_uv + 4)
mpeg4_dec_struct__vop_time_increment_bits = (mpeg4_dec_struct__pvector_cache + 4)
mpeg4_dec_struct__vop_fcode_forward = (mpeg4_dec_struct__vop_time_increment_bits + 1)
