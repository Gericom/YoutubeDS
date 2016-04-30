#ifndef __LISTSLICE_H__
#define __LISTSLICE_H__
#include "UISlice.h"
#include "ListSliceAdapter.h"
#include "PointerList.h"

class ListSlice : public UISlice
{
private:
	struct PositionedListElementSlice
	{
		int position;
		ListElementSlice* slice;
	};

	int mHeight;
	ListSliceAdapter* mAdapter;
	PointerListEntry* mPositionedSlices;
	int mFirstPositionedSlice;
	int mLastPositionedSlice;
	int mScrollY;

	int mOAMIdx;
	//int mOAMCount;
	int mOAMVramOffset;
	//int mOAMVramSize;

	int mPenDown;
	int mLastY;

	//OAM cell allocation
	int mOAMChunkSize;
	int mNrOAMChunks;
	uint32_t* mOAMChunkUsageBitmask;
	uint32_t* mOAMChunkCleanupBitmask;

	int GetIsOAMChunkInUse(int idx)
	{
		if(idx >= mNrOAMChunks) return TRUE;//prevent usage of non-existant chunks
		return ((mOAMChunkUsageBitmask[idx >> 5] >> (idx & 0x1F)) & 1);
	}
	void SetIsOAMChunkInUse(int idx, int isInUse)
	{
		if(idx >= mNrOAMChunks) return;
		if(isInUse)
			mOAMChunkUsageBitmask[idx >> 5] |= 1 << (idx & 0x1F);
		else
		{
			mOAMChunkUsageBitmask[idx >> 5] &= ~(1 << (idx & 0x1F));
			mOAMChunkCleanupBitmask[idx >> 5] |= 1 << (idx & 0x1F);
		}
	}
	void CleanupOAM()
	{
		for(int i = 0; i < mNrOAMChunks; i++)
		{
			if(mOAMChunkCleanupBitmask[i >> 5])
			{
				if((mOAMChunkCleanupBitmask[i >> 5] >> (i & 0x1F)) & 1)
				{
					for(int j = 0; j < mOAMChunkSize; j++)
					{
						OAM_SUB[(mOAMIdx + i * mOAMChunkSize + j) * 4] = ATTR0_DISABLED;
					}
					mOAMChunkCleanupBitmask[i >> 5] &= ~(1 << (i & 0x1F));
				}
			}
			else i += 31;
		}
	}
	int FindUnusedOAMChunk()
	{
		int idx = 0;
		while(idx < mNrOAMChunks)
		{
			if(!GetIsOAMChunkInUse(idx))
			{
				SetIsOAMChunkInUse(idx, TRUE);
				return idx;
			}
			idx++;
		}
		return -1;
	}

	//VRAM allocation
	//divide the vram up into chunks of the size the adapter wants
	//then keep track of which chunk is used and which chunk is not
	int mVramChunkSize;
	int mNrVramChunks;
	uint32_t* mVramChunkUsageBitmask;

	int GetIsVramChunkInUse(int idx)
	{
		if(idx >= mNrVramChunks) return TRUE;//prevent usage of non-existant chunks
		return ((mVramChunkUsageBitmask[idx >> 5] >> (idx & 0x1F)) & 1);
	}
	void SetIsVramChunkInUse(int idx, int isInUse)
	{
		if(idx >= mNrVramChunks) return;
		if(isInUse)
			mVramChunkUsageBitmask[idx >> 5] |= 1 << (idx & 0x1F);
		else 
			mVramChunkUsageBitmask[idx >> 5] &= ~(1 << (idx & 0x1F));
	}
	int FindUnusedVramChunk()
	{
		int idx = 0;
		while(idx < mNrVramChunks)
		{
			if(!GetIsVramChunkInUse(idx))
			{
				SetIsVramChunkInUse(idx, TRUE);
				return idx;
			}
			idx++;
		}
		return -1;
	}
public:
	ListSlice(int y, int height, ListSliceAdapter* adapter, int oamIdx, int oamCount, int oamVramOffset, int oamVramSize);

	~ListSlice()
	{
		PointerListEntry* sliceNode = mPositionedSlices;
		while (sliceNode != NULL)
		{
			delete ((PositionedListElementSlice*)sliceNode->ptr)->slice;
			free(sliceNode->ptr);
			sliceNode = sliceNode->next;
		}
		PointerList_Clear(&mPositionedSlices);
		if(mOAMChunkUsageBitmask)
			free(mOAMChunkUsageBitmask);
		if(mOAMChunkCleanupBitmask)
			free(mOAMChunkCleanupBitmask);
		if(mVramChunkUsageBitmask)
			free(mVramChunkUsageBitmask);
	}

	int OnPenDown(void* context, int x, int y);
	int OnPenMove(void* context, int x, int y);
	int OnPenUp(void* context, int x, int y);

	void Render(void* context);
	void CleanupVRAM();
};

#endif