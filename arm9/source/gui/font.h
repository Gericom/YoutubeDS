#ifndef __FONT_H__
#define __FONT_H__

#define NTFT_SIGNATURE_HEADER			MKTAG('N', 'T', 'F', 'T')
#define NTFT_SIGNATURE_CHARACTER_INFO	MKTAG('C', 'I', 'N', 'F')
#define NTFT_SIGNATURE_GLYPH_DATA		MKTAG('G', 'L', 'P', 'D')

typedef struct
{
	uint32_t signature;
	uint32_t fileSize;
	uint32_t charInfoOffset;
	uint32_t glyphDataOffset;
} NTFT_HEADER;

typedef struct
{
	uint32_t glyphDataOffset;
	int32_t characterBeginOffset;
	uint32_t characterWidth;
	int32_t characterEndOffset;
} NTFT_CHARACTER_INFO_CHARACTER;

typedef struct
{
	uint32_t signature;
	uint32_t blockSize;
	uint32_t characterHeight;
	NTFT_CHARACTER_INFO_CHARACTER characters[256];
} NTFT_CHARACTER_INFO;

typedef struct
{
	uint32_t signature;
	uint32_t blockSize;
	uint32_t glyphDataSize;
	uint8_t glyphData[0];
} NTFT_GLYPH_DATA;

typedef struct
{
	NTFT_HEADER* header;
	NTFT_CHARACTER_INFO* characterInfo;
	NTFT_GLYPH_DATA* glyphData;
} NTFT_FONT;

NTFT_FONT* Font_Load(const char* path);
void Font_Unload(NTFT_FONT* font);
void Font_GetStringSize(NTFT_FONT* font, const char* text, int* width, int* height);
void Font_CreateStringData(NTFT_FONT* font, const char* text, uint8_t* dst, int stride);

#endif