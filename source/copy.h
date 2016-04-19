#ifndef __COPY_H__
#define __COPY_H__

extern "C" void MI_CpuCopy8(const void *src, void *dest, u32 size);
extern "C" void MI_CpuCopyFast(const void *src, void *dest, u32 size);
extern "C" void MI_CpuFillFast(void *dest, u32 data, u32 size);

#endif