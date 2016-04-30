#ifndef __RINGBUFFERHTTPSTREAM_H__
#define __RINGBUFFERHTTPSTREAM_H__
#include "happyhttp/happyhttp.h"

#define RINGBUFFER_SIZE		(16 * 1024)	//use a 16kb ring buffer, that's safe, because happyhttp uses 2kb chunks max

class RingBufferHttpStream
{
private:
	happyhttp::Connection* mConnection;
	uint8_t* mRingBuffer;
	volatile uint8_t* mRingBufferReadPointer;
	volatile uint8_t* mRingBufferWritePointer;
	volatile int mRingBufferBytesAvailable;
	volatile int mStreamPosition;
public:
	RingBufferHttpStream(char* url);

	~RingBufferHttpStream()
	{
		free(mRingBuffer);
		delete mConnection;
	}

	void Read(uint8_t* dst, int count);

	int GetStreamPosition() { return mStreamPosition; }

	static void OnBeginCallback(const happyhttp::Response* r, void* userdata)
	{
		((RingBufferHttpStream*)userdata)->OnBeginCallback(r);
	}

	static void OnDataCallback(const happyhttp::Response* r, void* userdata, const unsigned char* data, int n)
	{
		((RingBufferHttpStream*)userdata)->OnDataCallback(r, data, n);
	}

	static void OnCompleteCallback(const happyhttp::Response* r, void* userdata)
	{
		((RingBufferHttpStream*)userdata)->OnCompleteCallback(r);
	}

private:
	void OnBeginCallback(const happyhttp::Response* r);
	void OnDataCallback(const happyhttp::Response* r, const unsigned char* data, int n);
	void OnCompleteCallback(const happyhttp::Response* r);

	void ReadFromBuffer(uint8_t* dst, int count);
	void WriteToBuffer(uint8_t* src, int count);

};

#endif