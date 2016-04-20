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

static char* mVideoId = "2JyUD79ky8s";//"2JyUD79ky8s";

#define TMP_BUFFER_SIZE		(192 * 256 * 2)

static uint8_t mVideoTmpBuffer[TMP_BUFFER_SIZE] __attribute__ ((aligned (32)));

static uint8_t mAudioTmpBuffer[TMP_BUFFER_SIZE] __attribute__ ((aligned (32)));

static mpeg4_dec_struct mpeg4DecStruct __attribute__ ((aligned (32)));

static uint8_t mYBufferA[256 * 144] __attribute__ ((aligned (32)));
static uint8_t mUVBufferA[256 * 72] __attribute__ ((aligned (32)));

static uint8_t mYBufferB[256 * 144] __attribute__ ((aligned (32)));
static uint8_t mUVBufferB[256 * 72] __attribute__ ((aligned (32)));

static int16_t mYDCCoefCache[256 / 8 * 144 / 8] __attribute__ ((aligned (32)));
static int16_t mUVDCCoefCache[256 / 8 * 72 / 8] __attribute__ ((aligned (32)));

static int16_t mMVecCache[(256 / 8 * 144 / 8) * 2] __attribute__ ((aligned (32))); //2 times, because there is both dx and dy

//tempoarly, should later be dynamically allocated of course
//static uint8_t mVideoHeader[0x1760];
static uint8_t* mVideoHeader;

static int mWiggleState = 0;

static int bg2, bg3;

/*void Vblank() 
{
	mWiggleState = (mWiggleState + 1) & 1;
	bgSet(bg3, 0, 176, 176, 64, 5 * 256 + mWiggleState * 64, 0, 0);
	bgSet(bg2, 0, 176, 176, -64, 5 * 256 - mWiggleState * 64, 0, 0);
	bgUpdate();
}*/


static bool mUpscalingEnabled = false;

#define AUDIO_BLOCK_SIZE	(1024)// * 10 / FRAME_RATE)

#define NR_WAVE_DATA_BUFFERS	(32)

#define WAVE_DATA_BUFFER_LENGTH		(AUDIO_BLOCK_SIZE * NR_WAVE_DATA_BUFFERS)

static s16 mWaveData[WAVE_DATA_BUFFER_LENGTH] __attribute__ ((aligned (32)));//ATTRIBUTE_ALIGN(32);

static int mWaveDataOffs_write = 0;

static bool hasAudioStarted = false;

//FrameQueue implementation
#define NR_FRAME_BLOCKS		(6)
static volatile int curBlock = -1;
static volatile int nrFramesInQueue = 0;
static volatile int firstQueueBlock = 0;//block to read from (most of the time (curBlock + 1) % 4)
static volatile int lastQueueBlock = 0;//block to write to (most of the time (firstQueueBlock + nrFramesInQueue) % 4)

static RingBufferHttpStream* mRingBufferHttpStream;

static volatile int nrFramesMissed = 0;
static int soundIdL = 0;
static int soundIdR = 0;

static void ResyncAudio()
{
	if(hasAudioStarted)
	{
		soundKill(soundIdL);
		soundKill(soundIdR);
		mWaveDataOffs_write = 0;
		hasAudioStarted = false;
	}
}

ITCM_CODE static void frameHandler()
{
	if(nrFramesInQueue <= 0)
	{
		BG_PALETTE_SUB[0] = RGB5(31,0,0);
		//nrFramesMissed++;
		return;//We can't do anything without data
	}
	//Solves non vsync (switch in middle of frame), but makes it much slower!
	//while(!GX_IsVBlank());
	BG_PALETTE_SUB[0] = RGB5(0,0,0);
	REG_BG3CNT = ((5 * firstQueueBlock) << 8) | 0x4084;
	curBlock = firstQueueBlock;
	firstQueueBlock = (firstQueueBlock + 1) % NR_FRAME_BLOCKS;
	nrFramesInQueue--;
}

#define USE_WIFI

int main()
{
	defaultExceptionHandler();
#ifdef USE_WIFI
	consoleDemoInit();
	if(!Wifi_InitDefault(WFC_CONNECT)) 
	{
		iprintf("Failed to connect!");
	} 
	iprintf("Connected\n\n");
	swiWaitForVBlank();
	swiWaitForVBlank();
	char* url = YT_GetVideoInfo(mVideoId);
	mRingBufferHttpStream = new RingBufferHttpStream(url);
	free(url);
#else
	nitroFSInit(NULL);
#endif
	soundEnable();
#ifdef USE_WIFI
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
    vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	vramSetBankB(VRAM_B_MAIN_BG_0x06020000);
	vramSetBankC(VRAM_C_MAIN_BG_0x06040000);
	vramSetBankD(VRAM_D_MAIN_BG_0x06060000);
	bg3 = bgInit(3, BgType_Bmp8, BgSize_B16_256x256, 0,0);
	bgSet(bg3, 0, 256, 256, -40 * 256, -24 * 256, 0, 0);
	WIN0_X0 = 40;
	WIN0_X1 = 40 + 176;
	WIN0_Y0 = 24;
	WIN0_Y1 = 24 + 144;
	WIN_IN = (1 << 3);
	REG_DISPCNT |= DISPLAY_WIN0_ON;
	bgUpdate();

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
	int frame = 0;
	timerStart(0, ClockDivider_1024, TIMER_FREQ_1024(/*12*/10), frameHandler);
	int keytimer = 0;
	while(frame < nrframes)
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
		//We can not use this, as it will somehow break everything
		//MI_CpuFillFast(&mYDCCoefCache[0], (1024 << 16) | 1024, sizeof(mYDCCoefCache));
		//MI_CpuFillFast(&mUVDCCoefCache[0], (1024 << 16) | 1024, sizeof(mUVDCCoefCache));
		//MI_CpuFillFast(&mMVecCache[0], 0, sizeof(mMVecCache));
		//So we're forced to use this instead
		dmaFillWords((1024 << 16) | 1024, &mYDCCoefCache[0], sizeof(mYDCCoefCache));
		dmaFillWords((1024 << 16) | 1024, &mUVDCCoefCache[0], sizeof(mUVDCCoefCache));
		memset(&mMVecCache[0], 0, sizeof(mMVecCache));
		DC_InvalidateRange(&mYDCCoefCache[0], sizeof(mYDCCoefCache));
		DC_InvalidateRange(&mUVDCCoefCache[0], sizeof(mUVDCCoefCache));
		if(mpeg4DecStruct.pData[2] == 1 && mpeg4DecStruct.pData[3] == 0xB3)
			mpeg4DecStruct.pData += 7;
		mpeg4DecStruct.pData += 4;//skip 000001B6
		int save = enterCriticalSection();
		mpeg4_VideoObjectPlane(&mpeg4DecStruct);
		leaveCriticalSection(save);
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
			/*if(nrFramesMissed > 10)
			{
				ResyncAudio();
				nrFramesMissed = 0;
			}*/
			while(audiosize > 0)
			{
				int err = 0;
				if((err = AACDecode(aacDec, &audioDataptr, &audiosize, &mWaveData[mWaveDataOffs_write * AUDIO_BLOCK_SIZE])) < 0) 
				{
					break;
				}
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

		if(keytimer <= 0)
		{
			scanKeys();
			int held = keysHeld();
			if(held & KEY_SELECT)//toggle upscaling
			{
				mUpscalingEnabled = !mUpscalingEnabled;
				if(!mUpscalingEnabled)
				{
					bgSet(bg3, 0, 256, 256, -40 * 256, -24 * 256, 0, 0);
					WIN0_X0 = 40;
					WIN0_X1 = 40 + 176;
					WIN0_Y0 = 24;
					WIN0_Y1 = 24 + 144;
					WIN_IN = (1 << 3);
					REG_DISPCNT |= DISPLAY_WIN0_ON;
				}
				else
				{
					bgSet(bg3, 0, 176, 176, 0, 6.5 * 256, 0, 0);
					REG_DISPCNT &= ~DISPLAY_WIN0_ON;
				}
				bgUpdate();
				keytimer = 2;
			}
		}
		else keytimer--;
		while(nrFramesInQueue >= NR_FRAME_BLOCKS || lastQueueBlock == curBlock);
		//copy2vram(mpeg4DecStruct.pDstY, mpeg4DecStruct.pDstUV, (uint16_t*)(((uint32_t)BG_GFX) + 96 * 1024 * lastQueueBlock));
		yuv2rgb_new(mpeg4DecStruct.pDstY, mpeg4DecStruct.pDstUV, (uint16_t*)(((uint32_t)BG_GFX) + /*96*/80 * 1024 * lastQueueBlock));
		lastQueueBlock = (lastQueueBlock + 1) % NR_FRAME_BLOCKS;
		nrFramesInQueue++;
		asm("mov r11, r11");
	}
	
	while(1)
	{
		swiWaitForVBlank();
	}
	return 0;
}