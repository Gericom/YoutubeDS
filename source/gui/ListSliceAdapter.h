#ifndef __LISTSLICEADAPTER_H__
#define __LISTSLICEADAPTER_H__
#include "ListElementSlice.h"

class ListSliceAdapter
{
private:
	int mItemHeight;
	int mItemNrOAMCellsRequired;
	int mItemOAMVramSpaceRequired;
	int mInvalidated;
protected:
	void SetIsInvalidated()
	{
		mInvalidated = TRUE;
	}
public:
	ListSliceAdapter(int itemHeight, int itemNrOAMCellsRequired, int itemOAMVramSpaceRequired)
		: mItemHeight(itemHeight), mItemNrOAMCellsRequired(itemNrOAMCellsRequired), mItemOAMVramSpaceRequired(itemOAMVramSpaceRequired), mInvalidated(FALSE)
	{ }

	virtual ~ListSliceAdapter()
	{

	}

	virtual int GetItemCount() = 0;
	int GetItemHeight() { return mItemHeight; }
	int GetItemNrOAMCellsRequired() { return mItemNrOAMCellsRequired; }
	int GetItemOAMVramSpaceRequired() { return mItemOAMVramSpaceRequired; }

	virtual ListElementSlice* GetSlice(int position) = 0;

	int GetIsInvalidated()
	{ 
		int result = mInvalidated;
		mInvalidated = FALSE;
		return result;
	}

	//Maybe recycling should be implemented later on
	//virtual ListElementSlice* OnCreateSlice(int sliceType) = 0;
	//virtual void OnBindSlice(ListElementSlice* slice, int position) = 0;
};

#endif