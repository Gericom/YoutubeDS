#include <nds.h>
#include <stdio.h>
#include "ProgressBar.h"

int ProgressBar::OnPenDown(void* context, int x, int y)
{
	if(x >= mX && x < mX + 32 && y >= mY && y < mY + 32) return TRUE;
	return FALSE;
}

int ProgressBar::OnPenMove(void* context, int x, int y)
{
	if(x >= mX && x < mX + 32 && y >= mY && y < mY + 32) return TRUE;
	return FALSE;
}

int ProgressBar::OnPenUp(void* context, int x, int y)
{
	if(x >= mX && x < mX + 32 && y >= mY && y < mY + 32) return TRUE;
	return FALSE;
}

void ProgressBar::Render(void* context)
{
	if(!(mFrame % 6))
	{
		OAM_SUB[mOamIdx * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_SQUARE | OBJ_Y(mY);
		OAM_SUB[mOamIdx * 4 + 1] = ATTR1_SIZE_32 | OBJ_X(mX);
		OAM_SUB[mOamIdx * 4 + 2] = ATTR2_PRIORITY(3) | ATTR2_PALETTE(6) | (4608 / 64 + 8 * (mFrame / 6));//(mOAMVramOffset / 64);
	}
	mFrame++;
	if(mFrame == 38 * 6)
		mFrame = 0;
}

void ProgressBar::CleanupVRAM()
{
	OAM_SUB[mOamIdx * 4] = ATTR0_DISABLED;
}

void ProgressBar::InitializeVRAM()
{
	uint8_t* load_tmp = (uint8_t*)malloc(19456);
	FILE* file = fopen("/Menu/progress.nbfc", "rb");
	fread(&load_tmp[0], 1, 19456, file);
	fclose(file);
	DC_FlushRange(load_tmp, 19456);
	dmaCopyWords(3, load_tmp, &SPRITE_GFX[0], 19456);
	dmaCopyWords(3, load_tmp, &SPRITE_GFX_SUB[4608 >> 1], 19456);
	free(load_tmp);
}