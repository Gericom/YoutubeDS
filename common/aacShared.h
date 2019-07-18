#pragma once

#define FIFO_AAC                    FIFO_USER_01

#define AAC_FIFO_CMD_SET_QUEUE      0
#define AAC_FIFO_CMD_DECSTART       1
#define AAC_FIFO_CMD_DECSTOP        2
#define AAC_FIFO_CMD_NOTIFY_BLOCK   3

#define AAC_QUEUE_BLOCK_SIZE		(2 * 1024)
#define AAC_QUEUE_BLOCK_COUNT	    20

struct aac_queue_t
{
    u8 queue[AAC_QUEUE_BLOCK_COUNT][AAC_QUEUE_BLOCK_SIZE];
    u32 queueBlockLength[AAC_QUEUE_BLOCK_COUNT];
    u8 readBlock;
    u8 writeBlock;
    volatile u8 countLock;
    volatile u8 blockCount;
} __attribute__ ((aligned (32)));
