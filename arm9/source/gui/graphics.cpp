#include "graphics.h"
#include <stdio.h>

#include "tonccpy.h"

#include "font_nftr.h"
#include "fileBrowseBg.h"

u8 *fontTiles;
const u8 *fontWidths;
std::vector<u16> fontMap;
u16 tileSize, tileWidth, tileHeight;

void loadPalettes(void) {
	u16 palette[] = {0x0000, 0xFFFF, 0xD6B5, 0xB9CE, // White
					 0x0000, 0xDEF7, 0xC631, 0xA94A, // Black
					 0x0000, (u16)(palettes[PersonalData->theme][1] & 0xFFFF), (u16)(palettes[PersonalData->theme][1] & 0xDEF7), (u16)(palettes[PersonalData->theme][1] & 0xCE73), // Light colored
					 0x0000, (u16)(palettes[PersonalData->theme][1] & 0xD6B5), (u16)(palettes[PersonalData->theme][1] & 0xCA52), (u16)(palettes[PersonalData->theme][1] & 0xC210)}; // Darker colored
	tonccpy(BG_PALETTE_SUB, &palette, sizeof(palette));
	tonccpy(BG_PALETTE_SUB+0x10, &fileBrowseBgPal, fileBrowseBgPalLen/2);
	tonccpy(BG_PALETTE_SUB+0x13, &palettes[PersonalData->theme], sizeof(palettes[PersonalData->theme]));
}

void loadFont(void) {
	// consoleDemoInit();
	// Get glyph start
	u8 glyphOfs = font_nftr[0x14] + 0x14;

	// Load glyph info
	u32 chunkSize = *(u32*)(font_nftr+glyphOfs);
	tileWidth = font_nftr[glyphOfs+4];
	tileHeight = font_nftr[glyphOfs+5];
	tileSize = *(u16*)(font_nftr+glyphOfs+6);
	// printf("%d\n%d\n%d\n", tileWidth, tileHeight, tileSize);
	// while(1)swiWaitForVBlank();

	// Load character glyphs
	int tileAmount = ((chunkSize-0x10)/tileSize);
	fontTiles = (u8*)font_nftr+glyphOfs+0xC;

	// Fix top rows
	for(int i=0;i<tileAmount;i++) {
		fontTiles[i*tileSize] = 0;
		fontTiles[i*tileSize+1] = 0;
		fontTiles[i*tileSize+2] = 0;
	}

	// Load character widths
	u32 locHDWC = *(u32*)(font_nftr+0x24);
	chunkSize = *(u32*)(font_nftr+locHDWC-4);
	fontWidths = font_nftr+locHDWC+8;

	// Load character maps
	fontMap = std::vector<u16>(tileAmount);
	u32 locPAMC = *(u32*)(font_nftr+0x28);

	while(locPAMC < font_nftr_size) {
		const u8* font = font_nftr+locPAMC;

		u16 firstChar = *(u16*)font;
		font += 2;
		u16 lastChar = *(u16*)font;
		font += 2;
		u32 mapType = *(u32*)font;
		font += 4;
		locPAMC = *(u32*)font;
		font += 4;

		switch(mapType) {
			case 0: {
				u16 firstTile = *(u16*)font;
				for(unsigned i=firstChar;i<=lastChar;i++) {
					fontMap[firstTile+(i-firstChar)] = i;
				}
				break;
			} case 1: {
				for(int i=firstChar;i<=lastChar;i++) {
					u16 tile = *(u16*)font;
					font += 2;
					fontMap[tile] = i;
				}
				break;
			} case 2: {
				u16 groupAmount = *(u16*)font;
				font += 2;
				for(int i=0;i<groupAmount;i++) {
					u16 charNo = *(u16*)font;
					font += 2;
					u16 tileNo = *(u16*)font;
					font += 2;
					fontMap[tileNo] = charNo;
				}
				break;
			}
		}
	}
}

void drawImage(int x, int y, int w, int h, const unsigned char *imageBuffer, int startPal) {
	u8* dst = (u8*)BG_GFX_SUB;
	for(int i=0;i<h;i++) {
		for(int j=0;j<w;j++) {
			if(BG_PALETTE_SUB[imageBuffer[(i*w)+j]+startPal] != 0) { // Do not render black
				dst[(y+i)*256+j+x] = imageBuffer[(i*w)+j]+startPal;
			}
		}
	}
}

void drawImageScaled(int x, int y, int w, int h, double scaleX, double scaleY, const unsigned char *imageBuffer, int startPal) {
	if(scaleX == 1 && scaleY == 1)	drawImage(x, y, w, h, imageBuffer, startPal);
	else {
		u8* dst = (u8*)BG_GFX_SUB;
		for(int i=0;i<(h*scaleY);i++) {
			for(int j=0;j<(w*scaleX);j++) {
				if(BG_PALETTE_SUB[imageBuffer[(int)((((int)(i/scaleY))*w)+(j/scaleX))]+startPal] != 0) { // Do not render black
					dst[(y+i)*256+x+j] = imageBuffer[(int)((((int)(i/scaleY))*w)+(j/scaleX))]+startPal;
				}
			}
		}
	}
}

void drawImageSection(int x, int y, int w, int h, const unsigned char *imageBuffer, int imageWidth, int xOffset, int yOffset, int startPal) {
	u8* dst = (u8*)BG_GFX_SUB;
	for(int i=0;i<h;i++) {
		for(int j=0;j<w;j++) {
			if(BG_PALETTE_SUB[imageBuffer[((i+yOffset)*imageWidth)+j+xOffset]+startPal] != 0) { // Do not render black
				dst[((y+i)*256)+j+x] = imageBuffer[((i+yOffset)*imageWidth)+j+xOffset]+startPal;
			}
		}
	}
}

void drawRectangle(int x, int y, int w, int h, u8 color) {
	u8* dst = (u8*)BG_GFX_SUB;
	for(int i=0;i<h;i++) {
		for(int j=0;j<w;j++) {
			dst[(y+i)*256+j+x] = color;
		}
	}
}

std::u16string UTF8toUTF16(const std::string &src) {
	std::u16string ret;
	for(size_t i = 0; i < src.size(); i++) {
		u16 codepoint = 0xFFFD;
		int iMod = 0;
		if(src[i] & 0x80 && src[i] & 0x40 && src[i] & 0x20 && !(src[i] & 0x10) && i + 2 < src.size()) {
			codepoint = src[i] & 0x0F;
			codepoint = codepoint << 6 | (src[i + 1] & 0x3F);
			codepoint = codepoint << 6 | (src[i + 2] & 0x3F);
			iMod = 2;
		} else if(src[i] & 0x80 && src[i] & 0x40 && !(src[i] & 0x20) && i + 1 < src.size()) {
			codepoint = src[i] & 0x1F;
			codepoint = codepoint << 6 | (src[i + 1] & 0x3F);
			iMod = 1;
		} else if(!(src[i] & 0x80)) {
			codepoint = src[i];
		}

		ret.push_back((char16_t)codepoint);
		i += iMod;
	}
	return ret;
}

int getCharIndex(char16_t c) {
	int spriteIndex = 0;
	int left = 0;
	int mid = 0;
	int right = fontMap.size();

	while(left <= right) {
		mid = left + ((right - left) / 2);
		if(fontMap[mid] == c) {
			spriteIndex = mid;
			break;
		}

		if(fontMap[mid] < c) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}
	return spriteIndex;
}

void printTextCenteredMaxW(std::string text, double w, double scaleY, int palette, int xOffset, int yPos) { printText(UTF8toUTF16(text), std::min(1.0, w/getTextWidth(text)), scaleY, palette, ((256-getTextWidthMaxW(text, w))/2)+xOffset, yPos); }
void printTextCenteredMaxW(std::u16string text, double w, double scaleY, int palette, int xOffset, int yPos) { printText(text, std::min(1.0, w/getTextWidth(text)), scaleY, palette, ((256-getTextWidthMaxW(text, w))/2)+xOffset, yPos); }
void printTextCentered(std::string text, double scaleX, double scaleY, int palette, int xOffset, int yPos) { printText(UTF8toUTF16(text), scaleX, scaleY, palette, ((256-getTextWidth(text))/2)+xOffset, yPos); }
void printTextCentered(std::u16string text, double scaleX, double scaleY, int palette, int xOffset, int yPos) { printText(text, scaleX, scaleY, palette, ((256-getTextWidth(text))/2)+xOffset, yPos); }
void printTextMaxW(std::string text, double w, double scaleY, int palette, int xPos, int yPos) { printText(UTF8toUTF16(text), std::min(1.0, w/getTextWidth(text)), scaleY, palette, xPos, yPos); }
void printTextMaxW(std::u16string text, double w,  double scaleY, int palette, int xPos, int yPos) { printText(text, std::min(1.0, w/getTextWidth(text)), scaleY, palette, xPos, yPos); }
void printText(std::string text, double scaleX, double scaleY, int palette, int xPos, int yPos) { printText(UTF8toUTF16(text), scaleX, scaleY, palette, xPos, yPos); }
void printText(std::u16string text, double scaleX, double scaleY, int palette, int xPos, int yPos) {
	int x=xPos;
	for(unsigned c=0;c<text.size();c++) {
		if(text[c] == '\n') {
			x = xPos;
			yPos += tileHeight;
			continue;
		}

		int t = getCharIndex(text[c]);
		unsigned char image[tileSize * 4];
		for(int i=0;i<tileSize;i++) {
			image[(i*4)]   = (palette*4 + (fontTiles[i+(t*tileSize)]>>6 & 3));
			image[(i*4)+1] = (palette*4 + (fontTiles[i+(t*tileSize)]>>4 & 3));
			image[(i*4)+2] = (palette*4 + (fontTiles[i+(t*tileSize)]>>2 & 3));
			image[(i*4)+3] = (palette*4 + (fontTiles[i+(t*tileSize)]    & 3));
		}

		x += fontWidths[t*3];
		if(x > 256) {
			x = xPos+fontWidths[t*3];
			yPos += tileHeight;
		}
		drawImageScaled(x, yPos, tileWidth, tileHeight, scaleX, scaleY, image);
		x += fontWidths[(t*3)+1]*scaleX;
	}
}

int getTextWidthMaxW(std::string text, int w) { return std::min(w, getTextWidth(UTF8toUTF16(text))); }
int getTextWidthMaxW(std::u16string text, int w) { return std::min(w, getTextWidth(text)); }
int getTextWidth(std::string text) { return getTextWidth(UTF8toUTF16(text)); }
int getTextWidth(std::u16string text) {
	int textWidth = 0;
	for(unsigned c=0;c<text.size();c++) {
		textWidth += fontWidths[(getCharIndex(text[c])*3)+2];
	}
	return textWidth;
}
