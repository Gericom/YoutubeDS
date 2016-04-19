#include <nds.h>
#include "happyhttp/happyhttp.h"
#include "ringbufferhttpstream.h"
#include "copy.h"

RingBufferHttpStream::RingBufferHttpStream(char* url)
{
	//skip http:// and seek for first slash
	char* pURL = url + 7;
	while(*pURL != '/') pURL++;
	char* host = (char*)malloc(pURL - url - 7 + 1);
	memset(host, 0, pURL - url - 7 + 1);
	memcpy(host, url + 7, pURL - url - 7);
	mConnection = new happyhttp::Connection(host, 80);
	free(host);
	mConnection->setcallbacks( RingBufferHttpStream::OnBeginCallback, RingBufferHttpStream::OnDataCallback, RingBufferHttpStream::OnCompleteCallback, this);
	mConnection->request("GET", pURL);
	//allocate the ringbuffer
	mRingBuffer = (uint8_t*)malloc(RINGBUFFER_SIZE);
	mRingBufferReadPointer = mRingBuffer;
	mRingBufferWritePointer = mRingBuffer;
	mRingBufferBytesAvailable = 0;
	mStreamPosition = 0;
}

//dst may be NULL to seek forward
ITCM_CODE void RingBufferHttpStream::Read(uint8_t* dst, int count)
{
	if(count == 0) return;
	if(mRingBufferBytesAvailable >= count)//just copy from the ringbuffer
		ReadFromBuffer(dst, count);
	else
	{
		//copy at least what we currently have
		if(mRingBufferBytesAvailable > 0)
		{
			int amount = mRingBufferBytesAvailable;//BECAUSE IT'S NOT THE SAME ANYMORE AFTER ReadFromBuffer!
			ReadFromBuffer(dst, amount);
			if(dst) dst += amount;
			count -= amount;
		}
		while(count > 0)
		{
			while(mRingBufferBytesAvailable < count && mRingBufferBytesAvailable < (RINGBUFFER_SIZE / 2))
				mConnection->pump();//read more data
			if(mRingBufferBytesAvailable > 0)
			{
				int amount = (count < mRingBufferBytesAvailable) ? count : mRingBufferBytesAvailable;
				ReadFromBuffer(dst, amount);
				if(dst) dst += amount;
				count -= amount;
			}
		}
	}
}

void RingBufferHttpStream::OnBeginCallback(const happyhttp::Response* r)
{

}

ITCM_CODE void RingBufferHttpStream::OnDataCallback(const happyhttp::Response* r, const unsigned char* data, int n)
{
	WriteToBuffer((uint8_t*)data, n);
}

void RingBufferHttpStream::OnCompleteCallback(const happyhttp::Response* r)
{

}

//this reads directly from the ringbuffer
//with dst = NULL, this is actually seeking forward
ITCM_CODE inline void RingBufferHttpStream::ReadFromBuffer(uint8_t* dst, int count)
{
	if(mRingBufferReadPointer + count <= mRingBuffer + RINGBUFFER_SIZE)//we can do it at once
	{
		if(dst) MI_CpuCopy8((void*)mRingBufferReadPointer, dst, count); //memcpy(dst, (void*)mRingBufferReadPointer, count);
		mRingBufferReadPointer += count;
	}
	else//it'll take 2 copies
	{
		int firstcopy = (mRingBuffer + RINGBUFFER_SIZE) - mRingBufferReadPointer;
		if(dst)
		{
			MI_CpuCopy8((void*)mRingBufferReadPointer, dst, firstcopy); //memcpy(dst, (void*)mRingBufferReadPointer, firstcopy);
			MI_CpuCopy8((void*)mRingBuffer, dst + firstcopy, count - firstcopy); //memcpy(dst + firstcopy, (void*)mRingBuffer, count - firstcopy);
		}
		mRingBufferReadPointer = mRingBuffer + count - firstcopy;
	}
	mRingBufferBytesAvailable -= count;
	mStreamPosition += count;
}

//this writes directly to the ringbuffer
ITCM_CODE inline void RingBufferHttpStream::WriteToBuffer(uint8_t* src, int count)
{
	if(mRingBufferWritePointer + count <= mRingBuffer + RINGBUFFER_SIZE)//we can do it at once
	{
		MI_CpuCopy8(src, (void*)mRingBufferWritePointer, count); //memcpy((void*)mRingBufferWritePointer, src, count);
		mRingBufferWritePointer += count;
	}
	else//it'll take 2 copies
	{
		int firstcopy = (mRingBuffer + RINGBUFFER_SIZE) - mRingBufferWritePointer;
		MI_CpuCopy8(src, (void*)mRingBufferWritePointer, firstcopy); //memcpy((void*)mRingBufferWritePointer, src, firstcopy);
		MI_CpuCopy8(src + firstcopy, (void*)mRingBuffer, count - firstcopy); //memcpy((void*)mRingBuffer, src + firstcopy, count - firstcopy);
		mRingBufferWritePointer = mRingBuffer + count - firstcopy;
	}
	mRingBufferBytesAvailable += count;
}