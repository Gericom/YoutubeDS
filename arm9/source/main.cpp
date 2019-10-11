#include <fat.h>
#include <filesystem.h>
#include <nds.h>

#include "aacShared.h"
#include "fileBrowse.h"
#include "lock.h"
#include "mpu.h"
#include "mpeg4.h"
#include "mpeg4_tables.h"
#include "mpeg4/mpeg4_header.s"
#include "print.h"
#include "util.h"
#include "yuv2rgb_new.h"

#define AAC_ARM7

#define VIDEO_HEIGHT	144

#define TMP_BUFFER_SIZE		(64 * 1024)

static uint8_t mVideoTmpBuffer[TMP_BUFFER_SIZE] __attribute__ ((aligned (32)));

//static uint8_t mAudioTmpBuffer[TMP_BUFFER_SIZE] __attribute__ ((aligned (32)));

//static DTCM_DATA uint8_t sDtcmVideoBuf[4096];

static mpeg4_dec_struct mpeg4DecStruct __attribute__ ((aligned (32)));

static aac_queue_t sAACQueue __attribute__ ((aligned (32)));
static aac_queue_t* sAACQueueUncached;

//static uint8_t mYBufferA[FB_STRIDE * VIDEO_HEIGHT] __attribute__ ((aligned (32)));
//static uint8_t mYBufferB[FB_STRIDE * VIDEO_HEIGHT] __attribute__ ((aligned (32)));
//static uint8_t mUVBufferA[FB_STRIDE * (VIDEO_HEIGHT / 2)] __attribute__ ((aligned (32)));
//static uint8_t mUVBufferB[FB_STRIDE * (VIDEO_HEIGHT / 2)] __attribute__ ((aligned (32)));

static int16_t mYDCCoefCache[FB_STRIDE / 8 * VIDEO_HEIGHT / 8] __attribute__ ((aligned (32)));
static int16_t mUVDCCoefCache[FB_STRIDE / 8 * (VIDEO_HEIGHT / 2) / 8] __attribute__ ((aligned (32)));

//static mpeg4_block_dct_cache_entry mYDCTCache[FB_STRIDE / 8 * VIDEO_HEIGHT / 8] __attribute__ ((aligned (32)));
//static mpeg4_block_dct_cache_entry mUVDCTCache[FB_STRIDE / 8 * (VIDEO_HEIGHT / 2) / 8] __attribute__ ((aligned (32)));

static int16_t mMVecCache[(FB_STRIDE / 8 * VIDEO_HEIGHT / 8) * 2] __attribute__ ((aligned (32))); //2 times, because there is both dx and dy

static uint8_t* mVideoHeader;

static volatile int mUpscalingEnabled = false;

//#define AUDIO_BLOCK_SIZE	(1024)

//#define NR_WAVE_DATA_BUFFERS	(128)//(32)

//#define WAVE_DATA_BUFFER_LENGTH		(AUDIO_BLOCK_SIZE * NR_WAVE_DATA_BUFFERS)

//static s16 mWaveData[WAVE_DATA_BUFFER_LENGTH] __attribute__ ((aligned (32)));

static int mWaveDataOffs_write = 0;

static bool hasAudioStarted = false;

//FrameQueue implementation
#define NR_FRAME_BLOCKS		(20)//(9)//(10)//(6)
static volatile int curBlock = -1;
static volatile int nrFramesInQueue = 0;
static volatile int firstQueueBlock = 0;//block to read from (most of the time (curBlock + 1) % 4)
static volatile int lastQueueBlock = 0;//block to write to (most of the time (firstQueueBlock + nrFramesInQueue) % 4)

static uint8_t mYBuffer[NR_FRAME_BLOCKS][FB_STRIDE * VIDEO_HEIGHT] __attribute__ ((aligned (32)));
static uint8_t mUVBuffer[NR_FRAME_BLOCKS][FB_STRIDE * (VIDEO_HEIGHT / 2)] __attribute__ ((aligned (32)));

static int sVideoWidth;

static volatile int nrFramesMissed = 0;

static volatile int stopVideo;
static volatile int isVideoPlaying;

static char* mStartVideoId;

static int mDoubleSpeedEnabled = 0;

#define FRAME_SIZE	(256 * 144)//(176 * 144)
//static uint16_t mFrameQueue[FRAME_SIZE * NR_FRAME_BLOCKS] __attribute__ ((aligned (32)));

static volatile int mShouldCopyFrame;

ITCM_CODE static void frameHandler()
{
	mShouldCopyFrame = TRUE;
}

static BG23AffineInfo mLineAffineInfoOriginal[192] __attribute__ ((aligned (32)));
static BG23AffineInfo mLineAffineInfoUpscaled[192] __attribute__ ((aligned (32)));

//#define USE_WIFI

int mTimeScale = 1;

void StartTimer(int timescale)
{
	if(timescale == 12 || timescale == 12288)
		timerStart(0, ClockDivider_1024, TIMER_FREQ_1024(12) / (mDoubleSpeedEnabled + 1), frameHandler);
	else if(timescale == 8333)
		timerStart(0, ClockDivider_1024, -3928 / (mDoubleSpeedEnabled + 1), frameHandler);
	else if(timescale == 6)
		timerStart(0, ClockDivider_1024, TIMER_FREQ_1024(6) / (mDoubleSpeedEnabled + 1), frameHandler);
	else if(timescale == 30000)
		timerStart(0, ClockDivider_1024, -1092 / (mDoubleSpeedEnabled + 1), frameHandler);
	else if(timescale == 24000)
		timerStart(0, ClockDivider_1024, -1365 / (mDoubleSpeedEnabled + 1), frameHandler);
	else if(timescale == 60000)
		timerStart(0, ClockDivider_1024, -546 / (mDoubleSpeedEnabled + 1), frameHandler);
	else if(timescale == 999)
		timerStart(0, ClockDivider_1024, -3276 / (mDoubleSpeedEnabled + 1), frameHandler);
	else 
		timerStart(0, ClockDivider_1024, TIMER_FREQ_1024(10) / (mDoubleSpeedEnabled + 1), frameHandler);
}

static void aac_setQueueArm7()
{
	fifoSendValue32(FIFO_AAC, (AAC_FIFO_CMD_SET_QUEUE << 28) | ((u32)&sAACQueue));
}

static void aac_startDecArm7(int sampleRate)
{
	fifoSendValue32(FIFO_AAC, (AAC_FIFO_CMD_DECSTART << 28) | (sampleRate & 0x0FFFFFFF));
}

static void aac_stopDecArm7()
{
	fifoSendValue32(FIFO_AAC, AAC_FIFO_CMD_DECSTOP << 28);
}

static inline void aac_notifyBlock()
{
	fifoSendValue32(FIFO_AAC, AAC_FIFO_CMD_NOTIFY_BLOCK << 28);
}

static void aac_initQueue()
{
	memset(&sAACQueue, 0, sizeof(sAACQueue));
	sAACQueueUncached = (aac_queue_t*)memUncached(&sAACQueue);
	DC_FlushRange(&sAACQueue, (sizeof(sAACQueue) + 31) & ~31);
}

ITCM_CODE void PlayVideo()
{
	printf("PlayVideo\n");
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
	FILE* video = fopen(browseForFile().c_str(), "rb");

	// Clear the screen
	printf("\x1b[2J");

	printf("Opened file: %p\n", video);
	//find the moov atom
	while(true)
	{
		u32 size;
		u32 atom;
		fread(&size, 4, 1, video);
		fread(&atom, 4, 1, video);
		printf("size: %lx\n", SWAP_CONSTANT_32(size));
		printf("atom: %lx\n", SWAP_CONSTANT_32(atom));
		if(atom == 0x766F6F6D)
		{
			fseek(video, -8, SEEK_CUR);			
			mVideoHeader = (uint8_t*)malloc(SWAP_CONSTANT_32(size));
			fread(mVideoHeader, 1, SWAP_CONSTANT_32(size), video);
			break;
		}
		fseek(video, SWAP_CONSTANT_32(size) - 8, SEEK_CUR);
	}
	printf("Found moov\n");
#endif
	memset(&mpeg4DecStruct, 0, sizeof(mpeg4DecStruct));
	mpeg4DecStruct.pData = &mVideoTmpBuffer[0];
	mpeg4DecStruct.height = 144;
	mpeg4DecStruct.pDstY = &mYBuffer[0][0];
	mpeg4DecStruct.pDstUV = &mUVBuffer[0][0];
	mpeg4DecStruct.pIntraDCTVLCTable = &mpeg4_table_b16[0];
	mpeg4DecStruct.pInterDCTVLCTable = &mpeg4_table_b17[0];
	mpeg4DecStruct.pdc_coef_cache_y = &mYDCCoefCache[0];
	mpeg4DecStruct.pdc_coef_cache_uv = &mUVDCCoefCache[0];
	mpeg4DecStruct.pvector_cache = &mMVecCache[0];
	//mpeg4DecStruct.pdctCacheY = &mYDCTCache[0];
	//mpeg4DecStruct.pdctCacheUV = &mUVDCTCache[0];
	//mpeg4DecStruct.vop_time_increment_bits =4; //10;

	//find the header data we need
	uint8_t* pHeader = mVideoHeader;
	u32 moovSize = READ_SAFE_UINT32_BE(pHeader);	
	printf("moovSize: %lx\n", moovSize);
	uint8_t* pHeaderEnd = pHeader + moovSize;
	pHeader += 8;	//skip moov header

	uint32_t timescale = 0;
	uint8_t* framesizes = 0;
	uint8_t* videoBlockOffsets = 0;
	int nrframes = 0;
	uint8_t* audioBlockOffsets = 0;
	int audioRate = 0;
	//parse atoms
	while(pHeader < pHeaderEnd)
	{
		u32 size = READ_SAFE_UINT32_BE(pHeader);
		u32 atom = READ_SAFE_UINT32_BE(pHeader + 4);
		switch(atom)
		{
			//trak
			case 0x7472616B:
			{
				u8* ptr = pHeader + 8;
				u32 trackId = READ_SAFE_UINT32_BE(ptr + 0x14);
				ptr += READ_SAFE_UINT32_BE(ptr);//skip tkhd
				while(READ_SAFE_UINT32_BE(ptr + 4) != 0x6D646961)//mdia
					ptr += READ_SAFE_UINT32_BE(ptr);
				
				if(trackId == 1)
				{
					ptr += 8;	//skip mdia
					timescale = READ_SAFE_UINT32_BE(ptr + 0x14);
					mpeg4DecStruct.vop_time_increment_bits = 32 - __builtin_clz(timescale);
					ptr += READ_SAFE_UINT32_BE(ptr);//skip mdhd
					ptr += READ_SAFE_UINT32_BE(ptr);//skip hdlr
					ptr += 8;	//skip minf
					ptr += READ_SAFE_UINT32_BE(ptr);//skip vmhd
					ptr += READ_SAFE_UINT32_BE(ptr);//skip dinf
					ptr += 8;	//skip stbl

					//stsd
					sVideoWidth = READ_SAFE_UINT32_BE(ptr + 0x2E);
					printf("width: %d\n", sVideoWidth);
					ptr += READ_SAFE_UINT32_BE(ptr);//skip stsd
					ptr += READ_SAFE_UINT32_BE(ptr);//skip stts
					ptr += READ_SAFE_UINT32_BE(ptr);//skip stss
					ptr += READ_SAFE_UINT32_BE(ptr);//skip stsc
					ptr += 8;	//skip stsz
					ptr += 8;
					nrframes = READ_SAFE_UINT32_BE(ptr);
					ptr += 4;
					framesizes = ptr;
					ptr += nrframes * 4;
					videoBlockOffsets = ptr + 16;
				}
				else if(trackId == 2)
				{
					ptr += 8;	//skip mdia
					ptr += READ_SAFE_UINT32_BE(ptr);//skip mdhd
					ptr += READ_SAFE_UINT32_BE(ptr);//skip hdlr
					ptr += 8;	//skip minf
					ptr += READ_SAFE_UINT32_BE(ptr);//skip smhd
					ptr += READ_SAFE_UINT32_BE(ptr);//skip dinf
					ptr += 8;	//skip stbl
					//stsd
					audioRate = READ_SAFE_UINT32_BE(ptr + 0x2E);
					printf("audiorate: %d\n", audioRate);
					ptr += READ_SAFE_UINT32_BE(ptr);//skip stsd
					ptr += READ_SAFE_UINT32_BE(ptr);//skip stts
					ptr += READ_SAFE_UINT32_BE(ptr);//skip stsc
					ptr += READ_SAFE_UINT32_BE(ptr);//skip stsz
					audioBlockOffsets = ptr + 16;
				}
				break;
			}
		}
		pHeader += size;
	}

	mpeg4DecStruct.width = sVideoWidth;

	uint32_t offset = READ_SAFE_UINT32_BE(videoBlockOffsets);
	uint32_t nextAudioBlockOffset = READ_SAFE_UINT32_BE(audioBlockOffsets);	
	audioBlockOffsets += 4;
	if(nextAudioBlockOffset < offset)
		offset = nextAudioBlockOffset;
	else		
		videoBlockOffsets += 4;
#ifdef USE_WIFI
	mRingBufferHttpStream->Read(NULL, offset - mRingBufferHttpStream->GetStreamPosition());
#else
	fseek(video, offset, SEEK_SET);
#endif

	memset(&mMVecCache[0], 0, sizeof(mMVecCache));
	videoSetMode(MODE_5_2D);
	//vramSetBankE(VRAM_E_MAIN_BG);
	dmaFillWords(0, (void*)0x06000000, 256 * 192 * 2);
	vramSetBankC(VRAM_C_LCD);
	dmaFillWords(0x80008000, (void*)VRAM_C, 256 * 144 * 2);
	vramSetBankC(VRAM_C_MAIN_BG_0x06000000);
	bgInit(2, BgType_Bmp8, BgSize_B16_256x256, 0,0);
	bgInit(3, BgType_Bmp8, BgSize_B16_256x256, 0,0);
	REG_DISPCNT &= ~(DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE | DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE);
	REG_DISPCNT |= /*DISPLAY_WIN0_ON |*/ DISPLAY_BG2_ACTIVE | (mpeg4DecStruct.width == 256 ? 0 : DISPLAY_BG3_ACTIVE);

	REG_BG2PA = mpeg4DecStruct.width == 256 ? 256 : 176;
	REG_BG2PB = 0;
	REG_BG2PC = 0;
	REG_BG2PD = 256;
	REG_BG2X = 0;
	REG_BG2Y = -24 << 8;

	if(mpeg4DecStruct.width != 256)
	{
		REG_BG3PA = 176;
		REG_BG3PB = 0;
		REG_BG3PC = 0;
		REG_BG3PD = 256;
		REG_BG3X = 80;
		REG_BG3Y = -24 << 8;
		REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG2 | BLEND_SRC_BG3 | BLEND_DST_BG2 | BLEND_DST_BG3 | BLEND_DST_BACKDROP;
		REG_BLDALPHA = 8 | (8 << 8);
	}
	else
		REG_BLDCNT = 0;

	

	aac_initQueue();
#ifdef AAC_ARM7
	aac_startDecArm7(audioRate);
#else
	HAACDecoder aacDec = AACInitDecoder();
	AACFrameInfo frameInfo;
	frameInfo.bitRate = 0;
	frameInfo.nChans = 1;
	frameInfo.sampRateCore = audioRate;
	frameInfo.sampRateOut = audioRate;
	frameInfo.bitsPerSample = 16;
	frameInfo.outputSamps = 0;
	frameInfo.profile = 0;
	frameInfo.tnsUsed = 0;
	frameInfo.pnsUsed = 0;
	AACSetRawBlockParams(aacDec, 0, &frameInfo);
#endif
	stopVideo = FALSE;
	mTimeScale = timescale;
	
	int frame = 0;
	StartTimer(timescale);
	while((!stopVideo || frame < 20) && frame < nrframes)
	{
		if(frame < nrframes && offset == nextAudioBlockOffset)
		{
			offset = READ_SAFE_UINT32_BE(videoBlockOffsets);
			int audiosize = offset - nextAudioBlockOffset;
			videoBlockOffsets += 4;		
			
#ifdef AAC_ARM7
			while(sAACQueueUncached->blockCount == AAC_QUEUE_BLOCK_COUNT);
			fread(&sAACQueue.queue[sAACQueueUncached->writeBlock][0], 1, audiosize, video);
			//DC_FlushRange(&sAACQueue.queue[sAACQueue.writeBlock][0], (audiosize + 31) & ~0x1F);
			sAACQueueUncached->queueBlockLength[sAACQueueUncached->writeBlock] = audiosize;
			//sAACQueue.writeBlock = (sAACQueue.writeBlock + 1) % AAC_QUEUE_BLOCK_COUNT;
			sAACQueueUncached->writeBlock = (sAACQueueUncached->writeBlock + 1) % AAC_QUEUE_BLOCK_COUNT;
			lock_lock(&sAACQueueUncached->countLock);
			{
				sAACQueueUncached->blockCount++;
			}
			lock_unlock(&sAACQueueUncached->countLock);
			aac_notifyBlock();
#else
			uint8_t* audioData = &mAudioTmpBuffer[0];
			uint8_t* audioDataptr = audioData;
			fread(audioData, 1, audiosize, video);
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
				soundIdL = soundPlaySample(&mWaveData[0], SoundFormat_16Bit, sizeof(mWaveData), audioRate * (mDoubleSpeedEnabled + 1), 127, 0, true, 0);
				soundIdR = soundPlaySample(&mWaveData[0], SoundFormat_16Bit, sizeof(mWaveData), audioRate * (mDoubleSpeedEnabled + 1), 127, 127, true, 0);
				hasAudioStarted = true;
			}
#endif
			//fread(&sAACQueueUncached->queue[sAACQueue.writeBlock][0], 1, audiosize, video);
			//fread(audioData, 1, audiosize, video);
			nextAudioBlockOffset = READ_SAFE_UINT32_BE(audioBlockOffsets);
			audioBlockOffsets += 4;
		}

		uint32_t size = READ_SAFE_UINT32_BE(framesizes);
		framesizes += 4;
		// if(size < sizeof(sDtcmVideoBuf))
		// {
		// 	fread(sDtcmVideoBuf, 1, size, video);
		// 	mpeg4DecStruct.pData = (u8*)sDtcmVideoBuf;
		// }
		// else
		// {
			fread(&mVideoTmpBuffer[0], 1, size, video);
			mpeg4DecStruct.pData = (u8*)&mVideoTmpBuffer[0];
		// }
		offset += size;
		//DC_InvalidateRange(VRAM_E, (size + 0x1F) & ~0x1F);
		//dmaCopyWords(3, &mVideoTmpBuffer[0], VRAM_E, (size + 3) & ~3);
		//mpeg4DecStruct.pData = (u8*)&mVideoTmpBuffer[0];
		while(nrFramesInQueue >= NR_FRAME_BLOCKS - 1 || lastQueueBlock == curBlock);
		mpeg4DecStruct.pPrevY = mpeg4DecStruct.pDstY;
		mpeg4DecStruct.pPrevUV = mpeg4DecStruct.pDstUV;
		mpeg4DecStruct.pDstY = &mYBuffer[lastQueueBlock][0];
		mpeg4DecStruct.pDstUV = &mUVBuffer[lastQueueBlock][0];
		for(uint q = 0; q < sizeof(mYDCCoefCache) / sizeof(mYDCCoefCache[0]); q++)
			mYDCCoefCache[q] = 1024;
		for(uint q = 0; q < sizeof(mUVDCCoefCache) / sizeof(mUVDCCoefCache[0]); q++)
			mUVDCCoefCache[q] = 1024;
		for(uint q = 0; q < sizeof(mMVecCache) / sizeof(mMVecCache[0]); q++)
			mMVecCache[q] = 0;
		if(mpeg4DecStruct.pData[2] == 1 && mpeg4DecStruct.pData[3] == 0xB3)
			mpeg4DecStruct.pData += 7;
		mpeg4DecStruct.pData += 4;//skip 000001B6

		//if(frame == 357)
		//	asm volatile("mov r11, r11");
		//cpuStartTiming(2);
		//cpuStartTiming(1);
		mpeg4_VideoObjectPlane(&mpeg4DecStruct);
		// u32 timing = cpuEndTiming();
		// printf("%d\n", timing);
		// if(frame == 20)
		// {
		// 	enterCriticalSection();
		// 	while(1);
		// }
		//uint32_t time = cpuEndTiming();
		//nocashPrint1("Time: %r0%", time);
		//char tmp[21];
		//memset(tmp, 0, sizeof(tmp));
		//sprintf(tmp,"0x%x",time);
		//mToolbar->SetTitle(tmp);
		frame++;
		if(frame == 1)//6)
			isVideoPlaying = TRUE;
		/*if(frame < nrframes && offset == nextAudioBlockOffset)
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
				soundIdL = soundPlaySample(&mWaveData[0], SoundFormat_16Bit, sizeof(mWaveData), 22050 * (mDoubleSpeedEnabled + 1), 127, 0, true, 0);
				soundIdR = soundPlaySample(&mWaveData[0], SoundFormat_16Bit, sizeof(mWaveData), 22050 * (mDoubleSpeedEnabled + 1), 127, 127, true, 0);
				hasAudioStarted = true;
			}
			nextAudioBlockOffset = READ_SAFE_UINT32_BE(audioBlockOffsets);
			audioBlockOffsets += 4;
		}*/
		//while(nrFramesInQueue >= NR_FRAME_BLOCKS || lastQueueBlock == curBlock);
		//cpuStartTiming(2);
		//yuv2rgb_new(mpeg4DecStruct.pDstY, mpeg4DecStruct.pDstUV, &mFrameQueue[lastQueueBlock * FRAME_SIZE]);
		//DC_FlushRange(&mFrameQueue[lastQueueBlock * FRAME_SIZE], FRAME_SIZE * 2);
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
		//if(mTestAdapter)
		//	mTestAdapter->DoFrameProc();
		if(frame < 20) stopVideo = FALSE;
	}
// video_stop:
	timerStop(0);
	REG_DISPCNT &= ~(DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE);
	DMA0_CR = 0;
	isVideoPlaying = FALSE;
#ifdef AAC_ARM7
	aac_stopDecArm7();
#else
	if(hasAudioStarted)
	{
		soundKill(soundIdL);
		soundKill(soundIdR);
		hasAudioStarted = FALSE;
	}
	AACFreeDecoder(aacDec);
#endif
	fclose(video);
	free(mVideoHeader);
}

static std::string mSearchString;

static int mKeyTimer = 0;

static bool mUseVramB = false;
static bool mCopyDone = false;

ITCM_CODE void VBlankProc()
{
	if(isVideoPlaying)//stride dma and frame copy
	{
		if(mCopyDone)
		{
			mCopyDone = false;
			if(mUseVramB)
			{
				vramSetBankA(VRAM_A_LCD);
				vramSetBankB(VRAM_B_MAIN_BG_0x06000000);
			}
			else
			{
				vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
				vramSetBankB(VRAM_B_LCD);
			}
			mUseVramB = !mUseVramB;
		}
		if(mShouldCopyFrame)
		{
			mShouldCopyFrame = FALSE;
			if(nrFramesInQueue > 0)
			{
				void* addr = mUseVramB ? VRAM_B : VRAM_A;
				//cpuStartTiming(1);
				if(sVideoWidth == 256)
					yog2rgb_convert256(&mYBuffer[firstQueueBlock][0], &mUVBuffer[firstQueueBlock][0], (u16*)addr);
				else
					yog2rgb_convert176(&mYBuffer[firstQueueBlock][0], &mUVBuffer[firstQueueBlock][0], (u16*)addr);				
				// if(sVideoWidth == 256)
				// 	y2r_convert256(&mYBuffer[firstQueueBlock][0], &mUVBuffer[firstQueueBlock][0], (u16*)addr);
				// else
				// 	y2r_convert176(&mYBuffer[firstQueueBlock][0], &mUVBuffer[firstQueueBlock][0], (u16*)addr);
				//u32 timing = cpuEndTiming();
				//printf("%d\n", timing);
				DC_FlushRange(addr, FRAME_SIZE * 2);
				mCopyDone = true;
			}
			else
			{
				printf("Skip\n");
				//dmaFillWords(0x801F801F, (void*)&BG_GFX[0], FRAME_SIZE * 2);
			}			
			curBlock = firstQueueBlock;
			firstQueueBlock = (firstQueueBlock + 1) % NR_FRAME_BLOCKS;
			nrFramesInQueue--;
		}

		scanKeys();
		if(keysDown() & KEY_B)	stopVideo = true;
	}
	else
		mKeyTimer = 0;
}

int main()
{
	mpu_enableVramCache();
	defaultExceptionHandler();
	mStartVideoId = NULL;
	isVideoPlaying = FALSE;
	mUpscalingEnabled = FALSE;
	if(!fatInitDefault())
		nitroFSInit(NULL);
	//defaultExceptionHandler();
	//consoleDemoInit();
	videoSetModeSub(MODE_0_2D);
	vramSetBankH(VRAM_H_SUB_BG);

	consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 2, 0, false, true);
	//if(nitroFSInit(NULL))
	//	printf("NitroFS works\n");
	keysSetRepeat(25, 5);

	soundEnable();

	aac_setQueueArm7();
	//make the bottom screen white and fade in a nice logo at the top screen
	//setBrightness(1, 16);
	//setBrightness(2, 16);
	//videoSetMode(MODE_0_2D);
	//uint8_t* load_tmp = (uint8_t*)malloc(6080);
	//FILE* file = fopen("/Logo.nbfp", "rb");
	//fread(&load_tmp[0], 1, 512, file);
	//fclose(file);
	//for(int i = 0; i < 256; i++)
	//{
	//	BG_PALETTE[i] = load_tmp[i*2]|(load_tmp[i*2+1] << 8);
	//}
	BG_PALETTE[0] = 0;
	//vramSetBankE(VRAM_E_MAIN_BG);
	vramSetBankA(VRAM_A_MAIN_BG);
	//vramSetBankB(VRAM_B_MAIN_BG_0x06040000);
	vramSetBankD(VRAM_D_ARM7_0x06000000);
	vramSetBankE(VRAM_E_LCD);
	vramSetBankF(VRAM_F_MAIN_SPRITE_0x06400000);
	vramSetBankG(VRAM_G_MAIN_SPRITE_0x06404000);

	/*file = fopen("/Logo.nbfc", "rb");
	fread(&load_tmp[0], 1, 6080, file);
	fclose(file);
	DC_FlushRange(&load_tmp[0], 4416);
	dmaCopyWords(3, load_tmp, &BG_GFX[0], 6080);
	file = fopen("/Logo.nbfs", "rb");
	fread(&load_tmp[0], 1, 2048, file);
	fclose(file);
	DC_FlushRange(&load_tmp[0], 2048);
	dmaCopyWords(3, load_tmp, &BG_GFX[6144 >> 1], 2048);
	bgInit(0, BgType_Text8bpp, BgSize_T_256x256, 3, 0);*/
	REG_DISPCNT |= DISPLAY_BG0_ACTIVE;
	/*int bright = 32;
	while(bright > 0)
	{
		bright--;
		setBrightness(1, (bright + 1) / 2);
		swiWaitForVBlank();
	}*/
#ifdef USE_WIFI
	Wifi_InitDefault(WFC_CONNECT);
#endif
	Util_SetupStrideFixAffine(&mLineAffineInfoOriginal[0], 176, 256, 0, 24, 176, 256, 0);
	Util_SetupStrideFixAffine(&mLineAffineInfoUpscaled[0], 176, 256, 0, 24, 176, 256, -88);
	//Toolbar test
	//vramSetBankH(VRAM_H_SUB_BG);
	//vramSetBankI(VRAM_I_SUB_BG_0x06208000);
	//vramSetBankD(VRAM_D_SUB_SPRITE);
	//Load toolbar shadow
	//file = fopen("/Menu/Shadow.nbfp", "rb");
	//fread(&load_tmp[0], 1, 32, file);
	//fclose(file);
	//for(int i = 0; i < 16; i++)
	//{
	//	BG_PALETTE_SUB[i] = load_tmp[i*2]|(load_tmp[i*2+1] << 8);
	//}
	//file = fopen("/Menu/Shadow.nbfc", "rb");
	//fread(&load_tmp[0], 1, 64, file);
	//fclose(file);
	//for(int i = 0; i < (64 >> 1); i++)
	//{
	//	BG_GFX_SUB[i] = load_tmp[i*2]|(load_tmp[i*2+1] << 8);
	//}
	//file = fopen("/Menu/Shadow.nbfs", "rb");
	//fread(&load_tmp[0], 1, 2048, file);
	//fclose(file);
	//for(int i = 0; i < 2048 / 2; i++)
	//{
	//	BG_GFX_SUB[(0x3800 >> 1) + i] = load_tmp[i*2]|(load_tmp[i*2+1] << 8);
	//}
	//free(load_tmp);
	//REG_BG1CNT_SUB = BG_32x32 | BG_PRIORITY_2 | BG_COLOR_16 | BG_MAP_BASE(7);
	//REG_BLDCNT_SUB = BLEND_ALPHA | BLEND_SRC_BG1 | BLEND_DST_BG0 | BLEND_DST_SPRITE | BLEND_DST_BACKDROP;
	//REG_BLDALPHA_SUB = 3 | (13 << 8);

	//roboto_medium_13 = Font_Load("/Fonts/RobotoMedium13.nft");
	//roboto_regular_10 = Font_Load("/Fonts/RobotoRegular10.nft");
	//for(int i = 0; i < 512; i++)
	//{
	//	if(!(i & 3))
	//		OAM_SUB[i] = ATTR0_DISABLED;
	//	else OAM_SUB[i] = 0;
	//}
	//BG_PALETTE_SUB[0] = RGB5(31,31,31);
	/*mToolbar = new Toolbar(RGB5(28,4,3), RGB5(31,31,31), RGB5(31,31,31), "YouTube", roboto_medium_13);
	mToolbar->SetShowSearchButton(TRUE);
	mToolbar->SetOnButtonClickCallback(OnToolbarButtonClick);
	mToolbar->Initialize();
	mUIManager->AddSlice(mToolbar);
	mKeyboard = new ScreenKeyboard();
	mKeyboard->SetPosition(0, 192);
	mKeyboard->SetOnButtonClickCallback(OnKeyboardButtonClick);
	mKeyboard->Initialize();*/
	/*for(int i = 0; i < 15; i++)
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
	ProgressBar::InitializeVRAM();*/
	//mUIManager->AddSlice(mKeyboard);
	//videoSetModeSub(MODE_0_2D);
	//REG_DISPCNT_SUB |= DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE | DISPLAY_WIN0_ON | DISPLAY_WIN1_ON | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64;
	irqSet(IRQ_VBLANK, VBlankProc);
	swiWaitForVBlank();
	/*bright = 32;
	while(bright > 0)
	{
		bright--;
		setBrightness(2, (bright + 1) / 2);
		swiWaitForVBlank();
	}*/
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
		printf("Failed to connect!");
	} 
	printf("Connected\n\n");
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