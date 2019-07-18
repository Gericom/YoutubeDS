#ifndef __PROGRESSBAR_H__
#define __PROGRESSBAR_H__
#include "UISlice.h"

class ProgressBar : public UISlice
{
private:
	int mOamIdx;
	int mFrame;
public:
	ProgressBar(int oamIdx)
		: mOamIdx(oamIdx), mFrame(0)
	{ }

	int OnPenDown(void* context, int x, int y);
	int OnPenMove(void* context, int x, int y);
	int OnPenUp(void* context, int x, int y);

	void Render(void* context);

	void CleanupVRAM();

	static void InitializeVRAM();
};

#endif