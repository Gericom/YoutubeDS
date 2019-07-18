#include <nds.h>
#include "PagingListSliceAdapter.h"

ListElementSlice* PagingListSliceAdapter::GetSlice(int position)
{
	if(position < 0)
		return GetLoadingSlice();
	//calculate page
	int page = position / mNrEntriesPerPage;
	int idx = position % mNrEntriesPerPage;
	int pageidx = 1 + page - mCurPage;
	if(pageidx < 0 || pageidx > 2)
		return GetLoadingSlice();
	ListElementSlice* result;
	if(mPageDatas[pageidx])
		result = GetPageSlice(mPageDatas[pageidx], idx);
	else
	{
		result = GetLoadingSlice();
		OnRequestPageData(page);
	}
	if(pageidx == 1 && mPageDatas[1] != NULL && mPageDatas[2] == NULL)
		OnRequestPageData(mCurPage + 1);
	else if(pageidx == 1 && mCurPage > 0 && mPageDatas[1] != NULL && mPageDatas[2] != NULL && mPageDatas[0] == NULL)
		OnRequestPageData(mCurPage - 1);
	if(pageidx == 0 && idx < mNrEntriesPerPage / 4)
	{
		if(mPageDatas[2] != NULL)
			OnFreePageData(mPageDatas[2]);
		mPageDatas[2] = mPageDatas[1];
		mPageDatas[1] = mPageDatas[0];
		mPageDatas[0] = NULL;
		mCurPage--;
	}
	else if(pageidx == 2 && idx > 3 * mNrEntriesPerPage / 4)
	{
		if(mPageDatas[0])
			OnFreePageData(mPageDatas[0]);
		mPageDatas[0] = mPageDatas[1];
		mPageDatas[1] = mPageDatas[2];
		mPageDatas[2] = NULL;
		mCurPage++;
	}
	return result;
}