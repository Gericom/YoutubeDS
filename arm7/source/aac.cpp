#include <nds.h>
#include <string.h>
#include "lock.h"
#include "aac_pub/aacdec.h"
extern "C"
{
    #include "coder.h"
}
#include "aacShared.h"
#include "aac.h"

#define AUDIO_CHANNEL_LEFT      0
#define AUDIO_CHANNEL_RIGHT     1

#define AUDIO_BLOCK_COUNT   32
#define AUDIO_BLOCK_SIZE    1024

// __attribute__((section(".crt0"))) static s16 sAudioBuf[AUDIO_BLOCK_COUNT * AUDIO_BLOCK_SIZE] __attribute__ ((aligned (32)));
extern s16 aac_audioBuffer[AUDIO_BLOCK_COUNT * AUDIO_BLOCK_SIZE];

//#define VRAM_AUDIO_BUF  ((s16*)0x06000000)

static volatile int sAudioBlockCount;
static int sNextAudioBlock;
static bool sAudioStarted;

static bool sAACInitialized;
static HAACDecoder sAACDecoder;
static int sSampleRate;

static aac_queue_t* sAACQueue;
static bool sDecStarted;

static u8 sAACCache[AAC_QUEUE_BLOCK_SIZE] __attribute__ ((aligned (32)));

static volatile bool sDecBlocksAvailable;

extern const int sinWindowEWram[128 + 1024];
extern const int kbdWindowEWram[128 + 1024];
extern const int twidTabOddEWram[8*6 + 32*6 + 128*6];
extern const int twidTabEvenEWram[4*6 + 16*6 + 64*6];

void trigtab_relocate()
{
    memcpy((void*)raac_sinWindow, sinWindowEWram, sizeof(sinWindowEWram));
    memcpy((void*)raac_kbdWindow, kbdWindowEWram, sizeof(kbdWindowEWram));
    memcpy((void*)raac_twidTabOdd, twidTabOddEWram, sizeof(twidTabOddEWram));
    memcpy((void*)raac_twidTabEven, twidTabEvenEWram, sizeof(twidTabEvenEWram));
}

static void onTimerTick()
{
    sAudioBlockCount--;
}

void aac_play()
{
    SCHANNEL_SOURCE(AUDIO_CHANNEL_LEFT) = (u32)aac_audioBuffer;//(u32)sAudioBuf;
    SCHANNEL_REPEAT_POINT(AUDIO_CHANNEL_LEFT) = 0;
    SCHANNEL_LENGTH(AUDIO_CHANNEL_LEFT) = (AUDIO_BLOCK_COUNT * AUDIO_BLOCK_SIZE * 2) >> 2;
    SCHANNEL_TIMER(AUDIO_CHANNEL_LEFT) = SOUND_FREQ(sSampleRate);

    SCHANNEL_SOURCE(AUDIO_CHANNEL_RIGHT) = (u32)aac_audioBuffer;//(u32)sAudioBuf;
    SCHANNEL_REPEAT_POINT(AUDIO_CHANNEL_RIGHT) = 0;
    SCHANNEL_LENGTH(AUDIO_CHANNEL_RIGHT) = (AUDIO_BLOCK_COUNT * AUDIO_BLOCK_SIZE * 2) >> 2;
    SCHANNEL_TIMER(AUDIO_CHANNEL_RIGHT) = SOUND_FREQ(sSampleRate);

    SCHANNEL_CR(AUDIO_CHANNEL_LEFT) = SCHANNEL_ENABLE | SOUND_VOL(0x7F) | SOUND_PAN(0) | SOUND_FORMAT_16BIT | SOUND_REPEAT;
    SCHANNEL_CR(AUDIO_CHANNEL_RIGHT) = SCHANNEL_ENABLE | SOUND_VOL(0x7F) | SOUND_PAN(0x7F) | SOUND_FORMAT_16BIT | SOUND_REPEAT;
    timerStart(0, ClockDivider_1024, TIMER_FREQ(sSampleRate), onTimerTick);
    sAudioStarted = true;
}

void aac_stop()
{
    SCHANNEL_CR(AUDIO_CHANNEL_LEFT) = 0;
    SCHANNEL_CR(AUDIO_CHANNEL_RIGHT) = 0;
    timerStop(0);
    sAudioStarted = false;
}

void aac_reset()
{    
    aac_stop();
    sDecStarted = false;
    sDecBlocksAvailable = false;
    if(sAACInitialized)
    {
        AACFreeDecoder(sAACDecoder);
        sAACInitialized = false;
    }
    sNextAudioBlock = 0;
    sAudioStarted = false;
    sAACInitialized = false;
}

void aac_initDecoder(int sampleRate)
{
    aac_reset();
    trigtab_relocate();
    sAACDecoder = AACInitDecoder();
    sSampleRate = sampleRate;
    AACFrameInfo frameInfo;
	frameInfo.bitRate = 0;
	frameInfo.nChans = 1;
	frameInfo.sampRateCore = sSampleRate;
	frameInfo.sampRateOut = sSampleRate;
	frameInfo.bitsPerSample = 16;
	frameInfo.outputSamps = 0;
	frameInfo.profile = 0;
	frameInfo.tnsUsed = 0;
	frameInfo.pnsUsed = 0;
	AACSetRawBlockParams(sAACDecoder, 0, &frameInfo);
    sAACInitialized = true;
}

static void onFifoCommand(u32 command, void* userdata)
{
    int cmd = command >> 28;
    u32 arg = command & 0x0FFFFFFF;
    switch(cmd)
    {
        case AAC_FIFO_CMD_SET_QUEUE:
            sAACQueue = (aac_queue_t*)arg;
            break;
        case AAC_FIFO_CMD_DECSTART:
            aac_initDecoder(arg);
            sDecStarted = true;
            break;
        case AAC_FIFO_CMD_DECSTOP:
            aac_reset();
            break;
        case AAC_FIFO_CMD_NOTIFY_BLOCK:
            sDecBlocksAvailable = true;
            break;
    }
}

void aac_init()
{
    aac_reset();
    fifoSetValue32Handler(FIFO_AAC, onFifoCommand, NULL);
}

void aac_decode(u8* data, int length)
{
    while(length > 0)
    {
        int err = 0;
        if((err = AACDecode(sAACDecoder, &data, &length, &aac_audioBuffer[sNextAudioBlock * AUDIO_BLOCK_SIZE])) < 0) 
        {
            while(1);
            break;
        }
        sNextAudioBlock = (sNextAudioBlock + 1) % AUDIO_BLOCK_COUNT;
        //critical section, we don't want the audio block count to clash with a timer tick
        int savedIrq = enterCriticalSection();
        {
            sAudioBlockCount++;
        }
        leaveCriticalSection(savedIrq);
    }
    if(!sAudioStarted && sAudioBlockCount >= 8)
        aac_play();
}

void aac_main()
{
    while(sAACInitialized && sDecStarted /*&& sAudioBlockCount < AUDIO_BLOCK_COUNT*/ && sDecBlocksAvailable)//sAACQueue->blockCount > 0)
    {
        sDecBlocksAvailable = false;
        int len = sAACQueue->queueBlockLength[sAACQueue->readBlock];
        if(len > sizeof(sAACCache))
        {
            while(1);
        }
        dmaCopyWords(3, &sAACQueue->queue[sAACQueue->readBlock][0], sAACCache, (len + 3) & ~3);
        aac_decode(sAACCache, len);
        sAACQueue->readBlock = (sAACQueue->readBlock + 1) % AAC_QUEUE_BLOCK_COUNT;
        lock_lock(&sAACQueue->countLock);
        {
            sAACQueue->blockCount--;
        }
        lock_unlock(&sAACQueue->countLock);
    }
}