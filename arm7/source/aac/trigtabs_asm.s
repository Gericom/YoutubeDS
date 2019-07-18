#define SINWINDOW_SIZE  ((128 + 1024) << 2)
.global raac_sinWindow
.set raac_sinWindow, 0x06000000

#define KBDWINDOW_SIZE  ((128 + 1024) << 2)
.global raac_kbdWindow
.set raac_kbdWindow, (raac_sinWindow + SINWINDOW_SIZE)

#define TWIDTABODD_SIZE  ((8*6 + 32*6 + 128*6) << 2)
.global raac_twidTabOdd
.set raac_twidTabOdd, (raac_kbdWindow + KBDWINDOW_SIZE)

#define TWIDTABEVEN_SIZE  ((4*6 + 16*6 + 64*6) << 2)
.global raac_twidTabEven
.set raac_twidTabEven, (raac_twidTabOdd + TWIDTABODD_SIZE)

.global aac_audioBuffer
.set aac_audioBuffer, (raac_twidTabEven + TWIDTABEVEN_SIZE)