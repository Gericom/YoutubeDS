#ifndef __UIMANAGER_H__
#define __UIMANAGER_H__
#include "PointerList.h"

class UISlice;

class UIManager
{
public:
	typedef void (*PenFunc)(void* context, int x, int y);
private:
	void* mContext;

	PointerListEntry* mSliceList;
	touchPosition mLastTouchState;
	uint16_t mLastButtonState;

	PenFunc mOnPenDownFunc;
	PenFunc mOnPenMoveFunc;
	PenFunc mOnPenUpFunc;
public:
	UIManager(void* context);

	void AddSlice(UISlice* slice);
	void RemoveSlice(UISlice* slice);
	void RegisterPenCallbacks(PenFunc onPenDown, PenFunc onPenMove, PenFunc onPenUp)
	{
		mOnPenDownFunc = onPenDown;
		mOnPenMoveFunc = onPenMove;
		mOnPenUpFunc = onPenUp;
	}

	void ProcessInput();
	void Render();
};

#endif