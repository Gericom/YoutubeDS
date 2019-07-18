#include <nds.h>
#include "ListSlice.h"

ListSlice::ListSlice(int y, int height, ListSliceAdapter* adapter, int oamIdx, int oamCount, int oamVramOffset, int oamVramSize)
	: mHeight(height), mAdapter(adapter), mPositionedSlices(NULL), mScrollY(0), mOAMIdx(oamIdx), mOAMVramOffset(oamVramOffset), mPenDown(FALSE), 
		mOAMChunkSize(0), mNrOAMChunks(0), mOAMChunkUsageBitmask(NULL), mOAMChunkCleanupBitmask(NULL),
		mVramChunkSize(0), mNrVramChunks(0), mVramChunkUsageBitmask(NULL)
{
	mY = y;
	if(mAdapter != NULL)
	{
		mOAMChunkSize = mAdapter->GetItemNrOAMCellsRequired();
		if(mOAMChunkSize)
		{
			mNrOAMChunks = oamCount / mOAMChunkSize;
			int size = 4 + ((mNrOAMChunks >> 5) << 3);
			mOAMChunkUsageBitmask = (uint32_t*)malloc(size);
			memset(mOAMChunkUsageBitmask, 0, size);
			mOAMChunkCleanupBitmask = (uint32_t*)malloc(size);
			memset(mOAMChunkCleanupBitmask, 0, size);
		}
		mVramChunkSize = mAdapter->GetItemOAMVramSpaceRequired();
		if(mVramChunkSize)
		{
			mNrVramChunks = oamVramSize / mVramChunkSize;
			int size = 4 + ((mNrVramChunks >> 5) << 3);
			mVramChunkUsageBitmask = (uint32_t*)malloc(size);
			memset(mVramChunkUsageBitmask, 0, size);
		}
		int nrItems = mAdapter->GetItemCount();
		if(nrItems)
		{
			//Get the first slices already
			int elementHeight = mAdapter->GetItemHeight();
			//int elementNrCellsRequired = mAdapter->GetItemNrOAMCellsRequired();
			//int elementVramSpaceRequired = mAdapter->GetItemOAMVramSpaceRequired();
			int elementY = 0;
			int idx = 0;
			while(idx < nrItems && elementY < mHeight)
			{
				ListElementSlice* slice = mAdapter->GetSlice(idx);
				if(mOAMChunkSize)
				{
					int oam = mOAMIdx + FindUnusedOAMChunk() * mOAMChunkSize;
					slice->SetFirstOAMIdx(oam);
				}
				if(mVramChunkSize)
				{
					int vram = mOAMVramOffset + FindUnusedVramChunk() * mVramChunkSize;
					slice->SetOAMVramOffset(vram);
				}
				slice->SetPosition(0, mY + elementY);
				slice->InitializeVram();
				PositionedListElementSlice* positionedSlice = (PositionedListElementSlice*)malloc(sizeof(PositionedListElementSlice));
				positionedSlice->position = idx;
				positionedSlice->slice = slice;
				PointerList_Add(&mPositionedSlices, positionedSlice);
				idx++;
				elementY += elementHeight;
			}
			mFirstPositionedSlice = 0;
			mLastPositionedSlice = idx - 1;
		}
	}
}

int ListSlice::OnPenDown(void* context, int x, int y)
{
	if(y < mY) return FALSE;
	if(y >= (mY + mHeight)) return FALSE;
	PointerListEntry* sliceNode = mPositionedSlices;
	while (sliceNode != NULL)
	{
		((PositionedListElementSlice*)sliceNode->ptr)->slice->OnPenDown(context, x, y);
		sliceNode = sliceNode->next;
	}
	mPenDown = TRUE;
	mLastY = y;
	return TRUE;
}

//TODO: Fix bug when delta is bigger as one list entry
int ListSlice::OnPenMove(void* context, int x, int y)
{
	if(y < mY) return FALSE;
	if(y >= (mY + mHeight)) return FALSE;
	PointerListEntry* sliceNode = mPositionedSlices;
	while (sliceNode != NULL)
	{
		((PositionedListElementSlice*)sliceNode->ptr)->slice->OnPenMove(context, x, y);
		sliceNode = sliceNode->next;
	}
	if(mPenDown)
	{
		int delta = y - mLastY;
		int prevscrolly = mScrollY;
		mScrollY -= delta;
		if(mScrollY < 0) mScrollY = 0;
		if(mScrollY > mAdapter->GetItemCount() * mAdapter->GetItemHeight() - mHeight)
			mScrollY = mAdapter->GetItemCount() * mAdapter->GetItemHeight() - mHeight;
		delta = prevscrolly - mScrollY;
		//update scroll
		PointerListEntry* sliceNode = mPositionedSlices;
		while (sliceNode != NULL)
		{
			int sx, sy;
			((PositionedListElementSlice*)sliceNode->ptr)->slice->GetPosition(sx, sy);
			sy += delta;
			if(sy + mAdapter->GetItemHeight() < mY)//drop list item
			{
				mFirstPositionedSlice++;
				if(mOAMChunkSize)
				{
					int oamidx = ((PositionedListElementSlice*)sliceNode->ptr)->slice->GetFirstOAMIdx();
					SetIsOAMChunkInUse((oamidx - mOAMIdx) / mOAMChunkSize, FALSE);
				}
				if(mVramChunkSize)
				{
					int vramoffset = ((PositionedListElementSlice*)sliceNode->ptr)->slice->GetOAMVramOffset();
					SetIsVramChunkInUse((vramoffset - mOAMVramOffset) / mVramChunkSize, FALSE);
				}
				delete ((PositionedListElementSlice*)sliceNode->ptr)->slice;
				free(sliceNode->ptr);
				PointerListEntry* old = sliceNode;
				sliceNode = sliceNode->next;
				PointerList_RemoveEntry(&mPositionedSlices, old);
				continue;
			}
			else if(sy >= mY + mHeight)//drop list item
			{
				mLastPositionedSlice--;
				if(mOAMChunkSize)
				{
					int oamidx = ((PositionedListElementSlice*)sliceNode->ptr)->slice->GetFirstOAMIdx();
					SetIsOAMChunkInUse((oamidx - mOAMIdx) / mOAMChunkSize, FALSE);
				}
				if(mVramChunkSize)
				{
					int vramoffset = ((PositionedListElementSlice*)sliceNode->ptr)->slice->GetOAMVramOffset();
					SetIsVramChunkInUse((vramoffset - mOAMVramOffset) / mVramChunkSize, FALSE);
				}
				delete ((PositionedListElementSlice*)sliceNode->ptr)->slice;
				free(sliceNode->ptr);
				PointerListEntry* old = sliceNode;
				sliceNode = sliceNode->next;
				PointerList_RemoveEntry(&mPositionedSlices, old);
				continue;
			}
			else if(((PositionedListElementSlice*)sliceNode->ptr)->position == mFirstPositionedSlice &&
				sy > mY)//pull in a new list item
			{
				mFirstPositionedSlice--;
				ListElementSlice* slice = mAdapter->GetSlice(mFirstPositionedSlice);
				if(mOAMChunkSize)
				{
					int oam = mOAMIdx + FindUnusedOAMChunk() * mOAMChunkSize;
					slice->SetFirstOAMIdx(oam);
				}
				if(mVramChunkSize)
				{
					int vram = mOAMVramOffset + FindUnusedVramChunk() * mVramChunkSize;
					slice->SetOAMVramOffset(vram);
				}
				slice->SetPosition(0, mY + mFirstPositionedSlice * mAdapter->GetItemHeight() - mScrollY);
				PositionedListElementSlice* positionedSlice = (PositionedListElementSlice*)malloc(sizeof(PositionedListElementSlice));
				positionedSlice->position = mFirstPositionedSlice;
				positionedSlice->slice = slice;
				PointerList_Add(&mPositionedSlices, positionedSlice);
			}
			else if(((PositionedListElementSlice*)sliceNode->ptr)->position == mLastPositionedSlice &&
				sy + mAdapter->GetItemHeight() < mY + mHeight)//pull in a new list item
			{
				mLastPositionedSlice++;
				ListElementSlice* slice = mAdapter->GetSlice(mLastPositionedSlice);
				if(mOAMChunkSize)
				{
					int oam = mOAMIdx + FindUnusedOAMChunk() * mOAMChunkSize;
					slice->SetFirstOAMIdx(oam);
				}
				if(mVramChunkSize)
				{
					int vram = mOAMVramOffset + FindUnusedVramChunk() * mVramChunkSize;
					slice->SetOAMVramOffset(vram);
				}
				slice->SetPosition(0, mY + mLastPositionedSlice * mAdapter->GetItemHeight() - mScrollY);
				PositionedListElementSlice* positionedSlice = (PositionedListElementSlice*)malloc(sizeof(PositionedListElementSlice));
				positionedSlice->position = mLastPositionedSlice;
				positionedSlice->slice = slice;
				PointerList_Add(&mPositionedSlices, positionedSlice);
			}

			((PositionedListElementSlice*)sliceNode->ptr)->slice->SetPosition(sx, sy);
			sliceNode = sliceNode->next;
		}
		mLastY = y;
	}
	return TRUE;
}

int ListSlice::OnPenUp(void* context, int x, int y)
{
	if(y < mY) return FALSE;
	if(y >= (mY + mHeight)) return FALSE;
	PointerListEntry* sliceNode = mPositionedSlices;
	while (sliceNode != NULL)
	{
		((PositionedListElementSlice*)sliceNode->ptr)->slice->OnPenUp(context, x, y);
		sliceNode = sliceNode->next;
	}
	mPenDown = FALSE;
	return TRUE;
}

void ListSlice::Render(void* context)
{
	CleanupOAM();
	PointerListEntry* sliceNode;
	if(mAdapter->GetIsInvalidated())
	{
		//reload all slices
		sliceNode = mPositionedSlices;
		while (sliceNode != NULL)
		{
			int x, y;
			((PositionedListElementSlice*)sliceNode->ptr)->slice->GetPosition(x, y);
			int oamidx, vramoffset;
			if(mOAMChunkSize)
				oamidx = ((PositionedListElementSlice*)sliceNode->ptr)->slice->GetFirstOAMIdx();
			if(mVramChunkSize)
				vramoffset = ((PositionedListElementSlice*)sliceNode->ptr)->slice->GetOAMVramOffset();
			delete ((PositionedListElementSlice*)sliceNode->ptr)->slice;
			((PositionedListElementSlice*)sliceNode->ptr)->slice = mAdapter->GetSlice(((PositionedListElementSlice*)sliceNode->ptr)->position);
			if(mOAMChunkSize)
				((PositionedListElementSlice*)sliceNode->ptr)->slice->SetFirstOAMIdx(oamidx);
			if(mVramChunkSize)
				((PositionedListElementSlice*)sliceNode->ptr)->slice->SetOAMVramOffset(vramoffset);
			((PositionedListElementSlice*)sliceNode->ptr)->slice->SetPosition(x, y);
			((PositionedListElementSlice*)sliceNode->ptr)->slice->InitializeVram();
			sliceNode = sliceNode->next;
		}
		//and load more if the screen was not filled and more entries have been added
		int elementY = (mLastPositionedSlice + 1) * mAdapter->GetItemHeight() - mScrollY;
		int idx = mLastPositionedSlice + 1;
		int nrItems = mAdapter->GetItemCount();
		while(idx < nrItems && elementY < mHeight)
		{
			ListElementSlice* slice = mAdapter->GetSlice(idx);
			if(mOAMChunkSize)
			{
				int oam = mOAMIdx + FindUnusedOAMChunk() * mOAMChunkSize;
				slice->SetFirstOAMIdx(oam);
			}
			if(mVramChunkSize)
			{
				int vram = mOAMVramOffset + FindUnusedVramChunk() * mVramChunkSize;
				slice->SetOAMVramOffset(vram);
			}
			slice->SetPosition(0, mY + elementY);
			slice->InitializeVram();
			PositionedListElementSlice* positionedSlice = (PositionedListElementSlice*)malloc(sizeof(PositionedListElementSlice));
			positionedSlice->position = idx;
			positionedSlice->slice = slice;
			PointerList_Add(&mPositionedSlices, positionedSlice);
			idx++;
			elementY += mAdapter->GetItemHeight();
			mLastPositionedSlice++;
		}
	}
	sliceNode = mPositionedSlices;
	while (sliceNode != NULL)
	{
		if(!((PositionedListElementSlice*)sliceNode->ptr)->slice->IsVramInitialized())
			((PositionedListElementSlice*)sliceNode->ptr)->slice->InitializeVram();
		((PositionedListElementSlice*)sliceNode->ptr)->slice->Render(context);
		sliceNode = sliceNode->next;
	}
}

void ListSlice::CleanupVRAM()
{
	PointerListEntry* sliceNode = mPositionedSlices;
	while (sliceNode != NULL)
	{
		((PositionedListElementSlice*)sliceNode->ptr)->slice->CleanupVRAM();
		sliceNode = sliceNode->next;
	}
}