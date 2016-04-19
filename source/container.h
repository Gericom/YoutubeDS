#ifndef __CONTAINER_H__
#define __CONTAINER_H__

typedef struct
{
	uint32_t size;
	uint32_t signature;//ftyp
	uint32_t major_brand_code;
	uint32_t version;
	uint32_t compatible_brand_codes[0];
} ftype_t;

#endif