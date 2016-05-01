#include <nds.h>
#include <filesystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <dswifi9.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>  
#include "youtube.h"
#include "mpeg4.h"
#include "util.h"
#include "mpeg4_tables.h"
#include "aac_pub/aacdec.h"
#include "ringbufferhttpstream.h"
#include "copy.h"
#include "yuv2rgb_new.h"
#include "gui/font.h"
#include "gui/UIManager.h"
#include "gui/Toolbar.h"
#include "gui/ScreenKeyboard.h"
#include "gui/ListSlice.h"
#include "gui/PagingListSliceAdapter.h"
#include "gui/ProgressBar.h"
#include "print.h"

#define TMP_BUFFER_SIZE		(32 * 1024)

static uint8_t mVideoTmpBuffer[TMP_BUFFER_SIZE] __attribute__ ((aligned (32)));

static uint8_t mAudioTmpBuffer[TMP_BUFFER_SIZE] __attribute__ ((aligned (32)));

static mpeg4_dec_struct mpeg4DecStruct __attribute__ ((aligned (32)));

static uint8_t mYBufferA[256 * 144] __attribute__ ((aligned (32)));
static uint8_t mYBufferB[256 * 144] __attribute__ ((aligned (32)));
static uint8_t mUVBufferA[256 * 72] __attribute__ ((aligned (32)));
static uint8_t mUVBufferB[256 * 72] __attribute__ ((aligned (32)));

static int16_t mYDCCoefCache[256 / 8 * 144 / 8] __attribute__ ((aligned (32)));
static int16_t mUVDCCoefCache[256 / 8 * 72 / 8] __attribute__ ((aligned (32)));

static int16_t mMVecCache[(256 / 8 * 144 / 8) * 2] __attribute__ ((aligned (32))); //2 times, because there is both dx and dy

static uint8_t* mVideoHeader;

static volatile int mUpscalingEnabled = false;

#define AUDIO_BLOCK_SIZE	(1024)

#define NR_WAVE_DATA_BUFFERS	(256)//(32)

#define WAVE_DATA_BUFFER_LENGTH		(AUDIO_BLOCK_SIZE * NR_WAVE_DATA_BUFFERS)

static s16 mWaveData[WAVE_DATA_BUFFER_LENGTH] __attribute__ ((aligned (32)));

static int mWaveDataOffs_write = 0;

static bool hasAudioStarted = false;

//FrameQueue implementation
#define NR_FRAME_BLOCKS		(10)//(9)//(10)//(6)
static volatile int curBlock = -1;
static volatile int nrFramesInQueue = 0;
static volatile int firstQueueBlock = 0;//block to read from (most of the time (curBlock + 1) % 4)
static volatile int lastQueueBlock = 0;//block to write to (most of the time (firstQueueBlock + nrFramesInQueue) % 4)

static RingBufferHttpStream* mRingBufferHttpStream;

static volatile int nrFramesMissed = 0;
static int soundIdL = 0;
static int soundIdR = 0;

static volatile int stopVideo;
static volatile int isVideoPlaying;

static char* mStartVideoId;

class TestTextSlice : public ListElementSlice
{
	char* mText;
	char* mVideoId;
	int mTextInvalidated;
	int mPenDown;
	int mPenDownX;
	int mPenDownY;
	NTFT_FONT* mFont;
public:
	TestTextSlice(const char* text, const char* videoId, NTFT_FONT* font)
		: mText(NULL), mVideoId(NULL), mTextInvalidated(TRUE), mFont(font), mPenDown(FALSE)
	{
		SetText(text);
		mVideoId = Util_CopyString(videoId);
	}

	~TestTextSlice()
	{
		if(mText)
			free(mText);
		if(mVideoId)
			free(mVideoId);
	}

	int OnPenDown(void* context, int x, int y)
	{
		if(y < mY) return FALSE;
		if(y >= (mY + 64)) return FALSE;
		mPenDown = TRUE;
		mPenDownX = x;
		mPenDownY = y;
		return TRUE;
	}

	int OnPenMove(void* context, int x, int y)
	{
		if(y < mY) return FALSE;
		if(y >= (mY + 64)) return FALSE;
		return TRUE;
	}

	int OnPenUp(void* context, int x, int y)
	{
		if(y < mY) return FALSE;
		if(y >= (mY + 64)) return FALSE;
		if(mPenDown && abs(mPenDownX - x) < 16 && abs(mPenDownY - y) < 16)
		{
			//start the video
			stopVideo = TRUE;
			mStartVideoId = Util_CopyString(mVideoId);
		}
		mPenDown = FALSE;
		return TRUE;
	}

	void OnInitializeVram()
	{
		int w, h;
		Font_GetStringSize(mFont, mText, &w, &h);
		uint8_t* tmp = (uint8_t*)malloc(256 * 32);
		memset(tmp, 0, 256 * 32);
		Font_CreateStringData(mFont, mText, tmp + (16 - ((h + 1) >> 1)) * 256, 256);
		Util_ConvertToObj(tmp, 64, 32, 256, &SPRITE_GFX_SUB[mOAMVramOffset / 2]);
		Util_ConvertToObj(tmp + 64, 64, 32, 256, &SPRITE_GFX_SUB[mOAMVramOffset / 2 + 512]);
		Util_ConvertToObj(tmp + 2 * 64, 64, 32, 256, &SPRITE_GFX_SUB[mOAMVramOffset / 2 + 2 * 512]);
		Util_ConvertToObj(tmp + 3 * 64, 64, 32, 256, &SPRITE_GFX_SUB[mOAMVramOffset / 2 + 3 * 512]);
		free(tmp);
		mTextInvalidated = FALSE;
	}

	void Render(void* context)
	{
		OAM_SUB[mFirstOAMIdx * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_WIDE | OBJ_Y(mY);
		OAM_SUB[mFirstOAMIdx * 4 + 1] = ATTR1_SIZE_64 | OBJ_X(0);
		OAM_SUB[mFirstOAMIdx * 4 + 2] = ATTR2_PRIORITY(3) | ATTR2_PALETTE(5) | (mOAMVramOffset / 64);
		OAM_SUB[(mFirstOAMIdx + 1) * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_WIDE | OBJ_Y(mY);
		OAM_SUB[(mFirstOAMIdx + 1) * 4 + 1] = ATTR1_SIZE_64 | OBJ_X(64);
		OAM_SUB[(mFirstOAMIdx + 1) * 4 + 2] = ATTR2_PRIORITY(3) | ATTR2_PALETTE(5) | ((mOAMVramOffset / 64) + 16);
		OAM_SUB[(mFirstOAMIdx + 2) * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_WIDE | OBJ_Y(mY);
		OAM_SUB[(mFirstOAMIdx + 2) * 4 + 1] = ATTR1_SIZE_64 | OBJ_X(2 * 64);
		OAM_SUB[(mFirstOAMIdx + 2) * 4 + 2] = ATTR2_PRIORITY(3) | ATTR2_PALETTE(5) | ((mOAMVramOffset / 64) + 2 * 16);
		OAM_SUB[(mFirstOAMIdx + 3) * 4] = ATTR0_NORMAL | ATTR0_TYPE_NORMAL | ATTR0_COLOR_16 | ATTR0_WIDE | OBJ_Y(mY);
		OAM_SUB[(mFirstOAMIdx + 3) * 4 + 1] = ATTR1_SIZE_64 | OBJ_X(3 * 64);
		OAM_SUB[(mFirstOAMIdx + 3) * 4 + 2] = ATTR2_PRIORITY(3) | ATTR2_PALETTE(5) | ((mOAMVramOffset / 64) + 3 * 16);
	}

	void CleanupVRAM()
	{
		OAM_SUB[mFirstOAMIdx * 4] = ATTR0_DISABLED;
		OAM_SUB[(mFirstOAMIdx + 1) * 4] = ATTR0_DISABLED;
		OAM_SUB[(mFirstOAMIdx + 2) * 4] = ATTR0_DISABLED;
		OAM_SUB[(mFirstOAMIdx + 3) * 4] = ATTR0_DISABLED;
	}

	void SetText(const char* text)
	{
		if(mText != NULL)
			free(mText);
		if(text == NULL)
			mText = Util_CopyString("");
		else 
			mText = Util_CopyString(text);
		mTextInvalidated = TRUE;
	}
};

class TestLoadingSlice : public ListElementSlice
{
private:
	ProgressBar* mProgressBar;
public:
	TestLoadingSlice()
		: mProgressBar(NULL)
	{ }

	~TestLoadingSlice()
	{
		if(mProgressBar)
			delete mProgressBar;
	}

	int OnPenDown(void* context, int x, int y)
	{
		if(y < mY) return FALSE;
		if(y >= (mY + 64)) return FALSE;
		return TRUE;
	}

	int OnPenMove(void* context, int x, int y)
	{
		if(y < mY) return FALSE;
		if(y >= (mY + 64)) return FALSE;
		return TRUE;
	}

	int OnPenUp(void* context, int x, int y)
	{
		if(y < mY) return FALSE;
		if(y >= (mY + 64)) return FALSE;
		return TRUE;
	}

	void OnInitializeVram()
	{
		if(mProgressBar)
			delete mProgressBar;
		mProgressBar = new ProgressBar(mFirstOAMIdx);
	}

	void Render(void* context)
	{
		mProgressBar->SetPosition(mX + 112, mY);
		mProgressBar->Render(context);
	}

	void CleanupVRAM()
	{
		if(mProgressBar)
			mProgressBar->CleanupVRAM();
	}
};

class TestAdapter : public PagingListSliceAdapter
{
private:
	NTFT_FONT* mFont;
	happyhttp::Connection* mConnection;
	int mCurrentlyRequestingPage;
	int mRequestInitialized;
	char* mRequestURL;
	char* mSearchQuery;
	uint8_t* mResponse;
	uint8_t* mpResponse;
public:
	static void OnHttpConnectionData(const happyhttp::Response* r, void* userdata, const unsigned char* data, int n)
	{
		((TestAdapter*)userdata)->OnHttpConnectionData(r, data, n);
	}

	void OnHttpConnectionData(const happyhttp::Response* r, const unsigned char* data, int n)
	{
		memcpy(mpResponse, data, n);
		mpResponse += n;
	}

	static void OnHttpConnectionComplete(const happyhttp::Response* r, void* userdata)
	{
		((TestAdapter*)userdata)->OnHttpConnectionComplete(r);
	}

	void OnHttpConnectionComplete(const happyhttp::Response* r)
	{
		mpResponse[0] = 0;
		YT_SearchListResponse* pageData = YT_Search_ParseResponse((char*)mResponse);
		free(mResponse);
		mResponse = NULL;
		mpResponse = NULL;
		SetPageData(mCurrentlyRequestingPage, pageData);
		mCurrentlyRequestingPage = -1;
		mRequestInitialized = FALSE;
	}

	TestAdapter(NTFT_FONT* font, const char* searchQuery)
		: PagingListSliceAdapter(64, 4, 4096, 10), mFont(font), mConnection(NULL), mCurrentlyRequestingPage(-1), mRequestInitialized(FALSE), mRequestURL(NULL),
			mSearchQuery(NULL), mResponse(NULL), mpResponse(NULL)
	{
		mSearchQuery = Util_CopyString(searchQuery);
	}

	~TestAdapter()
	{
		if(mPageDatas[0])
			OnFreePageData(mPageDatas[0]);
		if(mPageDatas[1])
			OnFreePageData(mPageDatas[1]);
		if(mPageDatas[2])
			OnFreePageData(mPageDatas[2]);
		if(mSearchQuery)
			free(mSearchQuery);
		if(mResponse)
			free(mResponse);
		if(mConnection)
			delete mConnection;
	}

	ListElementSlice* GetPageSlice(void* pageData, int index)
	{
		return new TestTextSlice(((YT_SearchListResponse*)pageData)->searchResults[index].title, ((YT_SearchListResponse*)pageData)->searchResults[index].videoId, mFont);
	}

	ListElementSlice* GetLoadingSlice()
	{
		return new TestLoadingSlice();
	}

	void OnRequestPageData(int page)
	{
		if(mCurrentlyRequestingPage >= 0) return;
		char* pagetoken = NULL;
		if(page)//!(mCurPage == 0 && page == 0 && mPageDatas[2] == NULL))
		{
			if(page == mCurPage && mPageDatas[0])
			{
				if(!((YT_SearchListResponse*)mPageDatas[0])->nextPageToken) return;
				pagetoken = ((YT_SearchListResponse*)mPageDatas[0])->nextPageToken;
			}
			else if(page == mCurPage && mPageDatas[2])
			{
				if(!((YT_SearchListResponse*)mPageDatas[2])->prevPageToken) return;
				pagetoken = ((YT_SearchListResponse*)mPageDatas[2])->prevPageToken;
			}
			else if(page == mCurPage - 1 && mPageDatas[1])
			{
				if(!((YT_SearchListResponse*)mPageDatas[1])->prevPageToken) return;
				pagetoken = ((YT_SearchListResponse*)mPageDatas[1])->prevPageToken;
			}
			else if(page == mCurPage + 1 && mPageDatas[1])
			{
				if(!((YT_SearchListResponse*)mPageDatas[1])->nextPageToken) return;
				pagetoken = ((YT_SearchListResponse*)mPageDatas[1])->nextPageToken;
			}
			else return;
		}
		mCurrentlyRequestingPage = page;
		mRequestURL = YT_Search_GetURL(mSearchQuery, 10, pagetoken);
		mResponse = (uint8_t*)malloc(32 * 1024);
		mpResponse = mResponse;
		mRequestInitialized = FALSE;
	}

	void OnFreePageData(void* pageData)
	{
		YT_FreeSearchListResponse((YT_SearchListResponse*)pageData);
	}

	void DoFrameProc()
	{
		if(mCurrentlyRequestingPage >= 0 && !mRequestInitialized)
		{
			if(mConnection)
				delete mConnection;
			mConnection = new happyhttp::Connection("florian.nouwt.com", 80);
			mConnection->setcallbacks(NULL, TestAdapter::OnHttpConnectionData, TestAdapter::OnHttpConnectionComplete, this);
			mConnection->request("GET", mRequestURL);
			free(mRequestURL);
			mRequestURL = NULL;
			mRequestInitialized = TRUE;
		}
		if(mConnection && mConnection->outstanding())
			mConnection->pump();
	}
};

static NTFT_FONT* roboto_medium_13;
static NTFT_FONT* roboto_regular_10;
static UIManager* mUIManager;
static Toolbar* mToolbar;
static ScreenKeyboard* mKeyboard;
static TestAdapter* mTestAdapter;
static ListSlice* mList;

#define FRAME_SIZE	(176 * 144)
static uint16_t mFrameQueue[FRAME_SIZE * NR_FRAME_BLOCKS] __attribute__ ((aligned (32)));

static volatile int mShouldCopyFrame;

ITCM_CODE static void frameHandler()
{
	mShouldCopyFrame = TRUE;
}

static BG23AffineInfo mLineAffineInfoOriginal[192] __attribute__ ((aligned (32)));
static BG23AffineInfo mLineAffineInfoUpscaled[192] __attribute__ ((aligned (32)));

#define USE_WIFI

void PlayVideo()
{
	stopVideo = FALSE;
	curBlock = -1;
	nrFramesInQueue = 0;
	firstQueueBlock = 0;
	lastQueueBlock = 0;
	mWaveDataOffs_write = 0;
	hasAudioStarted = false;
	mShouldCopyFrame = FALSE;
#ifdef USE_WIFI
	char* url = YT_GetVideoInfo(mStartVideoId);
	//mStartVideoId is always copied using Util_CopyString, so free it again
	free(mStartVideoId);
	mStartVideoId = NULL;
	if(url == NULL) return;
	mRingBufferHttpStream = new RingBufferHttpStream(url);
	free(url);
	//Skip ftyp box
	uint32_t ftyp_size;
	mRingBufferHttpStream->Read((uint8_t*)&ftyp_size, 4);
	mRingBufferHttpStream->Read(NULL, SWAP_CONSTANT_32(ftyp_size) - 4);
	//Read full header size
	uint32_t header_size;
	mRingBufferHttpStream->Read((uint8_t*)&header_size, 4);
	mVideoHeader = (uint8_t*)malloc(SWAP_CONSTANT_32(header_size));
	mRingBufferHttpStream->Read(mVideoHeader + 4, SWAP_CONSTANT_32(header_size) - 4);
#else
	FILE* video = fopen("/videoplayback.3gp", "rb");
	//Skip ftyp box
	uint32_t ftyp_size;
	fread(&ftyp_size, 4, 1, video);
	fseek(video, SWAP_CONSTANT_32(ftyp_size) - 4, SEEK_CUR);
	//Read full header size
	uint32_t header_size;
	fread(&header_size, 4, 1, video);
	mVideoHeader = (uint8_t*)malloc(SWAP_CONSTANT_32(header_size));
	fseek(video, -4, SEEK_CUR);
	//Read in the header
	fread(mVideoHeader, 1, SWAP_CONSTANT_32(header_size), video);
#endif
	memset(&mpeg4DecStruct, 0, sizeof(mpeg4DecStruct));
	mpeg4DecStruct.pData = &mVideoTmpBuffer[0];
	mpeg4DecStruct.width = 176;
	mpeg4DecStruct.height = 144;
	mpeg4DecStruct.pDstY = &mYBufferA[0];
	mpeg4DecStruct.pDstUV = &mUVBufferA[0];
	mpeg4DecStruct.pIntraDCTVLCTable = &mpeg4_table_b16[0];
	mpeg4DecStruct.pInterDCTVLCTable = &mpeg4_table_b17[0];
	mpeg4DecStruct.pdc_coef_cache_y = &mYDCCoefCache[0];
	mpeg4DecStruct.pdc_coef_cache_uv = &mUVDCCoefCache[0];
	mpeg4DecStruct.pvector_cache = &mMVecCache[0];
	//mpeg4DecStruct.vop_time_increment_bits =4; //10;

	//find the header data we need
	uint8_t* pHeader = mVideoHeader;
	pHeader += 8;	//skip moov
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip mvhd
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip iods
	//We're at the video track now
	pHeader += 8;	//skip trak
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip tkhd
	pHeader += 8;	//skip mdia
	uint32_t timescale = READ_SAFE_UINT32_BE(pHeader + 0x14);
	mpeg4DecStruct.vop_time_increment_bits = 32 - __builtin_clz(timescale);
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip mdhd
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip hdlr
	pHeader += 8;	//skip minf
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip vmhd
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip dinf
	pHeader += 8;	//skip stbl
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip stsd
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip stts
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip stss
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip stsc
	pHeader += 8;	//skip stsz
	pHeader += 8;
	int nrframes = READ_SAFE_UINT32_BE(pHeader);
	pHeader += 4;
	uint8_t* framesizes = pHeader;
	pHeader += nrframes * 4;
	uint8_t* videoBlockOffsets = pHeader + 16;
	pHeader += READ_SAFE_UINT32_BE(pHeader);
	//We're at the audio track now
	pHeader += 8;	//skip trak
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip tkhd
	pHeader += 8;	//skip mdia
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip mdhd
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip hdlr
	pHeader += 8;	//skip minf
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip smhd
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip dinf
	pHeader += 8;	//skip stbl
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip stsd
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip stts
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip stsc
	pHeader += READ_SAFE_UINT32_BE(pHeader);//skip stsz
	uint8_t* audioBlockOffsets = pHeader + 16;

	uint32_t offset = READ_SAFE_UINT32_BE(videoBlockOffsets);
	videoBlockOffsets += 4;
#ifdef USE_WIFI
	mRingBufferHttpStream->Read(NULL, offset - mRingBufferHttpStream->GetStreamPosition());
#else
	fseek(video, offset, SEEK_SET);
#endif
	uint32_t nextAudioBlockOffset = READ_SAFE_UINT32_BE(audioBlockOffsets);
	audioBlockOffsets += 4;

	memset(&mMVecCache[0], 0, sizeof(mMVecCache));
	videoSetMode(MODE_5_2D);
	vramSetBankE(VRAM_E_MAIN_BG);
	dmaFillWords(0, (void*)0x06000000, 64 * 1024);
	bgInit(2, BgType_Bmp8, BgSize_B16_256x256, 0,0);
	bgInit(3, BgType_Bmp8, BgSize_B16_256x256, 0,0);
	REG_DISPCNT &= ~(DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE | DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE);
	if(!mUpscalingEnabled)
	{
		WIN0_X0 = 40;
		WIN0_X1 = 40 + 176;
		WIN0_Y0 = 24;
		WIN0_Y1 = 24 + 144;
		WIN_IN = (1 << 3) | (1 << 2);
		REG_DISPCNT |= DISPLAY_WIN0_ON | DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE;
	}
	else
		REG_DISPCNT |= DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE;

	HAACDecoder aacDec = AACInitDecoder();
	AACFrameInfo frameInfo;
	frameInfo.bitRate = 0;
	frameInfo.nChans = 1;
	frameInfo.sampRateCore = 22050;
	frameInfo.sampRateOut = 22050;
	frameInfo.bitsPerSample = 16;
	frameInfo.outputSamps = 0;
	frameInfo.profile = 0;
	frameInfo.tnsUsed = 0;
	frameInfo.pnsUsed = 0;
	AACSetRawBlockParams(aacDec, 0, &frameInfo);
	stopVideo = FALSE;
	isVideoPlaying = TRUE;
	int frame = 0;
	if(timescale == 12)
		timerStart(0, ClockDivider_1024, TIMER_FREQ_1024(12), frameHandler);
	else if(timescale == 8333)
		timerStart(0, ClockDivider_1024, /*TIMER_FREQ_1024(8)*/-3928, frameHandler);
	else if(timescale == 6)
		timerStart(0, ClockDivider_1024, TIMER_FREQ_1024(6), frameHandler);
	else 
		timerStart(0, ClockDivider_1024, TIMER_FREQ_1024(10), frameHandler);
	int keytimer = 0;
	while((!stopVideo || frame < 20) && frame < nrframes)
	{
		uint32_t size = READ_SAFE_UINT32_BE(framesizes);
		framesizes += 4;
#ifdef USE_WIFI
		mRingBufferHttpStream->Read(&mVideoTmpBuffer[0], size);
#else
		fread(&mVideoTmpBuffer[0], 1, size, video);
#endif
		offset += size;
		mpeg4DecStruct.pData = &mVideoTmpBuffer[0];
		if(frame & 1)
		{
			mpeg4DecStruct.pDstY = &mYBufferB[0];
			mpeg4DecStruct.pDstUV = &mUVBufferB[0];
			mpeg4DecStruct.pPrevY = &mYBufferA[0];
			mpeg4DecStruct.pPrevUV = &mUVBufferA[0];
		}
		else
		{
			mpeg4DecStruct.pDstY = &mYBufferA[0];
			mpeg4DecStruct.pDstUV = &mUVBufferA[0];
			mpeg4DecStruct.pPrevY = &mYBufferB[0];
			mpeg4DecStruct.pPrevUV = &mUVBufferB[0];
		}
		for(int q = 0; q < sizeof(mYDCCoefCache) / sizeof(mYDCCoefCache[0]); q++)
		{
			mYDCCoefCache[q] = 1024;
		}
		for(int q = 0; q < sizeof(mUVDCCoefCache) / sizeof(mUVDCCoefCache[0]); q++)
		{
			mUVDCCoefCache[q] = 1024;
		}
		for(int q = 0; q < sizeof(mMVecCache) / sizeof(mMVecCache[0]); q++)
		{
			mMVecCache[q] = 0;
		}
		if(mpeg4DecStruct.pData[2] == 1 && mpeg4DecStruct.pData[3] == 0xB3)
			mpeg4DecStruct.pData += 7;
		mpeg4DecStruct.pData += 4;//skip 000001B6
		//cpuStartTiming(2);
		mpeg4_VideoObjectPlane(&mpeg4DecStruct);
		//uint32_t time = cpuEndTiming();
		//char tmp[21];
		//memset(tmp, 0, sizeof(tmp));
		//sprintf(tmp,"0x%x",time);
		//mToolbar->SetTitle(tmp);
		frame++;
		if(offset == nextAudioBlockOffset)
		{
			offset = READ_SAFE_UINT32_BE(videoBlockOffsets);
			int audiosize = offset - nextAudioBlockOffset;
			videoBlockOffsets += 4;
			uint8_t* audioData = &mAudioTmpBuffer[0];
			uint8_t* audioDataptr = audioData;
#ifdef USE_WIFI
			mRingBufferHttpStream->Read(audioData, audiosize);
#else
			fread(audioData, 1, audiosize, video);
#endif
			while(audiosize > 0)
			{
				int err = 0;
				if((err = AACDecode(aacDec, &audioDataptr, &audiosize, &mWaveData[mWaveDataOffs_write * AUDIO_BLOCK_SIZE])) < 0) 
				{
					break;
				}
				DC_FlushRange(&mWaveData[mWaveDataOffs_write * AUDIO_BLOCK_SIZE], AUDIO_BLOCK_SIZE * 2);
				mWaveDataOffs_write = (mWaveDataOffs_write + 1) % NR_WAVE_DATA_BUFFERS;
			}
			if(!hasAudioStarted)
			{
				soundIdL = soundPlaySample(&mWaveData[0], SoundFormat_16Bit, sizeof(mWaveData), 22050, 127, 0, true, 0);
				soundIdR = soundPlaySample(&mWaveData[0], SoundFormat_16Bit, sizeof(mWaveData), 22050, 127, 127, true, 0);
				hasAudioStarted = true;
			}
			nextAudioBlockOffset = READ_SAFE_UINT32_BE(audioBlockOffsets);
			audioBlockOffsets += 4;
		}
		while(nrFramesInQueue >= NR_FRAME_BLOCKS || lastQueueBlock == curBlock);
		//{
		//	if(stopVideo) goto video_stop;
		//}
		//cpuStartTiming(2);
		yuv2rgb_new(mpeg4DecStruct.pDstY, mpeg4DecStruct.pDstUV, &mFrameQueue[lastQueueBlock * FRAME_SIZE]);//(uint16_t*)frameOffsets[lastQueueBlock]);
		DC_FlushRange(&mFrameQueue[lastQueueBlock * FRAME_SIZE], FRAME_SIZE * 2);
		//uint32_t time = cpuEndTiming();
		//char tmp[21];
		//memset(tmp, 0, sizeof(tmp));
		//sprintf(tmp,"0x%x",time);
		//mToolbar->SetTitle(tmp);
		//nocashPrint1("Time: %r0%\n", time);

		lastQueueBlock = (lastQueueBlock + 1) % NR_FRAME_BLOCKS;
		nrFramesInQueue++;
		//asm("mov r11, r11");
		//keep network traffic from the gui working
		if(mTestAdapter)
			mTestAdapter->DoFrameProc();
		if(frame < 20) stopVideo = FALSE;
	}
video_stop:
	timerStop(0);
	REG_DISPCNT &= ~(DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE);
	DMA0_CR = 0;
	isVideoPlaying = FALSE;
	if(hasAudioStarted)
	{
		soundKill(soundIdL);
		soundKill(soundIdR);
		hasAudioStarted = FALSE;
	}
	AACFreeDecoder(aacDec);
#ifdef USE_WIFI
	delete mRingBufferHttpStream;
#else
	fclose(video);
#endif
	free(mVideoHeader);
}

static int state = 0;
static int state_counter = 0;

static std::string mSearchString;
static int mSearchStringInvalidated;
static int mSearchStringCursorPosition;

static void OnToolbarButtonClick(void* context, int button)
{
	if(state == 0 && button == TOOLBAR_BUTTON_SEARCH)
	{
		state = 1;
		state_counter = 0;
	}
	else if((state == 2 || state == 5) && button == TOOLBAR_BUTTON_BACK)
	{
		state = 3;
		state_counter = 0;
	}
	else if(state == 2 && button == TOOLBAR_BUTTON_CLEAR)
	{
		mSearchStringCursorPosition = 0;
		mSearchString.erase();
		mSearchStringInvalidated = TRUE;
	}
}

static void OnKeyboardButtonClick(void* context, int button)
{
	if(button & (1 << 16)) //special
	{
		if(button == SCREENKEYBOARD_BUTTON_SPECIAL_BACKSPACE && mSearchStringCursorPosition > 0)
		{
			mSearchStringCursorPosition--;
			mSearchString.erase(mSearchStringCursorPosition, 1);
			mSearchStringInvalidated = TRUE;
		}
		else if(button == SCREENKEYBOARD_BUTTON_SPECIAL_SEARCH)
		{
			if(mSearchString.length() > 0 && mSearchString.find_first_not_of(' ') != std::string::npos)
			{
				state = 4;
				state_counter = 0;
			}
		}
		return;
	}
	mSearchString.insert(mSearchStringCursorPosition, 1, (char)button);
	mSearchStringCursorPosition++;
	mSearchStringInvalidated = TRUE;
}

static int mTextCursorBlinking;
static int mTextCursorCounter;
static int mKeyTimer = 0;

void VBlankProc()
{
	scanKeys();
	int held = keysHeld();
	if(isVideoPlaying)//stride dma and frame copy
	{
		DMA0_CR = 0;
		if(mShouldCopyFrame)
		{
			mShouldCopyFrame = FALSE;
			if(nrFramesInQueue > 0)
			{
				dmaCopyWordsAsynch(2, &mFrameQueue[firstQueueBlock * FRAME_SIZE], (void*)&BG_GFX[0], FRAME_SIZE * 2);//REG_BG2PA, sizeof(BG23AffineInfo));
				curBlock = firstQueueBlock;
				firstQueueBlock = (firstQueueBlock + 1) % NR_FRAME_BLOCKS;
				nrFramesInQueue--;
			}
		}
		if(!mKeyTimer)
		{
			if(held & KEY_SELECT)//toggle upscaling
			{
				mUpscalingEnabled = !mUpscalingEnabled;
				if(!mUpscalingEnabled)
				{
					WIN0_X0 = 40;
					WIN0_X1 = 40 + 176;
					WIN0_Y0 = 24;
					WIN0_Y1 = 24 + 144;
					WIN_IN = (1 << 3) | (1 << 2);
					REG_DISPCNT |= DISPLAY_WIN0_ON;
				}
				else
					REG_DISPCNT &= ~DISPLAY_WIN0_ON;
				mKeyTimer = 20;
			}
		}
		else mKeyTimer--;
		if(mUpscalingEnabled)
		{
			REG_BG2PA = mLineAffineInfoUpscaled[0].BG2PA;
			REG_BG2PB = mLineAffineInfoUpscaled[0].BG2PB;
			REG_BG2PC = mLineAffineInfoUpscaled[0].BG2PC;
			REG_BG2PD = mLineAffineInfoUpscaled[0].BG2PD;
			REG_BG2X = mLineAffineInfoUpscaled[0].BG2X;
			REG_BG2Y = mLineAffineInfoUpscaled[0].BG2Y;
			REG_BG3PA = mLineAffineInfoUpscaled[0].BG3PA;
			REG_BG3PB = mLineAffineInfoUpscaled[0].BG3PB;
			REG_BG3PC = mLineAffineInfoUpscaled[0].BG3PC;
			REG_BG3PD = mLineAffineInfoUpscaled[0].BG3PD;
			REG_BG3X = mLineAffineInfoUpscaled[0].BG3X;
			REG_BG3Y = mLineAffineInfoUpscaled[0].BG3Y;
			DMA0_SRC = (uint32_t)&mLineAffineInfoUpscaled[1];
		}
		else
		{
			REG_BG2PA = mLineAffineInfoOriginal[0].BG2PA;
			REG_BG2PB = mLineAffineInfoOriginal[0].BG2PB;
			REG_BG2PC = mLineAffineInfoOriginal[0].BG2PC;
			REG_BG2PD = mLineAffineInfoOriginal[0].BG2PD;
			REG_BG2X = mLineAffineInfoOriginal[0].BG2X;
			REG_BG2Y = mLineAffineInfoOriginal[0].BG2Y;
			REG_BG3PA = mLineAffineInfoOriginal[0].BG3PA;
			REG_BG3PB = mLineAffineInfoOriginal[0].BG3PB;
			REG_BG3PC = mLineAffineInfoOriginal[0].BG3PC;
			REG_BG3PD = mLineAffineInfoOriginal[0].BG3PD;
			REG_BG3X = mLineAffineInfoOriginal[0].BG3X;
			REG_BG3Y = mLineAffineInfoOriginal[0].BG3Y;
			DMA0_SRC = (uint32_t)&mLineAffineInfoOriginal[1];
		}
		DMA0_DEST = (uint32_t)&REG_BG2PA;
		DMA0_CR = DMA_ENABLE | DMA_START_HBL | DMA_32_BIT | DMA_REPEAT | DMA_SRC_INC | DMA_DST_RESET | (sizeof(BG23AffineInfo) >> 2);
	}
	else
		mKeyTimer = 0;
	switch(state)
	{
	case 0:
		break;
	case 1:
		{
			if(state_counter == 0)
			{
				mToolbar->SetShowBackButton(TRUE);
				mToolbar->SetShowClearButton(FALSE);
				mToolbar->SetShowSearchButton(FALSE);
				mToolbar->SetShowMenuButton(FALSE);
				mToolbar->SetCursorX(0);
				mToolbar->SetShowCursor(TRUE);
				mTextCursorCounter = 0;
				mTextCursorBlinking = TRUE;
				mToolbar->SetTitle("Search YouTube");
				mSearchString = std::string("");
				mSearchStringInvalidated = TRUE;
				mSearchStringCursorPosition = 0;
				mKeyboard->Show();
			}
			int rbg = 28 + ((30 - 28) * state_counter) / 9;
			int gbg = 4 + ((30 - 4) * state_counter) / 9;
			int bbg = 3 + ((30 - 3) * state_counter) / 9;
			mToolbar->SetBGColor(RGB5(rbg,gbg,bbg));
			int ricon = 31 + ((14 - 31) * state_counter) / 9;
			int gicon = 31 + ((14 - 31) * state_counter) / 9;
			int bicon = 31 + ((14 - 31) * state_counter) / 9;
			mToolbar->SetIconColor(RGB5(ricon,gicon,bicon));
			mToolbar->SetTextColor(RGB5(ricon,gicon,bicon));

			state_counter++;
			if(state_counter == 10)
			{
				state = 2;
				state_counter = 0;
			}
		}
		break;
	case 2:
		if(mSearchStringInvalidated)
		{
			mSearchStringInvalidated = FALSE;
			if(mSearchString.length() > 0)
			{
				mToolbar->SetShowClearButton(TRUE);
				mToolbar->SetTextColor(RGB5(7,7,7));
				mToolbar->SetTitle(mSearchString.c_str());
				//update cursor
				std::string part = mSearchString.substr(0, mSearchStringCursorPosition);
				int w, h;
				Font_GetStringSize(roboto_medium_13, part.c_str(), &w, &h);
				mToolbar->SetCursorX(w);
			}
			else
			{
				mToolbar->SetCursorX(0);
				mToolbar->SetShowClearButton(FALSE);
				mToolbar->SetTextColor(RGB5(14,14,14));
				mToolbar->SetTitle("Search YouTube");
			}
		}
		if(held & KEY_B)
		{
			state = 3;
			state_counter = 0;
		}
		break;
	case 3:
		{
			if(state_counter == 0)
			{
				mToolbar->SetShowBackButton(FALSE);
				mToolbar->SetShowClearButton(FALSE);
				mToolbar->SetShowSearchButton(TRUE);
				mToolbar->SetShowMenuButton(TRUE);
				mToolbar->SetShowCursor(FALSE);
				mToolbar->SetTitle("YouTube");
				mTextCursorBlinking = FALSE;
				mTextCursorCounter = 0;
				mKeyboard->Hide();
				if(mList)
				{
					mUIManager->RemoveSlice(mList);
					mList->CleanupVRAM();
					delete mList;
					mList = NULL;
				}
				if(mTestAdapter)
				{
					delete mTestAdapter;
					mTestAdapter = NULL;
				}
			}
			int rbg = 30 + ((28 - 30) * state_counter) / 9;
			int gbg = 30 + ((4 - 30) * state_counter) / 9;
			int bbg = 30 + ((3 - 30) * state_counter) / 9;
			mToolbar->SetBGColor(RGB5(rbg,gbg,bbg));
			int ricon = 14 + ((31 - 14) * state_counter) / 9;
			int gicon = 14 + ((31 - 14) * state_counter) / 9;
			int bicon = 14 + ((31 - 14) * state_counter) / 9;
			mToolbar->SetIconColor(RGB5(ricon,gicon,bicon));
			if(mSearchString.length() > 0)
			{
				int rtext = 7 + ((31 - 7) * state_counter) / 9;
				int gtext = 7 + ((31 - 7) * state_counter) / 9;
				int btext = 7 + ((31 - 7) * state_counter) / 9;
				mToolbar->SetTextColor(RGB5(rtext,gtext,btext));
			}
			else 
				mToolbar->SetTextColor(RGB5(ricon,gicon,bicon));

			state_counter++;
			if(state_counter == 10)
			{
				state = 0;
				state_counter = 0;
			}
		}
		break;
	case 4:
		{
			if(state_counter == 0)
			{
				mToolbar->SetShowCursor(FALSE);
				mTextCursorBlinking = FALSE;
				mTextCursorCounter = 0;
				mKeyboard->Hide();
				mTestAdapter = new TestAdapter(roboto_regular_10, mSearchString.c_str());
				mList = new ListSlice(36, 192 - 36, mTestAdapter, 15 + 1, 128 - (15 + 1), 4608 + 19456, (64 * 1024) - (4608 + 19456));
				mUIManager->AddSlice(mList);
				state_counter++;
			}
			//mTestAdapter->DoFrameProc();
			if(mKeyboard->IsHidden())
			{
				state = 5;
				state_counter = 0;
			}
		}
		break;
	case 5:
		{
			//mTestAdapter->DoFrameProc();
		}
		break;
	}
	if(mTextCursorBlinking)
	{
		mTextCursorCounter++;
		if(mTextCursorCounter == 30)
			mToolbar->SetShowCursor(FALSE);
		else if(mTextCursorCounter == 60)
		{
			mToolbar->SetShowCursor(TRUE);
			mTextCursorCounter = 0;
		}
	}
	mUIManager->ProcessInput();
	mUIManager->Render();
}

int main()
{
	mStartVideoId = NULL;
	isVideoPlaying = FALSE;
	mUpscalingEnabled = FALSE;
	defaultExceptionHandler();
	nitroFSInit(NULL);
	soundEnable();
	//make the bottom screen white and fade in a nice logo at the top screen
	setBrightness(1, 16);
	setBrightness(2, 16);
	videoSetMode(MODE_0_2D);
	uint8_t* load_tmp = (uint8_t*)malloc(6080);
	FILE* file = fopen("/Logo.nbfp", "rb");
	fread(&load_tmp[0], 1, 512, file);
	fclose(file);
	for(int i = 0; i < 256; i++)
	{
		BG_PALETTE[i] = load_tmp[i*2]|(load_tmp[i*2+1] << 8);
	}
	BG_PALETTE[0] = 0;
	vramSetBankE(VRAM_E_MAIN_BG);
	vramSetBankA(VRAM_A_MAIN_BG_0x06020000);
	vramSetBankB(VRAM_B_MAIN_BG_0x06040000);
	vramSetBankC(VRAM_C_MAIN_BG_0x06060000);
	vramSetBankF(VRAM_F_MAIN_SPRITE_0x06400000);
	vramSetBankG(VRAM_G_MAIN_SPRITE_0x06404000);
	file = fopen("/Logo.nbfc", "rb");
	fread(&load_tmp[0], 1, 6080, file);
	fclose(file);
	DC_FlushRange(&load_tmp[0], 4416);
	dmaCopyWords(3, load_tmp, &BG_GFX[0], 6080);
	file = fopen("/Logo.nbfs", "rb");
	fread(&load_tmp[0], 1, 2048, file);
	fclose(file);
	DC_FlushRange(&load_tmp[0], 2048);
	dmaCopyWords(3, load_tmp, &BG_GFX[6144 >> 1], 2048);
	bgInit(0, BgType_Text8bpp, BgSize_T_256x256, 3, 0);
	REG_DISPCNT |= DISPLAY_BG0_ACTIVE;
	int bright = 32;
	while(bright > 0)
	{
		bright--;
		setBrightness(1, (bright + 1) / 2);
		swiWaitForVBlank();
	}
#ifdef USE_WIFI
	Wifi_InitDefault(WFC_CONNECT);
#endif
	Util_SetupStrideFixAffine(&mLineAffineInfoOriginal[0], 176, 256, 40, 24, 256, 256);
	Util_SetupStrideFixAffine(&mLineAffineInfoUpscaled[0], 176, 256, 0, -5, 176, 176);
	//Toolbar test
	vramSetBankH(VRAM_H_SUB_BG);
	vramSetBankI(VRAM_I_SUB_BG_0x06208000);
	vramSetBankD(VRAM_D_SUB_SPRITE);
	//Load toolbar shadow
	file = fopen("/Menu/Shadow.nbfp", "rb");
	fread(&load_tmp[0], 1, 32, file);
	fclose(file);
	for(int i = 0; i < 16; i++)
	{
		BG_PALETTE_SUB[i] = load_tmp[i*2]|(load_tmp[i*2+1] << 8);
	}
	file = fopen("/Menu/Shadow.nbfc", "rb");
	fread(&load_tmp[0], 1, 64, file);
	fclose(file);
	for(int i = 0; i < (64 >> 1); i++)
	{
		BG_GFX_SUB[i] = load_tmp[i*2]|(load_tmp[i*2+1] << 8);
	}
	file = fopen("/Menu/Shadow.nbfs", "rb");
	fread(&load_tmp[0], 1, 2048, file);
	fclose(file);
	for(int i = 0; i < 2048 / 2; i++)
	{
		BG_GFX_SUB[(0x3800 >> 1) + i] = load_tmp[i*2]|(load_tmp[i*2+1] << 8);
	}
	free(load_tmp);
	REG_BG1CNT_SUB = BG_32x32 | BG_PRIORITY_2 | BG_COLOR_16 | BG_MAP_BASE(7);
	REG_BLDCNT_SUB = BLEND_ALPHA | BLEND_SRC_BG1 | BLEND_DST_BG0 | BLEND_DST_SPRITE | BLEND_DST_BACKDROP;
	REG_BLDALPHA_SUB = 3 | (13 << 8);

	roboto_medium_13 = Font_Load("/Fonts/RobotoMedium13.nft");
	roboto_regular_10 = Font_Load("/Fonts/RobotoRegular10.nft");
	for(int i = 0; i < 512; i++)
	{
		if(!(i & 3))
			OAM_SUB[i] = ATTR0_DISABLED;
		else OAM_SUB[i] = 0;
	}
	BG_PALETTE_SUB[0] = RGB5(31,31,31);
	mUIManager = new UIManager(NULL);
	mToolbar = new Toolbar(RGB5(28,4,3), RGB5(31,31,31), RGB5(31,31,31), "YouTube", roboto_medium_13);
	mToolbar->SetShowSearchButton(TRUE);
	mToolbar->SetOnButtonClickCallback(OnToolbarButtonClick);
	mToolbar->Initialize();
	mUIManager->AddSlice(mToolbar);
	mKeyboard = new ScreenKeyboard();
	mKeyboard->SetPosition(0, 192);
	mKeyboard->SetOnButtonClickCallback(OnKeyboardButtonClick);
	mKeyboard->Initialize();
	for(int i = 0; i < 15; i++)
	{
		int rnew = 31 + ((4 - 31) * i + 7) / 14;
		int gnew = 31 + ((4 - 31) * i + 7) / 14;
		int bnew = 31 + ((4 - 31) * i + 7) / 14;
		SPRITE_PALETTE_SUB[i + 1 + 64 + 16] = RGB5(rnew, gnew, bnew);
	}
	for(int i = 0; i < 15; i++)
	{
		int rnew = 31 + ((8 - 31) * i + 7) / 14;
		int gnew = 31 + ((16 - 31) * i + 7) / 14;
		int bnew = 31 + ((30 - 31) * i + 7) / 14;
		SPRITE_PALETTE_SUB[i + 1 + 64 + 16 + 16] = RGB5(rnew, gnew, bnew);
	}
	ProgressBar::InitializeVRAM();
	mTestAdapter = NULL;
	mList = NULL;
	mUIManager->AddSlice(mKeyboard);
	videoSetModeSub(MODE_0_2D);
	REG_DISPCNT_SUB |= DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE | DISPLAY_WIN0_ON | DISPLAY_WIN1_ON | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64;
	mTextCursorBlinking = FALSE;
	mTextCursorCounter = 0;
	irqSet(IRQ_VBLANK, VBlankProc);
	swiWaitForVBlank();
	bright = 32;
	while(bright > 0)
	{
		bright--;
		setBrightness(2, (bright + 1) / 2);
		swiWaitForVBlank();
	}
idle_loop:
#ifdef USE_WIFI
	while(1)
	{
		swiWaitForVBlank();
		if(mTestAdapter)
			mTestAdapter->DoFrameProc();
		if(mStartVideoId)
			break;
	}
#endif
/*#ifdef USE_WIFI
	consoleDemoInit();
	if(!Wifi_InitDefault(WFC_CONNECT)) 
	{
		iprintf("Failed to connect!");
	} 
	iprintf("Connected\n\n");
	swiWaitForVBlank();
	swiWaitForVBlank();
	//YT_SearchListResponse* response = YT_Search("grand dad", NULL);
	char* url = YT_GetVideoInfo(mVideoId);//(&response->searchResults[0])->videoId);//mVideoId);
	//YT_FreeSearchListResponse(response);
	mRingBufferHttpStream = new RingBufferHttpStream(url);
	free(url);
#endif*/
	PlayVideo();
	goto idle_loop;
	return 0;
}