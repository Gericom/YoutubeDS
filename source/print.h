#ifndef __PRINT_H__
#define __PRINT_H__

#ifdef __cplusplus
extern "C" {
#endif

	void nocashPrint(const char* txt);
	void nocashPrint1(const char* txt, u32 r0);
	void nocashPrint2(const char* txt, u32 r0, u32 r1);
	void nocashPrint3(const char* txt, u32 r0, u32 r1, u32 r2);

#ifdef __cplusplus
}
#endif

#endif