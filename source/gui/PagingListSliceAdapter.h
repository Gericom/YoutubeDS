#ifndef __PAGINGLISTSLICEADAPTER_H__
#define __PAGINGLISTSLICEADAPTER_H__
#include "ListSliceAdapter.h"

class PagingListSliceAdapter : public ListSliceAdapter
{
protected:
	int mNrEntriesPerPage;
	void* mPageDatas[3];//prev,cur,next
	int mCurPage;
public:
	PagingListSliceAdapter(int itemHeight, int itemNrOAMCellsRequired, int itemOAMVramSpaceRequired, int nrEntriesPerPage)
		: ListSliceAdapter(itemHeight, itemNrOAMCellsRequired, itemOAMVramSpaceRequired), mNrEntriesPerPage(nrEntriesPerPage), mCurPage(0)
	{ 
		mPageDatas[0] = NULL;
		mPageDatas[1] = NULL;
		mPageDatas[2] = NULL;
	}

	int GetItemCount()
	{
		/*int nrentries = 0;
		if(mCurPage > 1)
			nrentries += (mCurPage - 1) * mNrEntriesPerPage;
		if(mPageDatas[1])
		{
			nrentries += mNrEntriesPerPage;
			if(mPageDatas[2])
				nrentries += mNrEntriesPerPage;
			else
				nrentries++;
		}
		else
			nrentries++;
		return nrentries;*/
		int nrentries = mCurPage * mNrEntriesPerPage;
		if(mPageDatas[1])
		{
			nrentries += mNrEntriesPerPage;
			if(mPageDatas[2])
				nrentries += mNrEntriesPerPage;
			else
				nrentries++;
		}
		else
			nrentries++;
		return nrentries;
		//return 1000;
	}

	ListElementSlice* GetSlice(int position);

	virtual ListElementSlice* GetPageSlice(void* pageData, int index) = 0;
	virtual ListElementSlice* GetLoadingSlice() = 0;

	virtual void OnRequestPageData(int page) = 0;
	virtual void OnFreePageData(void* pageData) = 0;
	void SetPageData(int page, void* pageData)
	{
		if(page == mCurPage || page == mCurPage - 1 || page == mCurPage + 1)
		{
			mPageDatas[1 + page - mCurPage] = pageData;
			SetIsInvalidated();
		}
		else
			OnFreePageData(pageData);
	}
};

#endif