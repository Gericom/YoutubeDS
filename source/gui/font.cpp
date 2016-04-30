#include <nds.h>
#include <stdio.h>
#include "font.h"

NTFT_FONT* Font_Load(const char* path)
{
	FILE* file = fopen(path, "rb");
	fseek(file, 4, SEEK_SET);
	uint32_t length;
	fread(&length, 4, 1, file);
	NTFT_FONT* result = (NTFT_FONT*)malloc(sizeof(NTFT_FONT));
	NTFT_HEADER* font = (NTFT_HEADER*)malloc(length);
	fseek(file, 0, SEEK_SET);
	fread(font, 1, length, file);
	fclose(file);
	result->header = font;
	result->characterInfo = (NTFT_CHARACTER_INFO*)((uint8_t*)font + font->charInfoOffset);
	result->glyphData = (NTFT_GLYPH_DATA*)((uint8_t*)font + font->glyphDataOffset);
	return result;
}

void Font_Unload(NTFT_FONT* font)
{
	if(font == NULL || font->header == NULL) return;
	free(font->header);
	free(font);
}

void Font_GetStringSize(NTFT_FONT* font, const char* text, int* width, int* height)
{
	*width = 0;
	int tmpwidth = 0;
	if(height != NULL) *height = font->characterInfo->characterHeight;
	char c = *text++;
	while(c != 0)
	{
		int end = 0;
		if(c == '\n')
		{
			if(*width < tmpwidth) *width = tmpwidth;
			tmpwidth = 0;
			if(height != NULL) *height += font->characterInfo->characterHeight + 1;
		}
		else
		{
			if((tmpwidth + font->characterInfo->characters[c].characterBeginOffset) >= 0)
				tmpwidth += font->characterInfo->characters[c].characterBeginOffset;
			tmpwidth += font->characterInfo->characters[c].characterWidth;
			end = font->characterInfo->characters[c].characterEndOffset;
		}
		c = *text++;
		if(c != 0) tmpwidth += end;
	}
	if(*width < tmpwidth) *width = tmpwidth;
}

void Font_CreateStringData(NTFT_FONT* font, const char* text, uint8_t* dst, int stride)
{
	int width;
	int height;
	Font_GetStringSize(font, text, &width, &height);
	int xpos = 0;
	int ypos = 0;
	char c = *text++;
	while(c != 0)
	{
		if(c == '\n') 
		{
			xpos = 0;
			ypos += font->characterInfo->characterHeight + 1;
		}
		else
		{
			if((xpos + font->characterInfo->characters[c].characterBeginOffset) >= 0)
				xpos += font->characterInfo->characters[c].characterBeginOffset;
			uint8_t* glyph = (uint8_t*)&font->glyphData->glyphData[font->characterInfo->characters[c].glyphDataOffset];
			uint8_t* dst_ptr = dst + ypos * stride + xpos;
			for(int y = 0; y < font->characterInfo->characterHeight; y++)
			{
				for(int x = 0; x < font->characterInfo->characters[c].characterWidth; x++)
				{
					uint8_t data = *glyph++;
					int oldval = *dst_ptr;
					int newval = oldval + data;
					if(newval > 255) newval = 255;
					*dst_ptr++ = newval;
				}
				dst_ptr -= font->characterInfo->characters[c].characterWidth;
				dst_ptr += stride;
			}
			xpos += font->characterInfo->characters[c].characterWidth;
			xpos += font->characterInfo->characters[c].characterEndOffset;
		}
		c = *text++;
	}
}