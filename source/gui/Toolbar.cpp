#include <nds.h>
#include <stdio.h>
#include "Toolbar.h"

#define OAM_BACK_ICON_IDX		0
#define OAM_CLEAR_ICON_IDX		(OAM_BACK_ICON_IDX + 1)
#define OAM_SEARCH_ICON_IDX		(OAM_CLEAR_ICON_IDX + 1)
#define OAM_MENU_ICON_IDX		(OAM_SEARCH_ICON_IDX + 1)
#define OAM_TEXT_CURSOR_IDX		(OAM_MENU_ICON_IDX + 1)
#define OAM_TITLE_TEXT_IDX		(OAM_TEXT_CURSOR_IDX + 1)
#define OAM_TOOLBAR_BG_IDX		(OAM_TITLE_TEXT_IDX + 5)

void Toolbar::Initialize()
{
	for(int i = 0; i < 512; i++)
	{
		SPRITE_GFX_SUB[i] = 0x1111;
	}
	FILE* file = fopen("/Menu/arrow-left.nbfc", "rb");
	uint8_t icon_tmp[128];
	fread(&icon_tmp[0], 1, 128, file);
	fclose(file);
	for(int i = 0; i < 128 / 2; i++)
	{
		SPRITE_GFX_SUB[i + 512] = icon_tmp[i*2]|(icon_tmp[i*2+1] << 8);
	}
	file = fopen("/Menu/magnify.nbfc", "rb");
	fread(&icon_tmp[0], 1, 128, file);
	fclose(file);
	for(int i = 0; i < 128 / 2; i++)
	{
		SPRITE_GFX_SUB[i + 512 + 64] = icon_tmp[i*2]|(icon_tmp[i*2+1] << 8);
	}
	file = fopen("/Menu/dots-vertical.nbfc", "rb");
	fread(&icon_tmp[0], 1, 64, file);
	fclose(file);
	for(int i = 0; i < 64 / 2; i++)
	{
		SPRITE_GFX_SUB[i + 512 + 64 + 64] = icon_tmp[i*2]|(icon_tmp[i*2+1] << 8);
	}
	file = fopen("/Menu/TextCursor.nbfc", "rb");
	fread(&icon_tmp[0], 1, 64, file);
	fclose(file);
	for(int i = 0; i < 64 / 2; i++)
	{
		SPRITE_GFX_SUB[i + 512 + 64 + 64 + 32] = icon_tmp[i*2]|(icon_tmp[i*2+1] << 8);
	}
	SPRITE_PALETTE_SUB[49] = RGB5(31,0,0);
	file = fopen("/Menu/close.nbfc", "rb");
	fread(&icon_tmp[0], 1, 128, file);
	fclose(file);
	for(int i = 0; i < 128 / 2; i++)
	{
		SPRITE_GFX_SUB[i + 512 + 64 + 64 + 32 + 32 + 5 * 128] = icon_tmp[i*2]|(icon_tmp[i*2+1] << 8);
	}
	OAM_SUB[OAM_TOOLBAR_BG_IDX * 4] = ATTR0_ROTSCALE_DOUBLE | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_WIDE | OBJ_Y(-28);
	OAM_SUB[OAM_TOOLBAR_BG_IDX * 4 + 1] = ATTR1_SIZE_64 | OBJ_X(0) | ATTR1_ROTDATA(0);
	OAM_SUB[OAM_TOOLBAR_BG_IDX * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(0);
	OAM_SUB[3] = 128;
	OAM_SUB[7] = 0;
	OAM_SUB[11] = 0;
	OAM_SUB[15] = 128;
	OAM_SUB[(OAM_TOOLBAR_BG_IDX + 1) * 4] = ATTR0_ROTSCALE_DOUBLE | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_WIDE | OBJ_Y(-28);
	OAM_SUB[(OAM_TOOLBAR_BG_IDX + 1) * 4 + 1] = ATTR1_SIZE_64 | OBJ_X(128) | ATTR1_ROTDATA(0);
	OAM_SUB[(OAM_TOOLBAR_BG_IDX + 1) * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(0);
}

int Toolbar::FindButtonByPoint(int x, int y)
{
	//this is for all buttons so only check it once
	if(!(y >= 2 && y < (2 + 32))) return TOOLBAR_BUTTON_INVALID;
	if(mFlags.showMenuButton)
	{
		if(x >= 230 && x < (230 + 24)) 
			return TOOLBAR_BUTTON_MENU;
		if(mFlags.showSearchButton)
		{
			if(x >= 196 && x < (196 + 32))
				return TOOLBAR_BUTTON_SEARCH;
			if(mFlags.showClearButton && x >= 164 && x < (164 + 32))
				return TOOLBAR_BUTTON_CLEAR;
		}
	}
	else if(mFlags.showSearchButton)
	{
		if(x >= 222 && x < (222 + 32))
			return TOOLBAR_BUTTON_SEARCH;
		if(mFlags.showClearButton && x >= 190 && x < (190 + 32))
			return TOOLBAR_BUTTON_CLEAR;
	}
	else if(mFlags.showClearButton && x >= 222 && x < (222 + 32))
		return TOOLBAR_BUTTON_CLEAR;
	if(mFlags.showBackButton && x >= 2 && x < (2 + 32))
			return TOOLBAR_BUTTON_BACK;
	return TOOLBAR_BUTTON_INVALID;
}

int Toolbar::OnPenDown(void* context, int x, int y)
{
	if(y >= 36) return FALSE;
	mDownButton = FindButtonByPoint(x, y);
	return TRUE;
}

int Toolbar::OnPenMove(void* context, int x, int y)
{
	if(y >= 36) return FALSE;
	return TRUE;
}

int Toolbar::OnPenUp(void* context, int x, int y)
{
	if(y >= 36) return FALSE;
	int button = FindButtonByPoint(x, y);
	if(button == mDownButton && button != TOOLBAR_BUTTON_INVALID)//handle a click
	{
		if(mOnButtonClickCallback != NULL)
			mOnButtonClickCallback(context, button);
		mDownButton = TOOLBAR_BUTTON_INVALID;
	}
	return TRUE;
}

//Assume rendering is done in vblank!
void Toolbar::Render(void* context)//VBlankUpdate()
{
	if(mFlags.colorInvalidated)
	{
		for(int i = 0; i < 15; i++)
		{
			int rnew = mBGColor.r + ((mIconColor.r - mBGColor.r) * i + 7) / 14;
			int gnew = mBGColor.g + ((mIconColor.g - mBGColor.g) * i + 7) / 14;
			int bnew = mBGColor.b + ((mIconColor.b - mBGColor.b) * i + 7) / 14;
			SPRITE_PALETTE_SUB[i + 1] = RGB5(rnew, gnew, bnew);
			rnew = mBGColor.r + ((mTextColor.r - mBGColor.r) * i + 7) / 14;
			gnew = mBGColor.g + ((mTextColor.g - mBGColor.g) * i + 7) / 14;
			bnew = mBGColor.b + ((mTextColor.b - mBGColor.b) * i + 7) / 14;
			SPRITE_PALETTE_SUB[i + 1 + 16] = RGB5(rnew, gnew, bnew);
		}
		mFlags.colorInvalidated = 0;
	}
	if(mFlags.titleInvalidated)
	{
		int w, h;
		Font_GetStringSize(mFont, mTitle, &w, &h);
		uint8_t* tmp = (uint8_t*)malloc(160 * 16);
		memset(tmp, 0, 160 * 16);
		Font_CreateStringData(mFont, mTitle, tmp + (8 - ((h + 1) >> 1)) * 160, 160);
		Util_ConvertToObj(tmp, 32, 16, 160, &SPRITE_GFX_SUB[512 + 64 + 64 + 32 + 32]);
		Util_ConvertToObj(tmp + 32, 32, 16, 160, &SPRITE_GFX_SUB[512 + 64 + 64 + 32 + 32 + 128]);
		Util_ConvertToObj(tmp + 2 * 32, 32, 16, 160, &SPRITE_GFX_SUB[512 + 64 + 64 + 32 + 32 + 2 * 128]);
		Util_ConvertToObj(tmp + 3 * 32, 32, 16, 160, &SPRITE_GFX_SUB[512 + 64 + 64 + 32 + 32 + 3 * 128]);
		Util_ConvertToObj(tmp + 4 * 32, 32, 16, 160, &SPRITE_GFX_SUB[512 + 64 + 64 + 32 + 32 + 4 * 128]);
		free(tmp);
		mFlags.titleInvalidated = 0;
	}
	if(mFlags.buttonsInvalidated)
	{
		int text_x;
		if(!mFlags.showBackButton)
		{
			OAM_SUB[OAM_BACK_ICON_IDX * 4] = ATTR0_DISABLED;
			text_x = 10;
		}
		else
		{
			OAM_SUB[OAM_BACK_ICON_IDX * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_SQUARE | OBJ_Y(10);
			OAM_SUB[OAM_BACK_ICON_IDX * 4 + 1] = ATTR1_SIZE_16 | OBJ_X(10);
			OAM_SUB[OAM_BACK_ICON_IDX * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(0) | (16);
			text_x = 47;
		}
		OAM_SUB[OAM_TITLE_TEXT_IDX * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_WIDE | OBJ_Y(11);
		OAM_SUB[OAM_TITLE_TEXT_IDX * 4 + 1] = ATTR1_SIZE_32 | OBJ_X(text_x);
		OAM_SUB[OAM_TITLE_TEXT_IDX * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(1) | (16 + 2 + 2 + 1 + 1);
		OAM_SUB[(OAM_TITLE_TEXT_IDX + 1) * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_WIDE | OBJ_Y(11);
		OAM_SUB[(OAM_TITLE_TEXT_IDX + 1) * 4 + 1] = ATTR1_SIZE_32 | OBJ_X(text_x + 32);
		OAM_SUB[(OAM_TITLE_TEXT_IDX + 1) * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(1) | (16 + 2 + 2 + 1 + 1 + 4);
		OAM_SUB[(OAM_TITLE_TEXT_IDX + 2) * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_WIDE | OBJ_Y(11);
		OAM_SUB[(OAM_TITLE_TEXT_IDX + 2) * 4 + 1] = ATTR1_SIZE_32 | OBJ_X(text_x + 2 * 32);
		OAM_SUB[(OAM_TITLE_TEXT_IDX + 2) * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(1) | (16 + 2 + 2 + 1 + 1 + 2 * 4);
		OAM_SUB[(OAM_TITLE_TEXT_IDX + 3) * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_WIDE | OBJ_Y(11);
		OAM_SUB[(OAM_TITLE_TEXT_IDX + 3) * 4 + 1] = ATTR1_SIZE_32 | OBJ_X(text_x + 3 * 32);
		OAM_SUB[(OAM_TITLE_TEXT_IDX + 3) * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(1) | (16 + 2 + 2 + 1 + 1 + 3 * 4);
		OAM_SUB[(OAM_TITLE_TEXT_IDX + 4) * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_WIDE | OBJ_Y(11);
		OAM_SUB[(OAM_TITLE_TEXT_IDX + 4) * 4 + 1] = ATTR1_SIZE_32 | OBJ_X(text_x + 4 * 32);
		OAM_SUB[(OAM_TITLE_TEXT_IDX + 4) * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(1) | (16 + 2 + 2 + 1 + 1 + 4 * 4);
		if(!mFlags.showMenuButton)
			OAM_SUB[OAM_MENU_ICON_IDX * 4]= ATTR0_DISABLED;
		else
		{
			OAM_SUB[OAM_MENU_ICON_IDX * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_TALL | OBJ_Y(10);
			OAM_SUB[OAM_MENU_ICON_IDX * 4 + 1] = ATTR1_SIZE_8 | OBJ_X(238);
			OAM_SUB[OAM_MENU_ICON_IDX * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(0) | (16 + 2 + 2);
		}
		if(!mFlags.showSearchButton)
			OAM_SUB[OAM_SEARCH_ICON_IDX * 4] = ATTR0_DISABLED;
		else
		{
			OAM_SUB[OAM_SEARCH_ICON_IDX * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_SQUARE | OBJ_Y(10);
			OAM_SUB[OAM_SEARCH_ICON_IDX * 4 + 1] = ATTR1_SIZE_16 | OBJ_X((mFlags.showMenuButton ? 204 : 230));
			OAM_SUB[OAM_SEARCH_ICON_IDX * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(0) | (16 + 2);
		}
		if(!mFlags.showClearButton)
			OAM_SUB[OAM_CLEAR_ICON_IDX * 4] = ATTR0_DISABLED;
		else
		{
			OAM_SUB[OAM_CLEAR_ICON_IDX * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_SQUARE | OBJ_Y(10);
			OAM_SUB[OAM_CLEAR_ICON_IDX * 4 + 1] = ATTR1_SIZE_16 | OBJ_X((mFlags.showMenuButton ? 204 : 230) - (mFlags.showSearchButton ? 32 : 0));
			OAM_SUB[OAM_CLEAR_ICON_IDX * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(0) | (16 + 2 + 2 + 1 + 1 + 5 * 4);
		}
		mFlags.buttonsInvalidated = 0;
	}
	if(mFlags.cursorInvalidated)
	{
		if(!mFlags.showCursor)
			OAM_SUB[OAM_TEXT_CURSOR_IDX * 4] = ATTR0_DISABLED;
		else
		{
			OAM_SUB[OAM_TEXT_CURSOR_IDX * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_TALL | OBJ_Y(10);
			OAM_SUB[OAM_TEXT_CURSOR_IDX * 4 + 1] = ATTR1_SIZE_8 | OBJ_X((mFlags.showBackButton ? 47 : 10) + mCursorX);
			OAM_SUB[OAM_TEXT_CURSOR_IDX * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(3) | (16 + 2 + 2 + 1);
		}
		mFlags.cursorInvalidated = 0;
	}
}

void Toolbar::CleanupVRAM()
{
	OAM_SUB[OAM_BACK_ICON_IDX * 4] = ATTR0_DISABLED;
	OAM_SUB[OAM_TITLE_TEXT_IDX * 4] = ATTR0_DISABLED;
	OAM_SUB[(OAM_TITLE_TEXT_IDX + 1) * 4] = ATTR0_DISABLED;
	OAM_SUB[(OAM_TITLE_TEXT_IDX + 2) * 4] = ATTR0_DISABLED;
	OAM_SUB[(OAM_TITLE_TEXT_IDX + 3) * 4] = ATTR0_DISABLED;
	OAM_SUB[(OAM_TITLE_TEXT_IDX + 4) * 4] = ATTR0_DISABLED;
	OAM_SUB[OAM_MENU_ICON_IDX * 4]= ATTR0_DISABLED;
	OAM_SUB[OAM_SEARCH_ICON_IDX * 4] = ATTR0_DISABLED;
	OAM_SUB[OAM_CLEAR_ICON_IDX * 4] = ATTR0_DISABLED;
	OAM_SUB[OAM_TEXT_CURSOR_IDX * 4] = ATTR0_DISABLED;
}