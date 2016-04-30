#ifndef __LISTELEMENTSLICE_H__
#define __LISTELEMENTSLICE_H__
#include "UISlice.h"

class ListElementSlice : public UISlice
{
protected:
	int mFirstOAMIdx;
	int mOAMVramOffset;
private:
	int mIsVramInitialized;
	virtual void OnInitializeVram();
public:
	ListElementSlice()
		: mFirstOAMIdx(-1), mOAMVramOffset(-1), mIsVramInitialized(FALSE)
	{ }

	void InitializeVram() 
	{ 
		OnInitializeVram(); 
		mIsVramInitialized = TRUE; 
	}

	int IsVramInitialized() { return mIsVramInitialized; }

	int GetFirstOAMIdx() { return mFirstOAMIdx; }
	void SetFirstOAMIdx(int firstOAMIdx) { mFirstOAMIdx = firstOAMIdx; }
	int GetOAMVramOffset() { return mOAMVramOffset; }
	void SetOAMVramOffset(int oamVramOffset) { mOAMVramOffset = oamVramOffset; }
};

#endif