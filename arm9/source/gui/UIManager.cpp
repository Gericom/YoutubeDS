#include <nds.h>
#include "PointerList.h"
#include "UISlice.h"
#include "UIManager.h"

UIManager::UIManager(void* context)
	: mContext(context), mSliceList(NULL)
{
	memset(&mLastTouchState, 0, sizeof(mLastTouchState));
	mOnPenDownFunc = NULL;
	mOnPenMoveFunc = NULL;
	mOnPenUpFunc = NULL;
}

void UIManager::AddSlice(UISlice* slice)
{
	PointerList_Add(&mSliceList, slice);
	//should we call an init function here?
}

void UIManager::RemoveSlice(UISlice* slice)
{
	PointerList_Remove(&mSliceList, slice);
}

void UIManager::ProcessInput()
{
    touchPosition disp_point;
	scanKeys();
	uint16_t keyData = keysHeld();
	touchRead(&disp_point);
	if(!(mLastButtonState & KEY_TOUCH) && (keyData & KEY_TOUCH))//PenDown
	{
		PointerListEntry* sliceNode = mSliceList;
		while (sliceNode != NULL)
		{
			if(((UISlice*)sliceNode->ptr)->OnPenDown(mContext, disp_point.px, disp_point.py))
				break;
			sliceNode = sliceNode->next;
		}
		if(sliceNode == NULL && mOnPenDownFunc != NULL) //no slice has done anything, let's pass the event through
			mOnPenDownFunc(mContext, disp_point.px, disp_point.py);
	}
	else if((mLastButtonState & KEY_TOUCH) && (keyData & KEY_TOUCH) && (mLastTouchState.px != disp_point.px || mLastTouchState.py != disp_point.py))//PenMove
	{
		PointerListEntry* sliceNode = mSliceList;
		while (sliceNode != NULL)
		{
			if(((UISlice*)sliceNode->ptr)->OnPenMove(mContext, disp_point.px, disp_point.py))
				break;
			sliceNode = sliceNode->next;
		}
		if(sliceNode == NULL && mOnPenMoveFunc != NULL) //no slice has done anything, let's pass the event through
			mOnPenMoveFunc(mContext, disp_point.px, disp_point.py);
	}
	else if((mLastButtonState & KEY_TOUCH) && !(keyData & KEY_TOUCH))//PenUp
	{
		PointerListEntry* sliceNode = mSliceList;
		while (sliceNode != NULL)
		{
			if(((UISlice*)sliceNode->ptr)->OnPenUp(mContext, mLastTouchState.px, mLastTouchState.py)) 
				break;
			sliceNode = sliceNode->next;
		}
		if(sliceNode == NULL && mOnPenUpFunc != NULL) //no slice has done anything, let's pass the event through
			mOnPenUpFunc(mContext, mLastTouchState.px, mLastTouchState.py);
	}
	mLastTouchState = disp_point;
	mLastButtonState = keyData;
}

void UIManager::Render()
{
	PointerListEntry* sliceNode = mSliceList;
	while (sliceNode != NULL)
	{
		((UISlice*)sliceNode->ptr)->Render(mContext);
		sliceNode = sliceNode->next;
	}
}