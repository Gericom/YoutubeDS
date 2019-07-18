#include <nds.h>
#include <stdio.h>
#include "ScreenKeyboard.h"

#define OAM_SCREENKEYBOARD_RIPPLE_IDX				12
#define OAM_SCREENKEYBOARD_SPACE_HIGHLIGHT_IDX		(OAM_SCREENKEYBOARD_RIPPLE_IDX + 1)

void ScreenKeyboard::Initialize()
{
	uint8_t* load_tmp = (uint8_t*)malloc(12800);
	FILE* file = fopen("/Keyboard/keyboard.nbfp", "rb");
	fread(&load_tmp[0], 1, 32, file);
	fclose(file);
	for(int i = 0; i < 16; i++)
	{
		BG_PALETTE_SUB[16 + i] = load_tmp[i*2]|(load_tmp[i*2+1] << 8);
	}
	file = fopen("/Keyboard/keyboard.nbfc", "rb");
	fread(&load_tmp[0], 1, 12800, file);
	fclose(file);
	DC_FlushRange(load_tmp, 12800);
	dmaCopyWords(3, load_tmp, &BG_GFX_SUB[64 >> 1], 12800);
	/*for(int i = 0; i < 12800 / 2; i++)
	{
		BG_GFX_SUB[(64 >> 1) + i] = load_tmp[i*2]|(load_tmp[i*2+1] << 8);
	}*/
	file = fopen("/Keyboard/keyboard_lower.nbfs", "rb");
	fread(&load_tmp[0], 1, 2048, file);
	fclose(file);
	for(int i = 0; i < 2048 / 2; i++)
	{
		BG_GFX_SUB[(0x4000 >> 1) + i] = ((load_tmp[i*2]|(load_tmp[i*2+1] << 8)) + 2) | (1 << 12);
	}
	file = fopen("/Keyboard/keyboard_upper.nbfs", "rb");
	fread(&load_tmp[0], 1, 2048, file);
	fclose(file);
	for(int i = 0; i < 2048 / 2; i++)
	{
		BG_GFX_SUB[(0x4800 >> 1) + i] = ((load_tmp[i*2]|(load_tmp[i*2+1] << 8)) + 2) | (1 << 12);
	}
	file = fopen("/Keyboard/keyboard_numsym.nbfs", "rb");
	fread(&load_tmp[0], 1, 2048, file);
	fclose(file);
	for(int i = 0; i < 2048 / 2; i++)
	{
		BG_GFX_SUB[(0x5000 >> 1) + i] = ((load_tmp[i*2]|(load_tmp[i*2+1] << 8)) + 2) | (1 << 12);
	}
	file = fopen("/Keyboard/ripple.nbfp", "rb");
	fread(&load_tmp[0], 1, 32, file);
	fclose(file);
	for(int i = 0; i < 16; i++)
	{
		SPRITE_PALETTE_SUB[32 + i] = load_tmp[i*2] | (load_tmp[i*2+1] << 8);
	}
	file = fopen("/Keyboard/ripple.nbfc", "rb");
	fread(&load_tmp[0], 1, 512, file);
	fclose(file);
	for(int i = 0; i < 512 / 2; i++)
	{
		SPRITE_GFX_SUB[512 + 64 + 64 + 32 + 32 + 5 * 128 + 64 + i] = load_tmp[i*2] | (load_tmp[i*2+1] << 8);
	}
	file = fopen("/Keyboard/space_highlight.nbfp", "rb");
	fread(&load_tmp[0], 1, 32, file);
	fclose(file);
	for(int i = 0; i < 16; i++)
	{
		SPRITE_PALETTE_SUB[64 + i] = load_tmp[i*2] | (load_tmp[i*2+1] << 8);
	}
	file = fopen("/Keyboard/space_highlight.nbfc", "rb");
	fread(&load_tmp[0], 1, 1024, file);
	fclose(file);
	for(int i = 0; i < 1024 / 2; i++)
	{
		SPRITE_GFX_SUB[512 + 64 + 64 + 32 + 32 + 5 * 128 + 64 + 256 + i] = load_tmp[i*2] | (load_tmp[i*2+1] << 8);
	}
	file = fopen("/Keyboard/space_highlight_small.nbfc", "rb");
	fread(&load_tmp[0], 1, 256, file);
	fclose(file);
	for(int i = 0; i < 256 / 2; i++)
	{
		SPRITE_GFX_SUB[512 + 64 + 64 + 32 + 32 + 5 * 128 + 64 + 256 + 512 + i] = load_tmp[i*2] | (load_tmp[i*2+1] << 8);
	}
	free(load_tmp);
	REG_BG0CNT_SUB = BG_32x32 | BG_PRIORITY_1 | BG_COLOR_16 | BG_MAP_BASE(8);
	REG_BG0VOFS_SUB = -mY;
	SUB_WIN0_X0 = 0;
	SUB_WIN0_X1 = 255;
	SUB_WIN1_X0 = 1;
	SUB_WIN1_X1 = 0;
	SUB_WIN0_Y0 = SUB_WIN1_Y0 = mY;
	SUB_WIN0_Y1 = SUB_WIN1_Y1 = mY;
	SUB_WIN_IN = (1 << 0) | (1 << 8) | (1 << 4) | (1 << (4 + 8)) | (1 << 5) | (1 << (5 + 8));
	SUB_WIN_OUT = (1 << 5) | (1 << 4) | (1 << 1);
}

static const char keyboardTopRow[3][10] =
{
	{ 'q','w','e','r','t','y','u','i','o','p' },
	{ 'Q','W','E','R','T','Y','U','I','O','P' },
	{ '1','2','3','4','5','6','7','8','9','0' }
};

static const char keyboardMiddleRow[3][9] =
{
	{ 'a','s','d','f','g','h','j','k','l' },
	{ 'A','S','D','F','G','H','J','K','L' },
	{ '@','#','$','%','&','-','+','(',')' }
};

static const char keyboardBottomRow[3][8] =
{
	{ 'z','x','c','v','b','n','m' },
	{ 'Z','X','C','V','B','N','M' },
	{ '*','"','\'',':',';','!','?'}
};

int ScreenKeyboard::FindButtonByPoint(int x, int y)
{
	if(y < 2) return 0;
	if(y >= 2 && y < (2 + 32))//top row
		return (int)keyboardTopRow[mKeyboardMode][(x + 2) / 26];
	if(y >= 44 && y < (44 + 32))//middle row
	{
		if(x < 13 || x >= 247) return 0;
		return (int)keyboardMiddleRow[mKeyboardMode][(x - 13) / 26];
	}
	if(x >= 38 && x < 220 && y >= 83 && y < (83 + 32))//bottom row
		return (int)keyboardBottomRow[mKeyboardMode][(x - 38) / 26];
	if(x >= 38 && x < (38 + 26) && y >= 122 && y < (122 + 32))//,
		return (int)',';
	if(mKeyboardMode == SCREENKEYBOARD_MODE_NUMSYM && x >= 64 && x < (64 + 26) && y >= 122 && y < (122 + 32))
		return (int)'_';
	if(mKeyboardMode == SCREENKEYBOARD_MODE_NUMSYM && x >= 167 && x < (167 + 26) && y >= 122 && y < (122 + 32))
		return (int)'/';
	if(x >= 192 && x < (192 + 26) && y >= 122 && y < (122 + 32))//,
		return (int)'.';
	//special keys
	if((mKeyboardMode == SCREENKEYBOARD_MODE_QWERTY_LOWER || mKeyboardMode == SCREENKEYBOARD_MODE_QWERTY_UPPER) &&
		x >= 92 && x < 190 && y >= 121 && y < (121 + 32))//space bar
		return (int)' ';
	if(mKeyboardMode == SCREENKEYBOARD_MODE_NUMSYM && x >= 92 && x < 165 && y >= 121 && y < (121 + 32))//space bar
		return (int)' ';
	if(x >= 7 && x < (7 + 26) && y >= 121 && y < (121 + 32))//modeswitch button
		return SCREENKEYBOARD_BUTTON_SPECIAL_MODESWITCH;
	if((mKeyboardMode == SCREENKEYBOARD_MODE_QWERTY_LOWER || mKeyboardMode == SCREENKEYBOARD_MODE_QWERTY_UPPER) &&
		x >= 5 && x < (5 + 26) && y >= 85 && y < (85 + 32))//caps
		return SCREENKEYBOARD_BUTTON_SPECIAL_CAPS;
	if(x >= 225 && x < (225 + 26) && y >= 82 && y < (82 + 32))//backspace
		return SCREENKEYBOARD_BUTTON_SPECIAL_BACKSPACE;
	if(x >= 224 && x < (224 + 26) && y >= 124 && y < (124 + 26))//search
		return SCREENKEYBOARD_BUTTON_SPECIAL_SEARCH;
	return 0;
}

void ScreenKeyboard::GetButtonCenter(int &x, int &y)
{
	if(y < 2) return;
	if(y >= 2 && y < (2 + 32))//top row
	{
		x = ((x + 2) / 26) * 26 + 26 / 2 - 2;
		y = (2 + 2 + 32) / 2;
		return;
	}
	if(y >= 44 && y < (44 + 32))//middle row
	{
		if(x < 13 || x >= 247) return;
		x = 13 + ((x - 13) / 26) * 26 + 26 / 2;
		y = (44 + 44 + 32) / 2;
		return;
	}
	if(x >= 38 && x < 220 && y >= 83 && y < (83 + 32))//bottom row
	{
		x = 38 + ((x - 38) / 26) * 26 + 26 / 2;
		y = (83 + 83 + 32) / 2;
		return;
	}
	if(x >= 38 && x < (38 + 26) && y >= 122 && y < (122 + 32))//,
	{
		x = (38 + 38 + 26) / 2;
		y = (122 + 122 + 32) / 2;
		return;
	}
	if(mKeyboardMode == SCREENKEYBOARD_MODE_NUMSYM && x >= 64 && x < (64 + 26) && y >= 122 && y < (122 + 32))// _
	{
		x = (64 + 64 + 26) / 2;
		y = (122 + 122 + 32) / 2;
		return;
	}
	if(mKeyboardMode == SCREENKEYBOARD_MODE_NUMSYM && x >= 167 && x < (167 + 26) && y >= 122 && y < (122 + 32))// /
	{
		x = (167 + 167 + 26) / 2;
		y = (122 + 122 + 32) / 2;
		return;
	}
	if(x >= 192 && x < (192 + 26) && y >= 122 && y < (122 + 32))//.
	{
		x = (192 + 192 + 26) / 2;
		y = (122 + 122 + 32) / 2;
		return;
	}
	//special keys
	//if((mKeyboardMode == SCREENKEYBOARD_MODE_QWERTY_LOWER || mKeyboardMode == SCREENKEYBOARD_MODE_QWERTY_UPPER) &&
	//	x >= 92 && x < 190 && y >= 128 && y < 145)//space bar
	//	return (int)' ';
	//if(mKeyboardMode == SCREENKEYBOARD_MODE_NUMSYM && x >= 92 && x < 165 && y >= 128 && y < 145)//space bar
	//	return (int)' ';
	if(x >= 7 && x < (7 + 26) && y >= 121 && y < (121 + 32))//modeswitch button
	{
		x = (7 + 7 + 26) / 2;
		y = (121 + 121 + 32) / 2;
		return;
	}
	if((mKeyboardMode == SCREENKEYBOARD_MODE_QWERTY_LOWER || mKeyboardMode == SCREENKEYBOARD_MODE_QWERTY_UPPER) &&
		x >= 5 && x < (5 + 26) && y >= 85 && y < (85 + 32))//caps
	{
		x = (5 + 5 + 26) / 2;
		y = (85 + 85 + 32) / 2;
		return;
	}
	if(x >= 225 && x < (225 + 26) && y >= 82 && y < (82 + 32))//backspace
	{
		x = (225 + 225 + 26) / 2;
		y = (82 + 82 + 32) / 2;
		return;
	}
	/*if(x >= 224 && x < (224 + 26) && y >= 124 && y < (124 + 26))//search
	{
		x = (224 + 224 + 26) / 2;
		y = (124 + 124 + 26) / 2;
		return;
	}*/
	return;
}

int ScreenKeyboard::OnPenDown(void* context, int x, int y)
{
	if(y < mY) return FALSE;
	y -= mY;
	mDownButton = FindButtonByPoint(x, y);
	if(mDownButton == (int)' ')
		mShowSpaceHighlight = TRUE;
	return TRUE;
}

int ScreenKeyboard::OnPenMove(void* context, int x, int y)
{
	if(y < mY) return FALSE;
	y -= mY;
	int button = FindButtonByPoint(x, y);
	mShowSpaceHighlight = button == (int)' ';
	return TRUE;
}

int ScreenKeyboard::OnPenUp(void* context, int x, int y)
{
	if(y < mY) return FALSE;
	y -= mY;
	int button = FindButtonByPoint(x, y);
	if(button == mDownButton && button != 0)//handle a click
	{
		if(button != (int)' ' && button != SCREENKEYBOARD_BUTTON_SPECIAL_SEARCH)
		{
			int cx = x;
			int cy = y;
			GetButtonCenter(cx, cy);
			//show ripple
			mShowRipple = TRUE;
			mRippleCenterX = (2 * cx + x) / 3;
			mRippleCenterY = (2 * cy + y) / 3;
			mRippleFrame = 0;
		}
		if(button == SCREENKEYBOARD_BUTTON_SPECIAL_MODESWITCH)
		{
			if(mKeyboardMode == SCREENKEYBOARD_MODE_QWERTY_LOWER || mKeyboardMode == SCREENKEYBOARD_MODE_QWERTY_UPPER)
				mKeyboardMode = SCREENKEYBOARD_MODE_NUMSYM;
			else
				mKeyboardMode = SCREENKEYBOARD_MODE_QWERTY_LOWER;
		}
		else if(button == SCREENKEYBOARD_BUTTON_SPECIAL_CAPS)
			mKeyboardMode = (mKeyboardMode == SCREENKEYBOARD_MODE_QWERTY_LOWER) ? SCREENKEYBOARD_MODE_QWERTY_UPPER : SCREENKEYBOARD_MODE_QWERTY_LOWER;
		else
		{
			if(button != SCREENKEYBOARD_BUTTON_SPECIAL_SEARCH && mKeyboardMode == SCREENKEYBOARD_MODE_QWERTY_UPPER)
				mKeyboardMode = SCREENKEYBOARD_MODE_QWERTY_LOWER;
			if(mOnButtonClickCallback != NULL)
				mOnButtonClickCallback(context, button);
		}
		mDownButton = 0;
	}
	mShowSpaceHighlight = FALSE;
	return TRUE;
}

void ScreenKeyboard::Render(void* context)
{
	if(mShowing)
	{
		SetPosition(0, 192 + ((36 - 192) * mShowHideFrame) / 9);
		mShowHideFrame++;
		if(mShowHideFrame == 10)
		{
			mShowHideFrame = 0;
			mShowing = FALSE;
			mHidden = FALSE;
		}
	}
	if(mHiding)
	{
		SetPosition(0, 36 + ((192 - 36) * mShowHideFrame) / 9);
		mShowHideFrame++;
		if(mShowHideFrame == 10)
		{
			mShowHideFrame = 0;
			mHiding = FALSE;
			mHidden = TRUE;
		}
	}
	REG_BG0CNT_SUB = BG_32x32 | BG_PRIORITY_1 | BG_COLOR_16 | BG_MAP_BASE(8 + mKeyboardMode);
	REG_BG0VOFS_SUB = -mY;
	SUB_WIN0_Y0 = SUB_WIN1_Y0 = mY;
	if(mShowRipple)
	{
		OAM_SUB[OAM_SCREENKEYBOARD_RIPPLE_IDX * 4] = ATTR0_ROTSCALE_DOUBLE | ATTR0_TYPE_BLENDED | ATTR0_COLOR_16 | ATTR0_SQUARE | OBJ_Y(mRippleCenterY + mY - 32);
		OAM_SUB[OAM_SCREENKEYBOARD_RIPPLE_IDX * 4 + 1] = ATTR1_SIZE_32 | OBJ_X(mRippleCenterX - 32) | ATTR1_ROTDATA(1);
		OAM_SUB[OAM_SCREENKEYBOARD_RIPPLE_IDX * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(2) | (16 + 2 + 2 + 1 + 1 + 5 * 4 + 2);
		int scale = 512 + ((256 - 512) * mRippleFrame) / 6;
		OAM_SUB[19] = scale;//256;
		OAM_SUB[23] = 0;
		OAM_SUB[27] = 0;
		OAM_SUB[31] = scale;//256;
		mRippleFrame++;
		if(mRippleFrame == 7)
			mShowRipple = FALSE;
	}
	else
		OAM_SUB[OAM_SCREENKEYBOARD_RIPPLE_IDX * 4] = ATTR0_DISABLED;
	if(mShowSpaceHighlight)
	{
		OAM_SUB[OAM_SCREENKEYBOARD_SPACE_HIGHLIGHT_IDX * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_WIDE | OBJ_Y(mY + 128);
		OAM_SUB[OAM_SCREENKEYBOARD_SPACE_HIGHLIGHT_IDX * 4 + 1] = ATTR1_SIZE_64 | OBJ_X(92);
		OAM_SUB[OAM_SCREENKEYBOARD_SPACE_HIGHLIGHT_IDX * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(4) | (16 + 2 + 2 + 1 + 1 + 5 * 4 + 2 + 8);
		if(mKeyboardMode == SCREENKEYBOARD_MODE_NUMSYM)
		{
			OAM_SUB[(OAM_SCREENKEYBOARD_SPACE_HIGHLIGHT_IDX + 1) * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_TALL | OBJ_Y(mY + 128);
			OAM_SUB[(OAM_SCREENKEYBOARD_SPACE_HIGHLIGHT_IDX + 1) * 4 + 1] = ATTR1_SIZE_32 | OBJ_X(156);
			OAM_SUB[(OAM_SCREENKEYBOARD_SPACE_HIGHLIGHT_IDX + 1) * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(4) | (16 + 2 + 2 + 1 + 1 + 5 * 4 + 2 + 8 + 16);
		}
		else
		{
			OAM_SUB[(OAM_SCREENKEYBOARD_SPACE_HIGHLIGHT_IDX + 1) * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_WIDE | OBJ_Y(mY + 128);
			OAM_SUB[(OAM_SCREENKEYBOARD_SPACE_HIGHLIGHT_IDX + 1) * 4 + 1] = ATTR1_SIZE_64 | OBJ_X(126) | ATTR1_FLIP_X;
			OAM_SUB[(OAM_SCREENKEYBOARD_SPACE_HIGHLIGHT_IDX + 1) * 4 + 2] = ATTR2_PRIORITY(0) | ATTR2_PALETTE(4) | (16 + 2 + 2 + 1 + 1 + 5 * 4 + 2 + 8);
		}
	}
	else
	{
		OAM_SUB[OAM_SCREENKEYBOARD_SPACE_HIGHLIGHT_IDX * 4] = ATTR0_DISABLED;
		OAM_SUB[(OAM_SCREENKEYBOARD_SPACE_HIGHLIGHT_IDX + 1) * 4] = ATTR0_DISABLED;
	}
}

void ScreenKeyboard::CleanupVRAM()
{
	OAM_SUB[OAM_SCREENKEYBOARD_RIPPLE_IDX * 4] = ATTR0_DISABLED;
	OAM_SUB[OAM_SCREENKEYBOARD_SPACE_HIGHLIGHT_IDX * 4] = ATTR0_DISABLED;
	OAM_SUB[(OAM_SCREENKEYBOARD_SPACE_HIGHLIGHT_IDX + 1) * 4] = ATTR0_DISABLED;
}