#include <nds.h>
#include "util.h"
#include "container.h"

ftype_t* CNTNR_ReadFType(uint8_t** ppData)
{
	uint8_t* ptr = *ppData;
	uint32_t size = SWAP_CONSTANT_32(*((uint32_t*)ptr));
	ptr += 4;
	ftype_t* ftype = (ftype_t*)malloc(size);
	ftype->size = size;
	ftype->signature = SWAP_CONSTANT_32(*((uint32_t*)ptr));
	ptr += 4;
	ftype->major_brand_code = SWAP_CONSTANT_32(*((uint32_t*)ptr));
	ptr += 4;
	ftype->version = SWAP_CONSTANT_32(*((uint32_t*)ptr));
	ptr += 4;
	int nrcompbrandcodes = (size - 0x10) / 4;
	for(int i = 0; i < nrcompbrandcodes; i++)
	{
		ftype->compatible_brand_codes[i] = SWAP_CONSTANT_32(*((uint32_t*)ptr));
		ptr += 4;
	}
	*ppData = ptr;
	return ftype;
}