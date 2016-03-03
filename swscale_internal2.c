 /*
   2  * Copyright (C) 2001-2011 Michael Niedermayer <michaelni@gmx.at>
   3  *
   4  * This file is part of FFmpeg.
   5  *
   6  * FFmpeg is free software; you can redistribute it and/or
   7  * modify it under the terms of the GNU Lesser General Public
   8  * License as published by the Free Software Foundation; either
   9  * version 2.1 of the License, or (at your option) any later version.
  10  *
  11  * FFmpeg is distributed in the hope that it will be useful,
  12  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  13  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  14  * Lesser General Public License for more details.
  15  *
  16  * You should have received a copy of the GNU Lesser General Public
  17  * License along with FFmpeg; if not, write to the Free Software
  18  * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
  19  */
  20 
  21 #ifndef SWSCALE_SWSCALE_INTERNAL_H
  22 #define SWSCALE_SWSCALE_INTERNAL_H
  23 
  24 #include "config.h"
  25 
  26 #if HAVE_ALTIVEC_H
  27 #include <altivec.h>
  28 #endif
  29 
  30 #include "libavutil/avassert.h"
  31 #include "libavutil/avutil.h"
  32 #include "libavutil/common.h"
  33 #include "libavutil/intreadwrite.h"
  34 #include "libavutil/log.h"
  35 #include "libavutil/pixfmt.h"
  36 #include "libavutil/pixdesc.h"
  37 
  38 #define STR(s) AV_TOSTRING(s) // AV_STRINGIFY is too long
  39 
  40 #define YUVRGB_TABLE_HEADROOM 128
  41 
  42 #define MAX_FILTER_SIZE SWS_MAX_FILTER_SIZE
  43 
  44 #define DITHER1XBPP
  45 
  46 #if HAVE_BIGENDIAN
  47 #define ALT32_CORR (-1)
  48 #else
  49 #define ALT32_CORR   1
  50 #endif
  51 
  52 #if ARCH_X86_64
  53 #   define APCK_PTR2  8
  54 #   define APCK_COEF 16
  55 #   define APCK_SIZE 24
  56 #else
  57 #   define APCK_PTR2  4
  58 #   define APCK_COEF  8
  59 #   define APCK_SIZE 16
  60 #endif
  61 
  62 struct SwsContext;
  63 
  64 typedef enum SwsDither {
  65     SWS_DITHER_NONE = 0,
  66     SWS_DITHER_AUTO,
  67     SWS_DITHER_BAYER,
  68     SWS_DITHER_ED,
  69     SWS_DITHER_A_DITHER,
  70     SWS_DITHER_X_DITHER,
  71     NB_SWS_DITHER,
  72 } SwsDither;
  73 
  74 typedef int (*SwsFunc)(struct SwsContext *context, const uint8_t *src[],
  75                        int srcStride[], int srcSliceY, int srcSliceH,
  76                        uint8_t *dst[], int dstStride[]);
  77 
  78 /**
  79  * Write one line of horizontally scaled data to planar output
  80  * without any additional vertical scaling (or point-scaling).
  81  *
  82  * @param src     scaled source data, 15bit for 8-10bit output,
  83  *                19-bit for 16bit output (in int32_t)
  84  * @param dest    pointer to the output plane. For >8bit
  85  *                output, this is in uint16_t
  86  * @param dstW    width of destination in pixels
  87  * @param dither  ordered dither array of type int16_t and size 8
  88  * @param offset  Dither offset
  89  */
  90 typedef void (*yuv2planar1_fn)(const int16_t *src, uint8_t *dest, int dstW,
  91                                const uint8_t *dither, int offset);
  92 
  93 /**
  94  * Write one line of horizontally scaled data to planar output
  95  * with multi-point vertical scaling between input pixels.
  96  *
  97  * @param filter        vertical luma/alpha scaling coefficients, 12bit [0,4096]
  98  * @param src           scaled luma (Y) or alpha (A) source data, 15bit for 8-10bit output,
  99  *                      19-bit for 16bit output (in int32_t)
 100  * @param filterSize    number of vertical input lines to scale
 101  * @param dest          pointer to output plane. For >8bit
 102  *                      output, this is in uint16_t
 103  * @param dstW          width of destination pixels
 104  * @param offset        Dither offset
 105  */
 106 typedef void (*yuv2planarX_fn)(const int16_t *filter, int filterSize,
 107                                const int16_t **src, uint8_t *dest, int dstW,
 108                                const uint8_t *dither, int offset);
 109 
 110 /**
 111  * Write one line of horizontally scaled chroma to interleaved output
 112  * with multi-point vertical scaling between input pixels.
 113  *
 114  * @param c             SWS scaling context
 115  * @param chrFilter     vertical chroma scaling coefficients, 12bit [0,4096]
 116  * @param chrUSrc       scaled chroma (U) source data, 15bit for 8-10bit output,
 117  *                      19-bit for 16bit output (in int32_t)
 118  * @param chrVSrc       scaled chroma (V) source data, 15bit for 8-10bit output,
 119  *                      19-bit for 16bit output (in int32_t)
 120  * @param chrFilterSize number of vertical chroma input lines to scale
 121  * @param dest          pointer to the output plane. For >8bit
 122  *                      output, this is in uint16_t
 123  * @param dstW          width of chroma planes
 124  */
 125 typedef void (*yuv2interleavedX_fn)(struct SwsContext *c,
 126                                     const int16_t *chrFilter,
 127                                     int chrFilterSize,
 128                                     const int16_t **chrUSrc,
 129                                     const int16_t **chrVSrc,
 130                                     uint8_t *dest, int dstW);
 131 
 132 /**
 133  * Write one line of horizontally scaled Y/U/V/A to packed-pixel YUV/RGB
 134  * output without any additional vertical scaling (or point-scaling). Note
 135  * that this function may do chroma scaling, see the "uvalpha" argument.
 136  *
 137  * @param c       SWS scaling context
 138  * @param lumSrc  scaled luma (Y) source data, 15bit for 8-10bit output,
 139  *                19-bit for 16bit output (in int32_t)
 140  * @param chrUSrc scaled chroma (U) source data, 15bit for 8-10bit output,
 141  *                19-bit for 16bit output (in int32_t)
 142  * @param chrVSrc scaled chroma (V) source data, 15bit for 8-10bit output,
 143  *                19-bit for 16bit output (in int32_t)
 144  * @param alpSrc  scaled alpha (A) source data, 15bit for 8-10bit output,
 145  *                19-bit for 16bit output (in int32_t)
 146  * @param dest    pointer to the output plane. For 16bit output, this is
 147  *                uint16_t
 148  * @param dstW    width of lumSrc and alpSrc in pixels, number of pixels
 149  *                to write into dest[]
 150  * @param uvalpha chroma scaling coefficient for the second line of chroma
 151  *                pixels, either 2048 or 0. If 0, one chroma input is used
 152  *                for 2 output pixels (or if the SWS_FLAG_FULL_CHR_INT flag
 153  *                is set, it generates 1 output pixel). If 2048, two chroma
 154  *                input pixels should be averaged for 2 output pixels (this
 155  *                only happens if SWS_FLAG_FULL_CHR_INT is not set)
 156  * @param y       vertical line number for this output. This does not need
 157  *                to be used to calculate the offset in the destination,
 158  *                but can be used to generate comfort noise using dithering
 159  *                for some output formats.
 160  */
 161 typedef void (*yuv2packed1_fn)(struct SwsContext *c, const int16_t *lumSrc,
 162                                const int16_t *chrUSrc[2],
 163                                const int16_t *chrVSrc[2],
 164                                const int16_t *alpSrc, uint8_t *dest,
 165                                int dstW, int uvalpha, int y);
 166 /**
 167  * Write one line of horizontally scaled Y/U/V/A to packed-pixel YUV/RGB
 168  * output by doing bilinear scaling between two input lines.
 169  *
 170  * @param c       SWS scaling context
 171  * @param lumSrc  scaled luma (Y) source data, 15bit for 8-10bit output,
 172  *                19-bit for 16bit output (in int32_t)
 173  * @param chrUSrc scaled chroma (U) source data, 15bit for 8-10bit output,
 174  *                19-bit for 16bit output (in int32_t)
 175  * @param chrVSrc scaled chroma (V) source data, 15bit for 8-10bit output,
 176  *                19-bit for 16bit output (in int32_t)
 177  * @param alpSrc  scaled alpha (A) source data, 15bit for 8-10bit output,
 178  *                19-bit for 16bit output (in int32_t)
 179  * @param dest    pointer to the output plane. For 16bit output, this is
 180  *                uint16_t
 181  * @param dstW    width of lumSrc and alpSrc in pixels, number of pixels
 182  *                to write into dest[]
 183  * @param yalpha  luma/alpha scaling coefficients for the second input line.
 184  *                The first line's coefficients can be calculated by using
 185  *                4096 - yalpha
 186  * @param uvalpha chroma scaling coefficient for the second input line. The
 187  *                first line's coefficients can be calculated by using
 188  *                4096 - uvalpha
 189  * @param y       vertical line number for this output. This does not need
 190  *                to be used to calculate the offset in the destination,
 191  *                but can be used to generate comfort noise using dithering
 192  *                for some output formats.
 193  */
 194 typedef void (*yuv2packed2_fn)(struct SwsContext *c, const int16_t *lumSrc[2],
 195                                const int16_t *chrUSrc[2],
 196                                const int16_t *chrVSrc[2],
 197                                const int16_t *alpSrc[2],
 198                                uint8_t *dest,
 199                                int dstW, int yalpha, int uvalpha, int y);
 200 /**
 201  * Write one line of horizontally scaled Y/U/V/A to packed-pixel YUV/RGB
 202  * output by doing multi-point vertical scaling between input pixels.
 203  *
 204  * @param c             SWS scaling context
 205  * @param lumFilter     vertical luma/alpha scaling coefficients, 12bit [0,4096]
 206  * @param lumSrc        scaled luma (Y) source data, 15bit for 8-10bit output,
 207  *                      19-bit for 16bit output (in int32_t)
 208  * @param lumFilterSize number of vertical luma/alpha input lines to scale
 209  * @param chrFilter     vertical chroma scaling coefficients, 12bit [0,4096]
 210  * @param chrUSrc       scaled chroma (U) source data, 15bit for 8-10bit output,
 211  *                      19-bit for 16bit output (in int32_t)
 212  * @param chrVSrc       scaled chroma (V) source data, 15bit for 8-10bit output,
 213  *                      19-bit for 16bit output (in int32_t)
 214  * @param chrFilterSize number of vertical chroma input lines to scale
 215  * @param alpSrc        scaled alpha (A) source data, 15bit for 8-10bit output,
 216  *                      19-bit for 16bit output (in int32_t)
 217  * @param dest          pointer to the output plane. For 16bit output, this is
 218  *                      uint16_t
 219  * @param dstW          width of lumSrc and alpSrc in pixels, number of pixels
 220  *                      to write into dest[]
 221  * @param y             vertical line number for this output. This does not need
 222  *                      to be used to calculate the offset in the destination,
 223  *                      but can be used to generate comfort noise using dithering
 224  *                      or some output formats.
 225  */
 226 typedef void (*yuv2packedX_fn)(struct SwsContext *c, const int16_t *lumFilter,
 227                                const int16_t **lumSrc, int lumFilterSize,
 228                                const int16_t *chrFilter,
 229                                const int16_t **chrUSrc,
 230                                const int16_t **chrVSrc, int chrFilterSize,
 231                                const int16_t **alpSrc, uint8_t *dest,
 232                                int dstW, int y);
 233 
 234 /**
 235  * Write one line of horizontally scaled Y/U/V/A to YUV/RGB
 236  * output by doing multi-point vertical scaling between input pixels.
 237  *
 238  * @param c             SWS scaling context
 239  * @param lumFilter     vertical luma/alpha scaling coefficients, 12bit [0,4096]
 240  * @param lumSrc        scaled luma (Y) source data, 15bit for 8-10bit output,
 241  *                      19-bit for 16bit output (in int32_t)
 242  * @param lumFilterSize number of vertical luma/alpha input lines to scale
 243  * @param chrFilter     vertical chroma scaling coefficients, 12bit [0,4096]
 244  * @param chrUSrc       scaled chroma (U) source data, 15bit for 8-10bit output,
 245  *                      19-bit for 16bit output (in int32_t)
 246  * @param chrVSrc       scaled chroma (V) source data, 15bit for 8-10bit output,
 247  *                      19-bit for 16bit output (in int32_t)
 248  * @param chrFilterSize number of vertical chroma input lines to scale
 249  * @param alpSrc        scaled alpha (A) source data, 15bit for 8-10bit output,
 250  *                      19-bit for 16bit output (in int32_t)
 251  * @param dest          pointer to the output planes. For 16bit output, this is
 252  *                      uint16_t
 253  * @param dstW          width of lumSrc and alpSrc in pixels, number of pixels
 254  *                      to write into dest[]
 255  * @param y             vertical line number for this output. This does not need
 256  *                      to be used to calculate the offset in the destination,
 257  *                      but can be used to generate comfort noise using dithering
 258  *                      or some output formats.
 259  */
 260 typedef void (*yuv2anyX_fn)(struct SwsContext *c, const int16_t *lumFilter,
 261                             const int16_t **lumSrc, int lumFilterSize,
 262                             const int16_t *chrFilter,
 263                             const int16_t **chrUSrc,
 264                             const int16_t **chrVSrc, int chrFilterSize,
 265                             const int16_t **alpSrc, uint8_t **dest,
 266                             int dstW, int y);
 267 
 268 /* This struct should be aligned on at least a 32-byte boundary. */
 269 typedef struct SwsContext {
 270     /**
 271      * info on struct for av_log
 272      */
 273     const AVClass *av_class;
 274 
 275     /**
 276      * Note that src, dst, srcStride, dstStride will be copied in the
 277      * sws_scale() wrapper so they can be freely modified here.
 278      */
 279     SwsFunc swscale;
 280     int srcW;                     ///< Width  of source      luma/alpha planes.
 281     int srcH;                     ///< Height of source      luma/alpha planes.
 282     int dstH;                     ///< Height of destination luma/alpha planes.
 283     int chrSrcW;                  ///< Width  of source      chroma     planes.
 284     int chrSrcH;                  ///< Height of source      chroma     planes.
 285     int chrDstW;                  ///< Width  of destination chroma     planes.
 286     int chrDstH;                  ///< Height of destination chroma     planes.
 287     int lumXInc, chrXInc;
 288     int lumYInc, chrYInc;
 289     enum AVPixelFormat dstFormat; ///< Destination pixel format.
 290     enum AVPixelFormat srcFormat; ///< Source      pixel format.
 291     int dstFormatBpp;             ///< Number of bits per pixel of the destination pixel format.
 292     int srcFormatBpp;             ///< Number of bits per pixel of the source      pixel format.
 293     int dstBpc, srcBpc;
 294     int chrSrcHSubSample;         ///< Binary logarithm of horizontal subsampling factor between luma/alpha and chroma planes in source      image.
 295     int chrSrcVSubSample;         ///< Binary logarithm of vertical   subsampling factor between luma/alpha and chroma planes in source      image.
 296     int chrDstHSubSample;         ///< Binary logarithm of horizontal subsampling factor between luma/alpha and chroma planes in destination image.
 297     int chrDstVSubSample;         ///< Binary logarithm of vertical   subsampling factor between luma/alpha and chroma planes in destination image.
 298     int vChrDrop;                 ///< Binary logarithm of extra vertical subsampling factor in source image chroma planes specified by user.
 299     int sliceDir;                 ///< Direction that slices are fed to the scaler (1 = top-to-bottom, -1 = bottom-to-top).
 300     double param[2];              ///< Input parameters for scaling algorithms that need them.
 301 
 302     uint32_t pal_yuv[256];
 303     uint32_t pal_rgb[256];
 304 
 305     /**
 306      * @name Scaled horizontal lines ring buffer.
 307      * The horizontal scaler keeps just enough scaled lines in a ring buffer
 308      * so they may be passed to the vertical scaler. The pointers to the
 309      * allocated buffers for each line are duplicated in sequence in the ring
 310      * buffer to simplify indexing and avoid wrapping around between lines
 311      * inside the vertical scaler code. The wrapping is done before the
 312      * vertical scaler is called.
 313      */
 314     //@{
 315     int16_t **lumPixBuf;          ///< Ring buffer for scaled horizontal luma   plane lines to be fed to the vertical scaler.
 316     int16_t **chrUPixBuf;         ///< Ring buffer for scaled horizontal chroma plane lines to be fed to the vertical scaler.
 317     int16_t **chrVPixBuf;         ///< Ring buffer for scaled horizontal chroma plane lines to be fed to the vertical scaler.
 318     int16_t **alpPixBuf;          ///< Ring buffer for scaled horizontal alpha  plane lines to be fed to the vertical scaler.
 319     int vLumBufSize;              ///< Number of vertical luma/alpha lines allocated in the ring buffer.
 320     int vChrBufSize;              ///< Number of vertical chroma     lines allocated in the ring buffer.
 321     int lastInLumBuf;             ///< Last scaled horizontal luma/alpha line from source in the ring buffer.
 322     int lastInChrBuf;             ///< Last scaled horizontal chroma     line from source in the ring buffer.
 323     int lumBufIndex;              ///< Index in ring buffer of the last scaled horizontal luma/alpha line from source.
 324     int chrBufIndex;              ///< Index in ring buffer of the last scaled horizontal chroma     line from source.
 325     //@}
 326 
 327     uint8_t *formatConvBuffer;
 328 
 329     /**
 330      * @name Horizontal and vertical filters.
 331      * To better understand the following fields, here is a pseudo-code of
 332      * their usage in filtering a horizontal line:
 333      * @code
 334      * for (i = 0; i < width; i++) {
 335      *     dst[i] = 0;
 336      *     for (j = 0; j < filterSize; j++)
 337      *         dst[i] += src[ filterPos[i] + j ] * filter[ filterSize * i + j ];
 338      *     dst[i] >>= FRAC_BITS; // The actual implementation is fixed-point.
 339      * }
 340      * @endcode
 341      */
 342     //@{
 343     int16_t *hLumFilter;          ///< Array of horizontal filter coefficients for luma/alpha planes.
 344     int16_t *hChrFilter;          ///< Array of horizontal filter coefficients for chroma     planes.
 345     int16_t *vLumFilter;          ///< Array of vertical   filter coefficients for luma/alpha planes.
 346     int16_t *vChrFilter;          ///< Array of vertical   filter coefficients for chroma     planes.
 347     int32_t *hLumFilterPos;       ///< Array of horizontal filter starting positions for each dst[i] for luma/alpha planes.
 348     int32_t *hChrFilterPos;       ///< Array of horizontal filter starting positions for each dst[i] for chroma     planes.
 349     int32_t *vLumFilterPos;       ///< Array of vertical   filter starting positions for each dst[i] for luma/alpha planes.
 350     int32_t *vChrFilterPos;       ///< Array of vertical   filter starting positions for each dst[i] for chroma     planes.
 351     int hLumFilterSize;           ///< Horizontal filter size for luma/alpha pixels.
 352     int hChrFilterSize;           ///< Horizontal filter size for chroma     pixels.
 353     int vLumFilterSize;           ///< Vertical   filter size for luma/alpha pixels.
 354     int vChrFilterSize;           ///< Vertical   filter size for chroma     pixels.
 355     //@}
 356 
 357     int lumMmxextFilterCodeSize;  ///< Runtime-generated MMXEXT horizontal fast bilinear scaler code size for luma/alpha planes.
 358     int chrMmxextFilterCodeSize;  ///< Runtime-generated MMXEXT horizontal fast bilinear scaler code size for chroma planes.
 359     uint8_t *lumMmxextFilterCode; ///< Runtime-generated MMXEXT horizontal fast bilinear scaler code for luma/alpha planes.
 360     uint8_t *chrMmxextFilterCode; ///< Runtime-generated MMXEXT horizontal fast bilinear scaler code for chroma planes.
 361 
 362     int canMMXEXTBeUsed;
 363 
 364     int dstY;                     ///< Last destination vertical line output from last slice.
 365     int flags;                    ///< Flags passed by the user to select scaler algorithm, optimizations, subsampling, etc...
 366     void *yuvTable;             // pointer to the yuv->rgb table start so it can be freed()
 367     // alignment ensures the offset can be added in a single
 368     // instruction on e.g. ARM
 369     DECLARE_ALIGNED(16, int, table_gV)[256 + 2*YUVRGB_TABLE_HEADROOM];
 370     uint8_t *table_rV[256 + 2*YUVRGB_TABLE_HEADROOM];
 371     uint8_t *table_gU[256 + 2*YUVRGB_TABLE_HEADROOM];
 372     uint8_t *table_bU[256 + 2*YUVRGB_TABLE_HEADROOM];
 373     DECLARE_ALIGNED(16, int32_t, input_rgb2yuv_table)[16+40*4]; // This table can contain both C and SIMD formatted values, the C vales are always at the XY_IDX points
 374 #define RY_IDX 0
 375 #define GY_IDX 1
 376 #define BY_IDX 2
 377 #define RU_IDX 3
 378 #define GU_IDX 4
 379 #define BU_IDX 5
 380 #define RV_IDX 6
 381 #define GV_IDX 7
 382 #define BV_IDX 8
 383 #define RGB2YUV_SHIFT 15
 384 
 385     int *dither_error[4];
 386 
 387     //Colorspace stuff
 388     int contrast, brightness, saturation;    // for sws_getColorspaceDetails
 389     int srcColorspaceTable[4];
 390     int dstColorspaceTable[4];
 391     int srcRange;                 ///< 0 = MPG YUV range, 1 = JPG YUV range (source      image).
 392     int dstRange;                 ///< 0 = MPG YUV range, 1 = JPG YUV range (destination image).
 393     int src0Alpha;
 394     int dst0Alpha;
 395     int srcXYZ;
 396     int dstXYZ;
 397     int src_h_chr_pos;
 398     int dst_h_chr_pos;
 399     int src_v_chr_pos;
 400     int dst_v_chr_pos;
 401     int yuv2rgb_y_offset;
 402     int yuv2rgb_y_coeff;
 403     int yuv2rgb_v2r_coeff;
 404     int yuv2rgb_v2g_coeff;
 405     int yuv2rgb_u2g_coeff;
 406     int yuv2rgb_u2b_coeff;
 407 
 408 #define RED_DITHER            "0*8"
 409 #define GREEN_DITHER          "1*8"
 410 #define BLUE_DITHER           "2*8"
 411 #define Y_COEFF               "3*8"
 412 #define VR_COEFF              "4*8"
 413 #define UB_COEFF              "5*8"
 414 #define VG_COEFF              "6*8"
 415 #define UG_COEFF              "7*8"
 416 #define Y_OFFSET              "8*8"
 417 #define U_OFFSET              "9*8"
 418 #define V_OFFSET              "10*8"
 419 #define LUM_MMX_FILTER_OFFSET "11*8"
 420 #define CHR_MMX_FILTER_OFFSET "11*8+4*4*"AV_STRINGIFY(MAX_FILTER_SIZE)
 421 #define DSTW_OFFSET           "11*8+4*4*"AV_STRINGIFY(MAX_FILTER_SIZE)"*2"
 422 #define ESP_OFFSET            "11*8+4*4*"AV_STRINGIFY(MAX_FILTER_SIZE)"*2+8"
 423 #define VROUNDER_OFFSET       "11*8+4*4*"AV_STRINGIFY(MAX_FILTER_SIZE)"*2+16"
 424 #define U_TEMP                "11*8+4*4*"AV_STRINGIFY(MAX_FILTER_SIZE)"*2+24"
 425 #define V_TEMP                "11*8+4*4*"AV_STRINGIFY(MAX_FILTER_SIZE)"*2+32"
 426 #define Y_TEMP                "11*8+4*4*"AV_STRINGIFY(MAX_FILTER_SIZE)"*2+40"
 427 #define ALP_MMX_FILTER_OFFSET "11*8+4*4*"AV_STRINGIFY(MAX_FILTER_SIZE)"*2+48"
 428 #define UV_OFF_PX             "11*8+4*4*"AV_STRINGIFY(MAX_FILTER_SIZE)"*3+48"
 429 #define UV_OFF_BYTE           "11*8+4*4*"AV_STRINGIFY(MAX_FILTER_SIZE)"*3+56"
 430 #define DITHER16              "11*8+4*4*"AV_STRINGIFY(MAX_FILTER_SIZE)"*3+64"
 431 #define DITHER32              "11*8+4*4*"AV_STRINGIFY(MAX_FILTER_SIZE)"*3+80"
 432 #define DITHER32_INT          (11*8+4*4*MAX_FILTER_SIZE*3+80) // value equal to above, used for checking that the struct hasnt been changed by mistake
 433 
 434     DECLARE_ALIGNED(8, uint64_t, redDither);
 435     DECLARE_ALIGNED(8, uint64_t, greenDither);
 436     DECLARE_ALIGNED(8, uint64_t, blueDither);
 437 
 438     DECLARE_ALIGNED(8, uint64_t, yCoeff);
 439     DECLARE_ALIGNED(8, uint64_t, vrCoeff);
 440     DECLARE_ALIGNED(8, uint64_t, ubCoeff);
 441     DECLARE_ALIGNED(8, uint64_t, vgCoeff);
 442     DECLARE_ALIGNED(8, uint64_t, ugCoeff);
 443     DECLARE_ALIGNED(8, uint64_t, yOffset);
 444     DECLARE_ALIGNED(8, uint64_t, uOffset);
 445     DECLARE_ALIGNED(8, uint64_t, vOffset);
 446     int32_t lumMmxFilter[4 * MAX_FILTER_SIZE];
 447     int32_t chrMmxFilter[4 * MAX_FILTER_SIZE];
 448     int dstW;                     ///< Width  of destination luma/alpha planes.
 449     DECLARE_ALIGNED(8, uint64_t, esp);
 450     DECLARE_ALIGNED(8, uint64_t, vRounder);
 451     DECLARE_ALIGNED(8, uint64_t, u_temp);
 452     DECLARE_ALIGNED(8, uint64_t, v_temp);
 453     DECLARE_ALIGNED(8, uint64_t, y_temp);
 454     int32_t alpMmxFilter[4 * MAX_FILTER_SIZE];
 455     // alignment of these values is not necessary, but merely here
 456     // to maintain the same offset across x8632 and x86-64. Once we
 457     // use proper offset macros in the asm, they can be removed.
 458     DECLARE_ALIGNED(8, ptrdiff_t, uv_off); ///< offset (in pixels) between u and v planes
 459     DECLARE_ALIGNED(8, ptrdiff_t, uv_offx2); ///< offset (in bytes) between u and v planes
 460     DECLARE_ALIGNED(8, uint16_t, dither16)[8];
 461     DECLARE_ALIGNED(8, uint32_t, dither32)[8];
 462 
 463     const uint8_t *chrDither8, *lumDither8;
 464 
 465 #if HAVE_ALTIVEC
 466     vector signed short   CY;
 467     vector signed short   CRV;
 468     vector signed short   CBU;
 469     vector signed short   CGU;
 470     vector signed short   CGV;
 471     vector signed short   OY;
 472     vector unsigned short CSHIFT;
 473     vector signed short  *vYCoeffsBank, *vCCoeffsBank;
 474 #endif
 475 
 476     int use_mmx_vfilter;
 477 
 478 /* pre defined color-spaces gamma */
 479 #define XYZ_GAMMA (2.6f)
 480 #define RGB_GAMMA (2.2f)
 481     int16_t *xyzgamma;
 482     int16_t *rgbgamma;
 483     int16_t *xyzgammainv;
 484     int16_t *rgbgammainv;
 485     int16_t xyz2rgb_matrix[3][4];
 486     int16_t rgb2xyz_matrix[3][4];
 487 
 488     /* function pointers for swscale() */
 489     yuv2planar1_fn yuv2plane1;
 490     yuv2planarX_fn yuv2planeX;
 491     yuv2interleavedX_fn yuv2nv12cX;
 492     yuv2packed1_fn yuv2packed1;
 493     yuv2packed2_fn yuv2packed2;
 494     yuv2packedX_fn yuv2packedX;
 495     yuv2anyX_fn yuv2anyX;
 496 
 497     /// Unscaled conversion of luma plane to YV12 for horizontal scaler.
 498     void (*lumToYV12)(uint8_t *dst, const uint8_t *src, const uint8_t *src2, const uint8_t *src3,
 499                       int width, uint32_t *pal);
 500     /// Unscaled conversion of alpha plane to YV12 for horizontal scaler.
 501     void (*alpToYV12)(uint8_t *dst, const uint8_t *src, const uint8_t *src2, const uint8_t *src3,
 502                       int width, uint32_t *pal);
 503     /// Unscaled conversion of chroma planes to YV12 for horizontal scaler.
 504     void (*chrToYV12)(uint8_t *dstU, uint8_t *dstV,
 505                       const uint8_t *src1, const uint8_t *src2, const uint8_t *src3,
 506                       int width, uint32_t *pal);
 507 
 508     /**
 509      * Functions to read planar input, such as planar RGB, and convert
 510      * internally to Y/UV/A.
 511      */
 512     /** @{ */
 513     void (*readLumPlanar)(uint8_t *dst, const uint8_t *src[4], int width, int32_t *rgb2yuv);
 514     void (*readChrPlanar)(uint8_t *dstU, uint8_t *dstV, const uint8_t *src[4],
 515                           int width, int32_t *rgb2yuv);
 516     void (*readAlpPlanar)(uint8_t *dst, const uint8_t *src[4], int width, int32_t *rgb2yuv);
 517     /** @} */
 518 
 519     /**
 520      * Scale one horizontal line of input data using a bilinear filter
 521      * to produce one line of output data. Compared to SwsContext->hScale(),
 522      * please take note of the following caveats when using these:
 523      * - Scaling is done using only 7bit instead of 14bit coefficients.
 524      * - You can use no more than 5 input pixels to produce 4 output
 525      *   pixels. Therefore, this filter should not be used for downscaling
 526      *   by more than ~20% in width (because that equals more than 5/4th
 527      *   downscaling and thus more than 5 pixels input per 4 pixels output).
 528      * - In general, bilinear filters create artifacts during downscaling
 529      *   (even when <20%), because one output pixel will span more than one
 530      *   input pixel, and thus some pixels will need edges of both neighbor
 531      *   pixels to interpolate the output pixel. Since you can use at most
 532      *   two input pixels per output pixel in bilinear scaling, this is
 533      *   impossible and thus downscaling by any size will create artifacts.
 534      * To enable this type of scaling, set SWS_FLAG_FAST_BILINEAR
 535      * in SwsContext->flags.
 536      */
 537     /** @{ */
 538     void (*hyscale_fast)(struct SwsContext *c,
 539                          int16_t *dst, int dstWidth,
 540                          const uint8_t *src, int srcW, int xInc);
 541     void (*hcscale_fast)(struct SwsContext *c,
 542                          int16_t *dst1, int16_t *dst2, int dstWidth,
 543                          const uint8_t *src1, const uint8_t *src2,
 544                          int srcW, int xInc);
 545     /** @} */
 546 
 547     /**
 548      * Scale one horizontal line of input data using a filter over the input
 549      * lines, to produce one (differently sized) line of output data.
 550      *
 551      * @param dst        pointer to destination buffer for horizontally scaled
 552      *                   data. If the number of bits per component of one
 553      *                   destination pixel (SwsContext->dstBpc) is <= 10, data
 554      *                   will be 15bpc in 16bits (int16_t) width. Else (i.e.
 555      *                   SwsContext->dstBpc == 16), data will be 19bpc in
 556      *                   32bits (int32_t) width.
 557      * @param dstW       width of destination image
 558      * @param src        pointer to source data to be scaled. If the number of
 559      *                   bits per component of a source pixel (SwsContext->srcBpc)
 560      *                   is 8, this is 8bpc in 8bits (uint8_t) width. Else
 561      *                   (i.e. SwsContext->dstBpc > 8), this is native depth
 562      *                   in 16bits (uint16_t) width. In other words, for 9-bit
 563      *                   YUV input, this is 9bpc, for 10-bit YUV input, this is
 564      *                   10bpc, and for 16-bit RGB or YUV, this is 16bpc.
 565      * @param filter     filter coefficients to be used per output pixel for
 566      *                   scaling. This contains 14bpp filtering coefficients.
 567      *                   Guaranteed to contain dstW * filterSize entries.
 568      * @param filterPos  position of the first input pixel to be used for
 569      *                   each output pixel during scaling. Guaranteed to
 570      *                   contain dstW entries.
 571      * @param filterSize the number of input coefficients to be used (and
 572      *                   thus the number of input pixels to be used) for
 573      *                   creating a single output pixel. Is aligned to 4
 574      *                   (and input coefficients thus padded with zeroes)
 575      *                   to simplify creating SIMD code.
 576      */
 577     /** @{ */
 578     void (*hyScale)(struct SwsContext *c, int16_t *dst, int dstW,
 579                     const uint8_t *src, const int16_t *filter,
 580                     const int32_t *filterPos, int filterSize);
 581     void (*hcScale)(struct SwsContext *c, int16_t *dst, int dstW,
 582                     const uint8_t *src, const int16_t *filter,
 583                     const int32_t *filterPos, int filterSize);
 584     /** @} */
 585 
 586     /// Color range conversion function for luma plane if needed.
 587     void (*lumConvertRange)(int16_t *dst, int width);
 588     /// Color range conversion function for chroma planes if needed.
 589     void (*chrConvertRange)(int16_t *dst1, int16_t *dst2, int width);
 590 
 591     int needs_hcscale; ///< Set if there are chroma planes to be converted.
 592 
 593     SwsDither dither;
 594 } SwsContext;
 595 //FIXME check init (where 0)
 596 
 597 SwsFunc ff_yuv2rgb_get_func_ptr(SwsContext *c);
 598 int ff_yuv2rgb_c_init_tables(SwsContext *c, const int inv_table[4],
 599                              int fullRange, int brightness,
 600                              int contrast, int saturation);
 601 void ff_yuv2rgb_init_tables_ppc(SwsContext *c, const int inv_table[4],
 602                                 int brightness, int contrast, int saturation);
 603 
 604 void updateMMXDitherTables(SwsContext *c, int dstY, int lumBufIndex, int chrBufIndex,
 605                            int lastInLumBuf, int lastInChrBuf);
 606 
 607 av_cold void ff_sws_init_range_convert(SwsContext *c);
 608 
 609 SwsFunc ff_yuv2rgb_init_x86(SwsContext *c);
 610 SwsFunc ff_yuv2rgb_init_ppc(SwsContext *c);
 611 
 612 #if FF_API_SWS_FORMAT_NAME
 613 /**
 614  * @deprecated Use av_get_pix_fmt_name() instead.
 615  */
 616 attribute_deprecated
 617 const char *sws_format_name(enum AVPixelFormat format);
 618 #endif
 619 
 620 static av_always_inline int is16BPS(enum AVPixelFormat pix_fmt)
 621 {
 622     const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
 623     av_assert0(desc);
 624     return desc->comp[0].depth_minus1 == 15;
 625 }
 626 
 627 static av_always_inline int is9_OR_10BPS(enum AVPixelFormat pix_fmt)
 628 {
 629     const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
 630     av_assert0(desc);
 631     return desc->comp[0].depth_minus1 >= 8 && desc->comp[0].depth_minus1 <= 13;
 632 }
 633 
 634 #define isNBPS(x) is9_OR_10BPS(x)
 635 
 636 static av_always_inline int isBE(enum AVPixelFormat pix_fmt)
 637 {
 638     const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
 639     av_assert0(desc);
 640     return desc->flags & AV_PIX_FMT_FLAG_BE;
 641 }
 642 
 643 static av_always_inline int isYUV(enum AVPixelFormat pix_fmt)
 644 {
 645     const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
 646     av_assert0(desc);
 647     return !(desc->flags & AV_PIX_FMT_FLAG_RGB) && desc->nb_components >= 2;
 648 }
 649 
 650 static av_always_inline int isPlanarYUV(enum AVPixelFormat pix_fmt)
 651 {
 652     const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
 653     av_assert0(desc);
 654     return ((desc->flags & AV_PIX_FMT_FLAG_PLANAR) && isYUV(pix_fmt));
 655 }
 656 
 657 static av_always_inline int isRGB(enum AVPixelFormat pix_fmt)
 658 {
 659     const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
 660     av_assert0(desc);
 661     return (desc->flags & AV_PIX_FMT_FLAG_RGB);
 662 }
 663 
 664 #if 0 // FIXME
 665 #define isGray(x) \
 666     (!(av_pix_fmt_desc_get(x)->flags & AV_PIX_FMT_FLAG_PAL) && \
 667      av_pix_fmt_desc_get(x)->nb_components <= 2)
 668 #else
 669 #define isGray(x)                      \
 670     ((x) == AV_PIX_FMT_GRAY8       ||  \
 671      (x) == AV_PIX_FMT_Y400A       ||  \
 672      (x) == AV_PIX_FMT_GRAY16BE    ||  \
 673      (x) == AV_PIX_FMT_GRAY16LE)
 674 #endif
 675 
 676 #define isRGBinInt(x) \
 677     (           \
 678      (x) == AV_PIX_FMT_RGB48BE     ||  \
 679      (x) == AV_PIX_FMT_RGB48LE     ||  \
 680      (x) == AV_PIX_FMT_RGB32       ||  \
 681      (x) == AV_PIX_FMT_RGB32_1     ||  \
 682      (x) == AV_PIX_FMT_RGB24       ||  \
 683      (x) == AV_PIX_FMT_RGB565BE    ||  \
 684      (x) == AV_PIX_FMT_RGB565LE    ||  \
 685      (x) == AV_PIX_FMT_RGB555BE    ||  \
 686      (x) == AV_PIX_FMT_RGB555LE    ||  \
 687      (x) == AV_PIX_FMT_RGB444BE    ||  \
 688      (x) == AV_PIX_FMT_RGB444LE    ||  \
 689      (x) == AV_PIX_FMT_RGB8        ||  \
 690      (x) == AV_PIX_FMT_RGB4        ||  \
 691      (x) == AV_PIX_FMT_RGB4_BYTE   ||  \
 692      (x) == AV_PIX_FMT_RGBA64BE    ||  \
 693      (x) == AV_PIX_FMT_RGBA64LE    ||  \
 694      (x) == AV_PIX_FMT_MONOBLACK   ||  \
 695      (x) == AV_PIX_FMT_MONOWHITE   \
 696     )
 697 #define isBGRinInt(x) \
 698     (           \
 699      (x) == AV_PIX_FMT_BGR48BE     ||  \
 700      (x) == AV_PIX_FMT_BGR48LE     ||  \
 701      (x) == AV_PIX_FMT_BGR32       ||  \
 702      (x) == AV_PIX_FMT_BGR32_1     ||  \
 703      (x) == AV_PIX_FMT_BGR24       ||  \
 704      (x) == AV_PIX_FMT_BGR565BE    ||  \
 705      (x) == AV_PIX_FMT_BGR565LE    ||  \
 706      (x) == AV_PIX_FMT_BGR555BE    ||  \
 707      (x) == AV_PIX_FMT_BGR555LE    ||  \
 708      (x) == AV_PIX_FMT_BGR444BE    ||  \
 709      (x) == AV_PIX_FMT_BGR444LE    ||  \
 710      (x) == AV_PIX_FMT_BGR8        ||  \
 711      (x) == AV_PIX_FMT_BGR4        ||  \
 712      (x) == AV_PIX_FMT_BGR4_BYTE   ||  \
 713      (x) == AV_PIX_FMT_BGRA64BE    ||  \
 714      (x) == AV_PIX_FMT_BGRA64LE    ||  \
 715      (x) == AV_PIX_FMT_MONOBLACK   ||  \
 716      (x) == AV_PIX_FMT_MONOWHITE   \
 717     )
 718 
 719 #define isRGBinBytes(x) (           \
 720            (x) == AV_PIX_FMT_RGB48BE     \
 721         || (x) == AV_PIX_FMT_RGB48LE     \
 722         || (x) == AV_PIX_FMT_RGBA64BE    \
 723         || (x) == AV_PIX_FMT_RGBA64LE    \
 724         || (x) == AV_PIX_FMT_RGBA        \
 725         || (x) == AV_PIX_FMT_ARGB        \
 726         || (x) == AV_PIX_FMT_RGB24       \
 727     )
 728 #define isBGRinBytes(x) (           \
 729            (x) == AV_PIX_FMT_BGR48BE     \
 730         || (x) == AV_PIX_FMT_BGR48LE     \
 731         || (x) == AV_PIX_FMT_BGRA64BE    \
 732         || (x) == AV_PIX_FMT_BGRA64LE    \
 733         || (x) == AV_PIX_FMT_BGRA        \
 734         || (x) == AV_PIX_FMT_ABGR        \
 735         || (x) == AV_PIX_FMT_BGR24       \
 736     )
 737 
 738 #define isBayer(x) ( \
 739            (x)==AV_PIX_FMT_BAYER_BGGR8    \
 740         || (x)==AV_PIX_FMT_BAYER_BGGR16LE \
 741         || (x)==AV_PIX_FMT_BAYER_BGGR16BE \
 742         || (x)==AV_PIX_FMT_BAYER_RGGB8    \
 743         || (x)==AV_PIX_FMT_BAYER_RGGB16LE \
 744         || (x)==AV_PIX_FMT_BAYER_RGGB16BE \
 745         || (x)==AV_PIX_FMT_BAYER_GBRG8    \
 746         || (x)==AV_PIX_FMT_BAYER_GBRG16LE \
 747         || (x)==AV_PIX_FMT_BAYER_GBRG16BE \
 748         || (x)==AV_PIX_FMT_BAYER_GRBG8    \
 749         || (x)==AV_PIX_FMT_BAYER_GRBG16LE \
 750         || (x)==AV_PIX_FMT_BAYER_GRBG16BE \
 751     )
 752 
 753 #define isAnyRGB(x) \
 754     (           \
 755           isBayer(x)          ||    \
 756           isRGBinInt(x)       ||    \
 757           isBGRinInt(x)       ||    \
 758           isRGB(x)      \
 759     )
 760 
 761 static av_always_inline int isALPHA(enum AVPixelFormat pix_fmt)
 762 {
 763     const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
 764     av_assert0(desc);
 765     if (pix_fmt == AV_PIX_FMT_PAL8)
 766         return 1;
 767     return desc->flags & AV_PIX_FMT_FLAG_ALPHA;
 768 }
 769 
 770 #if 1
 771 #define isPacked(x)         (       \
 772            (x)==AV_PIX_FMT_PAL8        \
 773         || (x)==AV_PIX_FMT_YUYV422     \
 774         || (x)==AV_PIX_FMT_YVYU422     \
 775         || (x)==AV_PIX_FMT_UYVY422     \
 776         || (x)==AV_PIX_FMT_Y400A       \
 777         ||  isRGBinInt(x)           \
 778         ||  isBGRinInt(x)           \
 779     )
 780 #else
 781 static av_always_inline int isPacked(enum AVPixelFormat pix_fmt)
 782 {
 783     const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
 784     av_assert0(desc);
 785     return ((desc->nb_components >= 2 && !(desc->flags & AV_PIX_FMT_FLAG_PLANAR)) ||
 786             pix_fmt == AV_PIX_FMT_PAL8);
 787 }
 788 
 789 #endif
 790 static av_always_inline int isPlanar(enum AVPixelFormat pix_fmt)
 791 {
 792     const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
 793     av_assert0(desc);
 794     return (desc->nb_components >= 2 && (desc->flags & AV_PIX_FMT_FLAG_PLANAR));
 795 }
 796 
 797 static av_always_inline int isPackedRGB(enum AVPixelFormat pix_fmt)
 798 {
 799     const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
 800     av_assert0(desc);
 801     return ((desc->flags & (AV_PIX_FMT_FLAG_PLANAR | AV_PIX_FMT_FLAG_RGB)) == AV_PIX_FMT_FLAG_RGB);
 802 }
 803 
 804 static av_always_inline int isPlanarRGB(enum AVPixelFormat pix_fmt)
 805 {
 806     const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
 807     av_assert0(desc);
 808     return ((desc->flags & (AV_PIX_FMT_FLAG_PLANAR | AV_PIX_FMT_FLAG_RGB)) ==
 809             (AV_PIX_FMT_FLAG_PLANAR | AV_PIX_FMT_FLAG_RGB));
 810 }
 811 
 812 static av_always_inline int usePal(enum AVPixelFormat pix_fmt)
 813 {
 814     const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
 815     av_assert0(desc);
 816     return (desc->flags & AV_PIX_FMT_FLAG_PAL) || (desc->flags & AV_PIX_FMT_FLAG_PSEUDOPAL);
 817 }
 818 
 819 extern const uint64_t ff_dither4[2];
 820 extern const uint64_t ff_dither8[2];
 821 
 822 extern const uint8_t ff_dither_2x2_4[3][8];
 823 extern const uint8_t ff_dither_2x2_8[3][8];
 824 extern const uint8_t ff_dither_4x4_16[5][8];
 825 extern const uint8_t ff_dither_8x8_32[9][8];
 826 extern const uint8_t ff_dither_8x8_73[9][8];
 827 extern const uint8_t ff_dither_8x8_128[9][8];
 828 extern const uint8_t ff_dither_8x8_220[9][8];
 829 
 830 extern const int32_t ff_yuv2rgb_coeffs[8][4];
 831 
 832 extern const AVClass sws_context_class;
 833 
 834 /**
 835  * Set c->swscale to an unscaled converter if one exists for the specific
 836  * source and destination formats, bit depths, flags, etc.
 837  */
 838 void ff_get_unscaled_swscale(SwsContext *c);
 839 void ff_get_unscaled_swscale_ppc(SwsContext *c);
 840 void ff_get_unscaled_swscale_arm(SwsContext *c);
 841 
 842 /**
 843  * Return function pointer to fastest main scaler path function depending
 844  * on architecture and available optimizations.
 845  */
 846 SwsFunc ff_getSwsFunc(SwsContext *c);
 847 
 848 void ff_sws_init_input_funcs(SwsContext *c);
 849 void ff_sws_init_output_funcs(SwsContext *c,
 850                               yuv2planar1_fn *yuv2plane1,
 851                               yuv2planarX_fn *yuv2planeX,
 852                               yuv2interleavedX_fn *yuv2nv12cX,
 853                               yuv2packed1_fn *yuv2packed1,
 854                               yuv2packed2_fn *yuv2packed2,
 855                               yuv2packedX_fn *yuv2packedX,
 856                               yuv2anyX_fn *yuv2anyX);
 857 void ff_sws_init_swscale_ppc(SwsContext *c);
 858 void ff_sws_init_swscale_x86(SwsContext *c);
 859 
 860 static inline void fillPlane16(uint8_t *plane, int stride, int width, int height, int y,
 861                                int alpha, int bits, const int big_endian)
 862 {
 863     int i, j;
 864     uint8_t *ptr = plane + stride * y;
 865     int v = alpha ? 0xFFFF>>(15-bits) : (1<<bits);
 866     for (i = 0; i < height; i++) {
 867 #define FILL(wfunc) \
 868         for (j = 0; j < width; j++) {\
 869             wfunc(ptr+2*j, v);\
 870         }
 871         if (big_endian) {
 872             FILL(AV_WB16);
 873         } else {
 874             FILL(AV_WL16);
 875         }
 876         ptr += stride;
 877     }
 878 }
 879 
 880 #endif /* SWSCALE_SWSCALE_INTERNAL_H */