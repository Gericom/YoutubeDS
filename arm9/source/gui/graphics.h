#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <nds.h>
#include <string>
#include <vector>

static constexpr u16 palettes[16][2] {
	{0x41A9, 0x49EB},	//  0: gray
	{0x00B5, 0x00F7},	//  1: brown
	{0x1419, 0x103D},	//  2: red
	{0x757F, 0x75FF},	//  3: pink
	{0x11DF, 0x023F},	//  4: orange
	{0x029D, 0x02FD},	//  5: yellow
	{0x032E, 0x0371},	//  6: yellow-green
	{0x0301, 0x0363},	//  7: lively green
	{0x1660, 0x16C0},	//  8: green
	{0x32E0, 0x3744},	//  9: light green
	{0x7E23, 0x7AA5},	// 10: sky blue
	{0x5CA0, 0x7120},	// 11: light blue
	{0x3800, 0x5400},	// 12: blue
	{0x444B, 0x5C0F},	// 13: violet
	{0x5412, 0x6C17},	// 14: purple
	{0x443D, 0x507F},	// 15: fuschia
};

void loadPalettes(void);
void loadFont(void);

void drawImage(int x, int y, int w, int h, const unsigned char *imageBuffer, int startPal = 0);
void drawImageScaled(int x, int y, int w, int h, double scaleX, double scaleY, const unsigned char *imageBuffer, int startPal = 0);
void drawImageSection(int x, int y, int w, int h, const unsigned char *imageBuffer, int imageWidth, int xOffset, int yOffset, int startPal = 0);
void drawRectangle(int x, int y, int w, int h, u8 color);

std::u16string UTF8toUTF16(const std::string &src);
int getCharIndex(char16_t c);

void printTextCenteredMaxW(std::string text, double w, double scaleY, int palette, int xOffset, int yPos);
void printTextCenteredMaxW(std::u16string text, double w, double scaleY, int palette, int xOffset, int yPos);
void printTextCentered(std::string text, double scaleX, double scaleY, int palette, int xOffset, int yPos);
void printTextCentered(std::u16string text, double scaleX, double scaleY, int palette, int xOffset, int yPos);
void printTextMaxW(std::string text, double w, double scaleY, int palette, int xPos, int yPos);
void printTextMaxW(std::u16string text, double w,  double scaleY, int palette, int xPos, int yPos);
void printText(std::string text, double scaleX, double scaleY, int palette, int xPos, int yPos);
void printText(std::u16string text, double scaleX, double scaleY, int palette, int xPos, int yPos);

int getTextWidthMaxW(std::string text, int w);
int getTextWidthMaxW(std::u16string text, int w);
int getTextWidth(std::string text);
int getTextWidth(std::u16string text);

#endif
