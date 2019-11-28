#include <nds.h>
#include "mpeg4.h"
#include "mpeg4_header.s"

typedef uint8_t PIXEL;
typedef uint32_t U32;

#define HX_MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define HX_MIN(a, b)  (((a) < (b)) ? (a) : (b))

#define LIMIT( low, x, high )    HX_MAX( low, HX_MIN( x, high ))

static ITCM_CODE void limitMC( int hSize, int vSize,
                    PIXEL const *in, PIXEL *out, int hdim,
                    int mvX, int mvY,   // Motion vector
                    int minX, int maxX, int minY, int maxY, // Limits for hor/vert indices
                    int roundingControl
                    )
{
#define MAX_HSIZE   (16)
    int intX, intY, fracX, fracY, outsideTop, outsideBot, repeatTop, repeatBot, x, y;
    static int  mapX[MAX_HSIZE + 1];
    PIXEL *outSave, *outBegin;
    union { // Copy words to speed up routine
        PIXEL   *pix;
        U32     *word;
    } pIn, pOut;

    if (hSize & 0x3) {
       // H261ErrMsg("limitMC -- hSize must be multiple of 4");
        return;
    }
    if (hSize > MAX_HSIZE) {
       // H261ErrMsg("limitMC -- hSize too large");
        return;
    }
    intX = mvX >> 1;    // Integer part of motion vector
    intY = mvY >> 1;
    fracX = mvX & 0x1;  // Fractional part of motion vector
    fracY = mvY & 0x1;
    // Create horizontal mapping vector
    for (x = 0; x <= hSize; ++x) {
        mapX[x] = LIMIT( minX, x + intX, maxX );
    }
    //repeatTop = HX_MAX( 0, minY - intY);                     // Lines on top that are outside
    //repeatBot = HX_MAX( 0, vSize - 1 + intY + fracY - maxY); // Lines at bottom that are outside
    outsideTop = HX_MAX( 0, minY - intY);						// Lines on top that are outside
    outsideBot = HX_MAX( 0, vSize - 1 + intY + fracY - maxY);  // Lines at bottom that are outside
	// Don't produce more lines than the blocksize (used to be a nasty bug hidden here)
	repeatTop = HX_MIN( outsideTop, vSize );
    if (outsideBot < vSize) {
        repeatBot = outsideBot;
        in += (intY + outsideTop) * hdim;	// Apply vert motion comp. (hor MC thru mapping)
    } else {    // Whole block is "outside" bottom of picture
        repeatBot = vSize;
        in += /*(vSize - 1)*/(intY - 1) * hdim;   // Point to last line of picture
    }
	// Output pointers
    outSave = out;						// Upper left corner of output block
    out += repeatTop * hdim;			// "Repeated" lines will be filled in later
    outBegin = out;						// Save address for first valid output line
    if (fracY == 0) {
		// Ensure that at least one output line gets written
		if (repeatTop == vSize) {
			--repeatTop;
			out -= hdim;
			outBegin = out;
		} else if (repeatBot == vSize) {
			--repeatBot;
		}
        if (fracX == 0) {
            // No interpolation
            for (y = repeatTop; y < vSize - repeatBot; ++y) {
                for (x = 0; x < hSize; x += 4) {
                    out[x+0] = in[ mapX[x+0] ];
                    out[x+1] = in[ mapX[x+1] ];
                    out[x+2] = in[ mapX[x+2] ];
                    out[x+3] = in[ mapX[x+3] ];
                }
                in += hdim;
                out += hdim;
            }
        } else {
            // Horizontal interpolation
            for (y = repeatTop; y < vSize - repeatBot; ++y) {
                for (x = 0; x < hSize; x += 4) {
                    out[x+0] = (in[mapX[x+0]] + in[mapX[x+1]] + 1 - roundingControl) >> 1;
                    out[x+1] = (in[mapX[x+1]] + in[mapX[x+2]] + 1 - roundingControl) >> 1;
                    out[x+2] = (in[mapX[x+2]] + in[mapX[x+3]] + 1 - roundingControl) >> 1;
                    out[x+3] = (in[mapX[x+3]] + in[mapX[x+4]] + 1 - roundingControl) >> 1;
                }
                in += hdim;
                out += hdim;
            }
        }
    } else if (fracX == 0) {
        // Vertical interpolation
        if (repeatTop > 0) {    // Produce line to repeat
            outBegin = out - hdim;
            for (x = 0; x < hSize; ++x) {
                outBegin[x] = in[ mapX[x] ];
            }
        }
        for (y = repeatTop; y < vSize - repeatBot; ++y) {
            for (x = 0; x < hSize; x += 4) {
                out[x+0] = (in[mapX[x+0]] + in[mapX[x+0] + hdim] + 1 - roundingControl) >> 1;
                out[x+1] = (in[mapX[x+1]] + in[mapX[x+1] + hdim] + 1 - roundingControl) >> 1;
                out[x+2] = (in[mapX[x+2]] + in[mapX[x+2] + hdim] + 1 - roundingControl) >> 1;
                out[x+3] = (in[mapX[x+3]] + in[mapX[x+3] + hdim] + 1 - roundingControl) >> 1;
            }
            in += hdim;
            out += hdim;
        }
        if (repeatBot > 0) {    // Produce line to repeat
            for (x = 0; x < hSize; ++x) {
                out[x] = in[ mapX[x] ];
            }
            out += hdim;
        }
    } else {    // Bilinear interpolation
        if (repeatTop > 0) {    // Produce line to repeat
            outBegin = out - hdim;
            for (x = 0; x < hSize; ++x) {
                outBegin[x] = (in[mapX[x]] + in[mapX[x+1]] + 1 - roundingControl) >> 1;
            }
        }
        for (y = repeatTop; y < vSize - repeatBot; ++y) {
            for (x = 0; x < hSize; x += 4) {
                    out[x+0] = (in[mapX[x+0]] + in[mapX[x+0] + hdim]
                                + in[mapX[x+1]] + in[mapX[x+1] + hdim] + 2 - roundingControl) >> 2;
                    out[x+1] = (in[mapX[x+1]] + in[mapX[x+1] + hdim]
                                + in[mapX[x+2]] + in[mapX[x+2] + hdim] + 2 - roundingControl) >> 2;
                    out[x+2] = (in[mapX[x+2]] + in[mapX[x+2] + hdim]
                                + in[mapX[x+3]] + in[mapX[x+3] + hdim] + 2 - roundingControl) >> 2;
                    out[x+3] = (in[mapX[x+3]] + in[mapX[x+3] + hdim]
                                + in[mapX[x+4]] + in[mapX[x+4] + hdim] + 2 - roundingControl) >> 2;
            }
            in += hdim;
            out += hdim;
        }
        if (repeatBot > 0) {    // Produce line to repeat
            for (x = 0; x < hSize; ++x) {
                out[x] = (in[mapX[x]] + in[mapX[x+1]] + 1 - roundingControl) >> 1;
            }
            out += hdim;
        }
    }
    if (fracY == 1) {
        --repeatTop;    // Already did one line
        --repeatBot;
    }
    // Repeat first line at top
    pIn.pix = outBegin;
    pOut.pix = outSave;
    for (y = 0; y < repeatTop; ++y) {
        for (x = 0; x < (hSize >> 2); ++x) {
            *(pOut.word + x) = *(pIn.word + x);
        }
        pOut.pix += hdim;
    }
    // Repeat last line at the bottom
    pIn.pix = out - hdim;
    pOut.pix = out;
    for (y = 0; y < repeatBot; ++y) {
        for (x = 0; x < (hSize >> 2); ++x) {
            *(pOut.word + x) = *(pIn.word + x);
        }
        pOut.pix += hdim;
    }
    return;
}

extern "C" ITCM_CODE void __attribute__((noinline)) __attribute__((optimize("-O1"))) mpeg4_blockcopy_16x16_tmp(mpeg4_dec_struct* context, uint32_t r8, int dx, int dy)
{
	limitMC(16, 16, context->pPrevY + r8, context->pDstY + r8, FB_STRIDE, dx, dy,
		-(r8 & ((1 << FB_STRIDE_SHIFT) - 1)), context->width - 1 - (r8 & ((1 << FB_STRIDE_SHIFT) - 1)), -(r8 >> FB_STRIDE_SHIFT), context->height - 1 - (r8 >> FB_STRIDE_SHIFT), context->vop_rounding_control);//0, context->width, 0, context->height);
}

extern "C" ITCM_CODE void __attribute__((noinline)) __attribute__((optimize("-O1"))) mpeg4_blockcopy_8x8_Y_tmp(mpeg4_dec_struct* context, uint32_t r8, int dx, int dy)
{
	limitMC(8, 8, context->pPrevY + r8, context->pDstY + r8, FB_STRIDE, dx, dy,
		-(r8 & ((1 << FB_STRIDE_SHIFT) - 1)), context->width - 1 - (r8 & ((1 << FB_STRIDE_SHIFT) - 1)), -(r8 >> FB_STRIDE_SHIFT), context->height - 1 - (r8 >> FB_STRIDE_SHIFT), context->vop_rounding_control);//0, context->width, 0, context->height);
}

extern "C" ITCM_CODE void __attribute__((noinline)) __attribute__((optimize("-O1"))) mpeg4_blockcopy_8x8_UV_tmp(mpeg4_dec_struct* context, uint32_t r8, int dx, int dy)
{
	limitMC(8, 8, context->pPrevUV + r8, context->pDstUV + r8, FB_STRIDE, dx, dy,
		-(r8 & ((1 << (FB_STRIDE_SHIFT - 1)) - 1)), (context->width >> 1) - 1 - (r8 & ((1 << (FB_STRIDE_SHIFT - 1)) - 1)), -(r8 >> FB_STRIDE_SHIFT), (context->height >> 1) - 1 - (r8 >> FB_STRIDE_SHIFT), context->vop_rounding_control);//0, context->width, 0, context->height);
}