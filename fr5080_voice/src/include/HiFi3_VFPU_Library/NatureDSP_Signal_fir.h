/* ------------------------------------------------------------------------ */
/* Copyright (c) 2017 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.  */
/* These coded instructions, statements, and computer programs ("Cadence    */
/* Libraries") are the copyrighted works of Cadence Design Systems Inc.	    */
/* Cadence IP is licensed for use with Cadence processor cores only and     */
/* must not be used for any other processors and platforms. Your use of the */
/* Cadence Libraries is subject to the terms of the license agreement you   */
/* have entered into with Cadence Design Systems, or a sublicense granted   */
/* to you by a direct Cadence licensee.                                     */
/* ------------------------------------------------------------------------ */
/*  IntegrIT, Ltd.   www.integrIT.com, info@integrIT.com                    */
/*                                                                          */
/* DSP Library                                                              */
/*                                                                          */
/* This library contains copyrighted materials, trade secrets and other     */
/* proprietary information of IntegrIT, Ltd. This software is licensed for  */
/* use with Cadence processor cores only and must not be used for any other */
/* processors and platforms. The license to use these sources was given to  */
/* Cadence, Inc. under Terms and Condition of a Software License Agreement  */
/* between Cadence, Inc. and IntegrIT, Ltd.                                 */
/* ------------------------------------------------------------------------ */
/*          Copyright (C) 2015-2017 IntegrIT, Limited.                      */
/*                      All Rights Reserved.                                */
/* ------------------------------------------------------------------------ */
#ifndef __NATUREDSP_SIGNAL_FIR_H__
#define __NATUREDSP_SIGNAL_FIR_H__

#include "NatureDSP_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
  FIR filters and related functions:
  bkfir                                   Block real FIR filter 
  cxfir                                   Complex block FIR filter
  firdec                                  Decimating block real FIR filter
  firinterp                               Interpolating block real FIR filter
  fir_convol, cxfir_convol, fir_lconvola  Circular/linear convolution
  fir_xcorr, cxfir_xcorr, fir_lxcorra     Circular/linear correlation
  fir_acorr, fir_lacorra                  Circular/linear autocorrelation  
  fir_blms                                Blockwise Adaptive LMS algorithm
                                          for real data
===========================================================================*/

/*-------------------------------------------------------------------------
  Real FIR filter.
  Computes a real FIR filter (direct-form) using IR stored in vector h. The 
  real data input is stored in vector x. The filter output result is stored 
  in vector y. The filter calculates N output samples using M coefficients 
  and requires last M-1 samples in the delay line.
  NOTE: 
  User application is not responsible for management of delay lines


  Precision: 
  16x16    16-bit data, 16-bit coefficients, 16-bit outputs
  24x24    24-bit data, 24-bit coefficients, 24-bit outputs
  24x24p   use 24-bit data packing for internal delay line buffer
           and internal coefficients storage
  32x16    32-bit data, 16-bit coefficients, 32-bit outputs
  32x32    32-bit data, 32-bit coefficients, 32-bit outputs
  f        floating point

  Input:
  x[N]     input samples, Q31, Q15, floating point
  h[M]     filter coefficients in normal order, Q31, Q15, floating point
  N        length of sample block, should be a multiple of 4
  M        length of filter, should be a multiple of 4
  Output:
  y[N]     output samples, Q31, Q15, floating point

  Restrictions:
  x, y      should not be overlapping
  x, h      aligned on a 8-bytes boundary
  N, M      multiples of 4 
-------------------------------------------------------------------------*/
typedef void * bkfir16x16_handle_t;
typedef void * bkfir24x24_handle_t;
typedef void * bkfir24x24p_handle_t;
typedef void * bkfir32x16_handle_t;
typedef void * bkfir32x32_handle_t;
typedef void * bkfirf_handle_t;

/* Update the delay line and compute filter output */
void bkfir16x16_process ( bkfir16x16_handle_t  handle, int16_t   * restrict y, const int16_t   * restrict x, int N);
void bkfir24x24_process ( bkfir24x24_handle_t  handle, f24       * restrict y, const f24       * restrict x, int N);
void bkfir24x24p_process( bkfir24x24p_handle_t handle, f24       * restrict y, const f24       * restrict x, int N);
void bkfir32x16_process ( bkfir32x16_handle_t  handle, int32_t   * restrict y, const int32_t   * restrict x, int N);
void bkfir32x32_process ( bkfir32x32_handle_t  handle, int32_t   * restrict y, const int32_t   * restrict x, int N);
void bkfirf_process     ( bkfirf_handle_t      handle, float32_t * restrict y, const float32_t * restrict x, int N);

/* Returns: size of memory in bytes to be allocated */
size_t bkfir16x16_alloc (int M);
size_t bkfir24x24_alloc (int M); 
size_t bkfir24x24p_alloc(int M);
size_t bkfir32x16_alloc (int M);
size_t bkfir32x32_alloc (int M);
size_t bkfirf_alloc     (int M); 

/* Returns: handle to the object */
bkfir16x16_handle_t   bkfir16x16_init  (void * objmem, int M, const int16_t  * restrict h);
bkfir24x24_handle_t   bkfir24x24_init  (void * objmem, int M, const f24      * restrict h);
bkfir24x24p_handle_t  bkfir24x24p_init (void * objmem, int M, const f24      * restrict h);
bkfir32x16_handle_t   bkfir32x16_init  (void * objmem, int M, const int16_t  * restrict h);
bkfir32x32_handle_t   bkfir32x32_init  (void * objmem, int M, const int32_t  * restrict h);
bkfirf_handle_t       bkfirf_init      (void * objmem, int M, const float32_t* restrict h);

/*-------------------------------------------------------------------------
  Real FIR filter.
  Computes a real FIR filter (direct-form) using IR stored in vector h. The 
  real data input is stored in vector x. The filter output result is stored 
  in vector y. The filter calculates N output samples using M coefficients 
  and requires last M-1 samples in the delay line.
  These functions implement FIR filter with no limitation on size of data
  block, alignment and length of impulse response at the cost of increased
  processing complexity.
  NOTE: 
  User application is not responsible for management of delay lines.

  Precision: 
  16x16    16-bit data, 16-bit coefficients, 16-bit outputs
  24x24    24-bit data, 24-bit coefficients, 24-bit outputs
  32x16    32-bit data, 16-bit coefficients, 32-bit outputs
  32x32    32-bit data, 32-bit coefficients, 32-bit outputs
  f        floating point
  Input:
  x[N]     input samples, Q15, Q31, floating point
  h[M]     filter coefficients in normal order, Q15, Q31, floating point
  N        length of sample block
  M        length of filter
  Output:
  y[N]     input samples, Q15, Q31, floating point 

  Restrictions:
  x,y      should not be overlapping
-------------------------------------------------------------------------*/
typedef void * bkfira16x16_handle_t;
typedef void * bkfira24x24_handle_t;
typedef void * bkfira32x16_handle_t;
typedef void * bkfira32x32_handle_t;
typedef void * bkfiraf_handle_t;

/* Returns: size of memory in bytes to be allocated */
size_t bkfira16x16_alloc(int M);
size_t bkfira24x24_alloc(int M);
size_t bkfira32x16_alloc(int M);
size_t bkfira32x32_alloc(int M);
size_t bkfiraf_alloc    (int M);

/* Returns: handle to the object */
bkfira16x16_handle_t bkfira16x16_init (void * objmem, int M, const int16_t  * restrict h);
bkfira24x24_handle_t bkfira24x24_init (void * objmem, int M, const f24      * restrict h);
bkfira32x16_handle_t bkfira32x16_init (void * objmem, int M, const int16_t  * restrict h);
bkfira32x32_handle_t bkfira32x32_init (void * objmem, int M, const int32_t  * restrict h);
bkfira32x16_handle_t bkfiraf_init     (void * objmem, int M, const float32_t* restrict h);

/* Update the delay line and compute filter output */
void bkfira16x16_process ( bkfira16x16_handle_t  handle, int16_t   * restrict y, const int16_t   * restrict x , int N);
void bkfira24x24_process ( bkfira24x24_handle_t  handle, f24       * restrict y, const f24       * restrict x , int N);
void bkfira32x16_process ( bkfira32x16_handle_t  handle, int32_t   * restrict y, const int32_t   * restrict x , int N);
void bkfira32x32_process ( bkfira32x32_handle_t  handle, int32_t   * restrict y, const int32_t   * restrict x , int N);
void bkfiraf_process     ( bkfiraf_handle_t      handle, float32_t * restrict y, const float32_t * restrict x , int N);

/*-------------------------------------------------------------------------
  Complex Block FIR Filter
  Computes a complex FIR filter (direct-form) using complex IR stored in 
  vector h. The complex data input is stored in vector x. The filter output
  result is stored in vector r. The filter calculates N output samples using 
  M coefficients and requires last M-1 samples in the delay line. Real and
  imaginary parts are interleaved and real parts go first (at even indexes).
  NOTE: 
  User application is not responsible for management of delay lines

  Precision: 
  16x16     16-bit data, 16-bit coefficients, 16-bit outputs
  24x24     24-bit data, 24-bit coefficients, 24-bit outputs
  32x16     32-bit data, 16-bit coefficients, 32-bit outputs
  32x32     32-bit data, 32-bit coefficients, 32-bit outputs
  f         floating point
  Input:
  h[M]      complex filter coefficients; h[0] is to be multiplied with the 
            newest sample , Q15, Q31, floating point
  x[N]      input samples, Q15, Q31, floating point
  N         length of sample block (in complex samples) 
  M         length of filter 
  Output:			
  y[2*N]    output samples, Q15, Q31, floating point

  Restriction:
  x,y       should not overlap
  x,h       aligned on a 8-bytes boundary
  N,M       multiples of 4
-------------------------------------------------------------------------*/
typedef void* cxfir16x16_handle_t;
typedef void* cxfir24x24_handle_t;
typedef void* cxfir32x16_handle_t;
typedef void* cxfir32x32_handle_t;
typedef void* cxfirf_handle_t;

/* Returns: size of memory in bytes to be allocated */
size_t cxfir16x16_alloc(int M);
size_t cxfir24x24_alloc(int M);
size_t cxfir32x16_alloc(int M);
size_t cxfir32x32_alloc(int M);
size_t cxfirf_alloc(int M);

/* Returns: handle to the object */
cxfir16x16_handle_t cxfir16x16_init(void * objmem, int M, const complex_fract16* restrict h);
cxfir24x24_handle_t cxfir24x24_init(void * objmem, int M, const complex_fract32* restrict h);
cxfir32x16_handle_t cxfir32x16_init(void * objmem, int M, const complex_fract16* restrict h);
cxfir32x32_handle_t cxfir32x32_init(void * objmem, int M, const complex_fract32* restrict h);
cxfirf_handle_t     cxfirf_init    (void * objmem, int M, const complex_float  * restrict h);

/* Update the delay line and compute filter output */
void cxfir16x16_process(cxfir16x16_handle_t handle, complex_fract16 * restrict y, const complex_fract16 * restrict x , int N);
void cxfir24x24_process(cxfir24x24_handle_t handle, complex_fract32 * restrict y, const complex_fract32 * restrict x , int N);
void cxfir32x16_process(cxfir32x16_handle_t handle, complex_fract32 * restrict y, const complex_fract32 * restrict x , int N);
void cxfir32x32_process(cxfir32x32_handle_t handle, complex_fract32 * restrict y, const complex_fract32 * restrict x , int N);
void cxfirf_process    (cxfirf_handle_t     handle, complex_float   * restrict y, const complex_float   * restrict x , int N);

/*-------------------------------------------------------------------------
  Decimating Block Real FIR Filter
  Computes a real FIR filter (direct-form) with decimation using IR stored 
  in vector h. The real data input is stored in vector x. The filter output 
  result is stored in vector r. The filter calculates N output samples using
  M coefficients and requires last D*N+M-1 samples on the delay line.
  NOTE:
  - To avoid aliasing IR should be synthesized in such a way to be narrower 
    than input sample rate divided to 2D.
  - user application is not responsible for management of delay lines

  Precision: 
  16x16     16-bit data, 16-bit coefficients, 16-bit outputs
  24x24     24-bit data, 24-bit coefficients, 24-bit outputs
  32x16     32-bit data, 16-bit coefficients, 32-bit outputs
  32x32     32-bit data, 32-bit coefficients, 32-bit outputs
  f         floating point

  Input:
  h[M]      filter coefficients; h[0] is to be multiplied with the newest 
            sample, Q15, Q31, floating point
  D         decimation factor 
  N         length of output sample block
  M         length of filter
  x[D*N]    input samples, Q15, Q31, floating point
  Output:
  y[N]      output samples, Q15, Q31, floating point

  Restriction:
  x,h,r     should not overlap
  x, h      aligned on an 8-bytes boundary
  N         multiple of 8
  D         should exceed 1

  PERFORMANCE NOTE:
  for optimum performance follow rules:
  D - 2, 3 or 4
-------------------------------------------------------------------------*/
typedef void* firdec16x16_handle_t;
typedef void* firdec24x24_handle_t;
typedef void* firdec32x16_handle_t;
typedef void* firdec32x32_handle_t;
typedef void* firdecf_handle_t;

/* Returns: size of memory in bytes to be allocated */
size_t firdec16x16_alloc(int D, int M);
size_t firdec24x24_alloc(int D, int M);
size_t firdec32x16_alloc(int D, int M);
size_t firdec32x32_alloc(int D, int M);
size_t firdecf_alloc    (int D, int M);

/* Returns: handle to the object */
firdec16x16_handle_t firdec16x16_init(void * objmem, int D, int M, const int16_t   * restrict h);
firdec24x24_handle_t firdec24x24_init(void * objmem, int D, int M, const f24       * restrict h);
firdec32x16_handle_t firdec32x16_init(void * objmem, int D, int M, const int16_t   * restrict h);
firdec32x32_handle_t firdec32x32_init(void * objmem, int D, int M, const int32_t   * restrict h);
firdecf_handle_t     firdecf_init    (void * objmem, int D, int M, const float32_t * restrict h);

/* Update the delay line and compute decimator output */
void firdec16x16_process(firdec16x16_handle_t handle, int16_t   * restrict y, const int16_t   * x, int N);
void firdec24x24_process(firdec24x24_handle_t handle, f24       * restrict y, const f24       * x, int N);
void firdec32x16_process(firdec32x16_handle_t handle, int32_t   * restrict y, const int32_t   * x, int N);
void firdec32x32_process(firdec32x32_handle_t handle, int32_t   * restrict y, const int32_t   * x, int N);
void firdecf_process    (firdecf_handle_t     handle, float32_t * restrict y, const float32_t * x, int N);

/*-------------------------------------------------------------------------
  Interpolating Block Real FIR Filter
  Computes a real FIR filter (direct-form) with interpolation using IR stored 
  in vector h. The real data input is stored in vector x. The filter output 
  result is stored in vector y. The filter calculates N*D output samples 
  using M*D coefficients and requires last N+M*D-1 samples on the delay line.
  NOTE:
  user application is not responsible for management of delay lines

  Precision: 
  16x16     16-bit data, 16-bit coefficients, 16-bit outputs
  24x24     24-bit data, 24-bit coefficients, 24-bit outputs
  32x16     32-bit data, 16-bit coefficients, 32-bit outputs
  32x32     32-bit data, 32-bit coefficients, 32-bit outputs
  f         floating point

  Input:
  h[M*D]    filter coefficients; h[0] is to be multiplied with the 
            newest sample, Q15, Q31, floating point
  D         interpolation ratio
  N         length of input sample block
  M         length of subfilter. Total length of filter is M*D
  x[N]      input samples, Q15, Q31, floating point
  Output:
  y[N*D]    output samples, Q15, Q31, floating point

  Restrictions:
  x,h,y     should not overlap
  x,h       aligned on an 8-bytes boundary
  N         multiple of 8
  M         multiple of 4
  D         should be >1

  PERFORMANCE NOTE:
  for optimum performance follow rules:
  D   - 2, 3 or 4

-------------------------------------------------------------------------*/
typedef void* firinterp16x16_handle_t;
typedef void* firinterp24x24_handle_t;
typedef void* firinterp32x16_handle_t;
typedef void* firinterp32x32_handle_t;
typedef void* firinterpf_handle_t;

/* Returns: size of memory in bytes to be allocated */
size_t firinterp16x16_alloc(int D, int M);
size_t firinterp24x24_alloc(int D, int M);
size_t firinterp32x16_alloc(int D, int M);
size_t firinterp32x32_alloc(int D, int M);
size_t firinterpf_alloc    (int D, int M);

/* Returns: handle to the object */
firinterp16x16_handle_t firinterp16x16_init(void * objmem, int D, int M, const int16_t   * restrict h);
firinterp24x24_handle_t firinterp24x24_init(void * objmem, int D, int M, const f24       * restrict h);
firinterp32x16_handle_t firinterp32x16_init(void * objmem, int D, int M, const int16_t   * restrict h);
firinterp32x32_handle_t firinterp32x32_init(void * objmem, int D, int M, const int32_t   * restrict h);
firinterpf_handle_t     firinterpf_init    (void * objmem, int D, int M, const float32_t * restrict h);

/* Update the delay line and compute interpolator output */
void firinterp16x16_process(firinterp16x16_handle_t handle, int16_t  * restrict y, const int16_t  * restrict x, int N);
void firinterp24x24_process(firinterp24x24_handle_t handle, f24      * restrict y, const f24      * restrict x, int N);
void firinterp32x16_process(firinterp32x16_handle_t handle, int32_t  * restrict y, const int32_t  * restrict x, int N);
void firinterp32x32_process(firinterp32x32_handle_t handle, int32_t  * restrict y, const int32_t  * restrict x, int N);
void firinterpf_process    (firinterpf_handle_t     handle, float32_t* restrict y, const float32_t* restrict x, int N);

/*-------------------------------------------------------------------------
  Circular Convolution
  Performs circular convolution between vectors x (of length N) and y (of 
  length M)  resulting in vector r of length N.

  Precision: 
  16x16     16x16-bit data, 16-bit outputs
  24x24     24x24-bit data, 24-bit outputs
  32x16     32x16-bit data, 32-bit outputs 
  32x32     32x32-bit data, 32-bit outputs
  f         floating point

  Input:
  x[N]      input data, Q15, Q31 or floating point
  y[M]      input data, Q15, Q31 or floating point
  N         length of x
  M         length of y
  Output:
  r[N]      output data, Q15, Q31 or floating point

  Restriction:
  x,y,r     should not overlap
  x,y,r     aligned on an 8-bytes boundary
  N,M       multiples of 4 and >0
-------------------------------------------------------------------------*/
void fir_convol16x16 (int16_t * restrict r,
                const int16_t * restrict x,
                const int16_t * restrict y,
                int N,
                int M);
void fir_convol24x24 (f24 * restrict r,
                const f24 * restrict x,
                const f24 * restrict y,
                int N,
                int M);
void fir_convol32x16 (int32_t * restrict r,
                const int32_t * restrict x,
                const int16_t * restrict y,
                int N,
                int M);
void fir_convol32x32 (int32_t * restrict r,
                const int32_t * restrict x,
                const int32_t * restrict y,
                int N,
                int M);
void fir_convolf ( float32_t * restrict r,
             const float32_t * restrict x,
             const float32_t * restrict y,
             int N,
             int M);
void cxfir_convol32x16 (complex_fract32 * restrict r,
                  const complex_fract32 * restrict x,
                  const complex_fract16 * restrict y,
                  int N,
                  int M);

/*-------------------------------------------------------------------------
  Circular Convolution
  Performs circular convolution between vectors x (of length N) and y (of 
  length M) resulting in vector r of length N.
  These functions implement the circular convolution algorithm with no 
  limitations on x and y vectors length and alignment at the cost of 
  increased processing complexity. In addition, this implementation variant
  requires scratch memory area.

  Precision: 
  16x16     16x16-bit data, 16-bit outputs
  24x24     24x24-bit data, 24-bit outputs
  32x16     32x16-bit data, 32-bit outputs 
  32x32     32x32-bit data, 32-bit outputs
  f         floating point

  Input:
  s[]       scratch area, 
              FIR_CONVOLA16X16_SCRATCH_SIZE(N,M) or
              FIR_CONVOLA24X24_SCRATCH_SIZE(N,M) or
              FIR_CONVOLA32X16_SCRATCH_SIZE(N,M) or
              CXFIR_CONVOLA32X16_SCRATCH_SIZE(N,M) or
              FIR_CONVOLA32X32_SCRATCH_SIZE(N,M) or
              FIR_CONVOLAF_SCRATCH_SIZE(N,M) bytes

  x[N]      input data Q15, Q31 or floating point
  y[M]      input data Q15, Q31 or floating point
  N         length of x
  M         length of y
  Output:
  r[N]      output data, Q15, Q31 or floating point

  Restriction:
  x,y,r     should not overlap
  s         must be aligned on an 8-bytes boundary
  N,M       must be >0
  N >= M-1  minimum allowed length of vector x is the length of y minus one
-------------------------------------------------------------------------*/
#define FIR_CONVOLA16X16_SCRATCH_SIZE( N, M )    ( 2*( 2*(M) + (N) + 6 ) + 6              )
#define FIR_CONVOLA24X24_SCRATCH_SIZE( N, M )    ( 4*( 2*(M) + (N) + 6 )                  )
#define FIR_CONVOLAF_SCRATCH_SIZE( N, M )        ( 4*( 2*(M) + (N) + 6 )                  )
#define FIR_CONVOLA32X16_SCRATCH_SIZE( N, M )    ( 4*( (M) + (N) + 2 ) +  2*( (M) + 6 )   )
#define CXFIR_CONVOLA32X16_SCRATCH_SIZE( N, M )  ( 2*( 4*( (M) + (N) + 2 ) + 2 * ((M)+6)) )
#define FIR_CONVOLA32X32_SCRATCH_SIZE( N, M )    ( 4*( 2*(M) + (N) + 6 )                  )

void fir_convola16x16 (void    * restrict s,
                       int16_t * restrict r,
                 const int16_t * restrict x,
                 const int16_t * restrict y,
                 int N,
                 int M);
void fir_convola24x24 (void * restrict s,
                       f24  * restrict r,
                 const f24  * restrict x,
                 const f24  * restrict y,
                 int N,
                 int M);
void fir_convola32x16 (void    * restrict s,
                       int32_t * restrict r,
                 const int32_t * restrict x,
                 const int16_t * restrict y,
                 int N,
                 int M);
void cxfir_convola32x16(void          * restrict s,
                      complex_fract32 * restrict r,
                const complex_fract32 * restrict x,
                const complex_fract16 * restrict y,
                int N,
                int M);
void fir_convola32x32 (void    * restrict s,
                       int32_t * restrict r,
                 const int32_t * restrict x,
                 const int32_t * restrict y,
                 int N,
                 int M);
void fir_convolaf     (void       * restrict s,
                       float32_t  * restrict r,
                 const float32_t  * restrict x,
                 const float32_t  * restrict y,
                 int N,
                 int M);

/*-------------------------------------------------------------------------
  Linear Convolution
  Functions perform linear convolution between vectors x (of length N) and
  y (of length M) resulting in vector r of length N+M-1.

  Precision: 
  16x16       16x16-bit data, 16-bit outputs
  32x32       32x32-bit data, 32-bit outputs 

  Input:
  s[]         Scratch memory,
              FIR_LCONVOLA16X16_SCRATCH_SIZE( N, M )
              FIR_LCONVOLA32X32_SCRATCH_SIZE( N, M )
              bytes
  x[N]        input data, Q15, Q31
  y[M]        input data, Q15, Q31
  N           length of x
  M           length of y
  Output:
  r[N+M-1]    output data, Q15, Q31

  Restriction:
  x,y,r       should not overlap
  s           must be aligned on an 8-bytes boundary
  N,M         must be >0
  N >= M-1    minimum allowed length of vector x is the length of y minus one
-------------------------------------------------------------------------*/
#define FIR_LCONVOLA16X16_SCRATCH_SIZE( N, M )    ( 2*(N + M + 16) )
#define FIR_LCONVOLA32X32_SCRATCH_SIZE( N, M )    ( 4*(N + M + 9) )

void fir_lconvola16x16 (void    * restrict s,
                       int16_t  * restrict r,
                 const int16_t  * restrict x, 
                 const int16_t  * restrict y, int N, int M);
void fir_lconvola32x32 (void    * restrict s,
                       int32_t  * restrict r,
                 const int32_t  * restrict x, 
                 const int32_t  * restrict y, int N, int M);

/*-------------------------------------------------------------------------
  Circular Correlation
  Estimates the circular cross-correlation between vectors x (of length N) 
  and y (of length M)  resulting in vector r of length N. It is a similar 
  to correlation but x is read in opposite direction.

  Precision: 
  16x16     16x16-bit data, 16-bit outputs
  24x24     24x24-bit data, 24-bit outputs
  32x16     32x16-bit data, 32-bit outputs
  32x32     32x32-bit data, 32-bit outputs
  f         floating point 


  Input:
  x[N]      input data Q15, Q31 or floating point
  y[M]      input data Q15, Q31 or floating point
  N         length of x
  M         length of y
  Output:
  r[N]      output data, Q15, Q31 or floating point

  Restriction:
  x,y,r     should not overlap
  x,y,r     aligned on an 8-bytes boundary
  N,M       multiples of 4 and >0
-------------------------------------------------------------------------*/
void fir_xcorr16x16 ( int16_t  * restrict r,
                const int16_t  * restrict x, 
                const int16_t  * restrict y, int N, int M);
void fir_xcorr24x24 ( f24 * restrict r,
                const f24 * restrict x,
                const f24 * restrict y,
                int N,
                int M);
void fir_xcorr32x16 ( int32_t * restrict r,
                const int32_t * restrict x,
                const int16_t * restrict y,
                int N,
                int M);
void fir_xcorr32x32 ( int32_t  * restrict r,
                const int32_t  * restrict x, 
                const int32_t  * restrict y, int N, int M);
void fir_xcorrf (   float32_t * restrict r,
              const float32_t * restrict x,
              const float32_t * restrict y,
              int N,
              int M);
void cxfir_xcorrf (  complex_float * restrict r,
              const  complex_float * restrict x,
              const  complex_float * restrict y,
              int N,
              int M);
/*-------------------------------------------------------------------------
  Circular Correlation
  Estimates the circular cross-correlation between vectors x (of length N) 
  and y (of length M)  resulting in vector r of length N. It is a similar 
  to correlation but x is read in opposite direction.
  These functions implement the circular correlation algorithm with no 
  limitations on x and y vectors length and alignment at the cost of 
  increased processing complexity. In addition, this implementation variant
  requires scratch memory area.

  Precision: 
  16x16     16x16-bit data, 16-bit outputs
  24x24     24x24-bit data, 24-bit outputs
  32x16     32x16-bit data, 32-bit outputs
  32x32     32x32-bit data, 32-bit outputs
  f         floating point

  Input:
  s[]       scratch area, 
              FIR_XCORRA16X16_SCRATCH_SIZE( N, M )
              FIR_XCORRA24X24_SCRATCH_SIZE( N, M ) or
              FIR_XCORRA32X16_SCRATCH_SIZE( N, M ) or
              FIR_XCORRA32X32_SCRATCH_SIZE( N, M ) or
              CXFIR_XCORRAF_SCRATCH_SIZE( N, M ) or
              FIR_XCORRAF_SCRATCH_SIZE( N, M ) bytes

  x[N]      input data Q15, Q31 or floating point
  y[M]      input data Q15, Q31 or floating point
  N         length of x
  M         length of y
  Output:
  r[N]      output data, Q15, Q31 or floating point

  Restrictions:
  x,y,r     should not overlap
  s         must be aligned on an 8-bytes boundary
  N,M       must be >0
  N >= M-1  minimum allowed length of vector x is the length of y minus one

-------------------------------------------------------------------------*/

#define FIR_XCORRA16X16_SCRATCH_SIZE( N, M )      ( 2*( 2*(M) + (N) + 6 ) + 6           )
#define FIR_XCORRA24X24_SCRATCH_SIZE( N, M )      ( 4*( 2*(M) + (N) + 6 )               )
#define FIR_XCORRAF_SCRATCH_SIZE( N, M )          ( 4*(M+N-1)                           )
#define FIR_XCORRA32X32_SCRATCH_SIZE( N, M )      ( 4*( 2*(M) + (N) + 6 )               )
#define CXFIR_XCORRAF_SCRATCH_SIZE( N, M )        ( 0                                   )
#define FIR_XCORRA32X16_SCRATCH_SIZE( N, M )      ( 4*( (M) + (N) + 2 ) + 2*( (M) + 6 ) )

void fir_xcorra16x16 ( void     * restrict s,
                       int16_t  * restrict r,
                 const int16_t  * restrict x, 
                 const int16_t  * restrict y, int N, int M);
void fir_xcorra24x24 (void * restrict s,
                       f24 * restrict r,
                 const f24 * restrict x,
                 const f24 * restrict y,
                 int N,
                 int M);
void fir_xcorra32x16 (void    * restrict s,
                      int32_t * restrict r,
                const int32_t * restrict x,
                const int16_t * restrict y,
                int N,
                int M);
void fir_xcorra32x32 ( void     * restrict s,
                       int32_t  * restrict r,
                 const int32_t  * restrict x, 
                 const int32_t  * restrict y, int N, int M);
void fir_xcorraf      (void      * restrict s,
                       float32_t * restrict r,
                 const float32_t * restrict x,
                 const float32_t * restrict y,
                 int N,
                 int M);
void cxfir_xcorraf    (void      * restrict s,
                       complex_float * restrict r,
                 const complex_float * restrict x,
                 const complex_float * restrict y,
                 int N,
                 int M);

/*-------------------------------------------------------------------------
  Linear Correlation
  Functions estimate the linear cross-correlation between vectors x (of 
  length N) and y (of length M) resulting in vector r of length N+M-1. It 
  is similar to convolution but y is read in opposite direction.

  Precision: 
  16x16        16x16-bit data, 16-bit outputs
  32x32        32x32-bit data, 32-bit outputs

  Input:
  s[]          Scratch memory, 
               FIR_LXCORRA16X16_SCRATCH_SIZE(N, M) 
               FIR_LXCORRA32X32_SCRATCH_SIZE(N, M) bytes
               
  x[N]         input data Q15, Q31 
  y[M]         input data Q15, Q31
  N            length of x
  M            length of y
  Output:
  r[N+M-1]     output data, Q15, Q31 

  Restrictions:
  x,y,r,s      should not overlap
  s            should be aligned on 8-byte boundary
  N,M          must be >0
  N>=M-1       minimum allowed length of vector x is the length of y minus one
-------------------------------------------------------------------------*/
#define FIR_LXCORRA16X16_SCRATCH_SIZE(N, M)      ( 2*(N + M + 16) )
#define FIR_LXCORRA32X32_SCRATCH_SIZE(N, M)      ( 4*(N + M + 9) )

void fir_lxcorra16x16 ( void    * restrict s,
                       int16_t  * restrict r,
                 const int16_t  * restrict x, 
                 const int16_t  * restrict y, int N, int M);
void fir_lxcorra32x32 ( void    * restrict s,
                       int32_t  * restrict r,
                 const int32_t  * restrict x,
                 const int32_t  * restrict y, int N, int M);

/*-------------------------------------------------------------------------
  Circular Autocorrelation 
  Estimates the auto-correlation of vector x. Returns autocorrelation of 
  length N.

  Precision: 
  16x16     16-bit data, 16-bit outputs
  24x24     24-bit data, 24-bit outputs
  32x32     32-bit data, 32-bit outputs
  f         floating point

  Input:
  x[N]      input data, Q15, Q31 or floating point
  N         length of x
  Output:
  r[N]      output data, Q15, Q31 or floating point

  Restrictions:
  x,r       should not overlap
  x,r       aligned on an 8-bytes boundary
  N         multiple of 4 and >0
-------------------------------------------------------------------------*/
void fir_acorr16x16 (   int16_t   *  restrict r, const int16_t  *  restrict x, int N);
void fir_acorr24x24 (   f24       *  restrict r, const f24      *  restrict x, int N);
void fir_acorr32x32 (   int32_t   *  restrict r, const int32_t  *  restrict x, int N);
void fir_acorrf     (   float32_t *  restrict r, const float32_t*  restrict x, int N);

/*-------------------------------------------------------------------------
  Circular Autocorrelation 
  Estimates the auto-correlation of vector x. Returns autocorrelation of 
  length N.
  These functions implement the circular autocorrelation algorithm with no 
  limitations on x vector length and alignment at the cost of increased 
  processing complexity. In addition, this implementation variant requires
  scratch memory area.

  Precision: 
  16x16   16-bit data, 16-bit outputs
  24x24   24-bit data, 24-bit outputs
  32x32   32-bit data, 32-bit outputs
  f       floating point

  Input:
  s[]     scratch area of
            FIR_ACORRA16X16_SCRATCH_SIZE( N )
            FIR_ACORRA24X24_SCRATCH_SIZE( N )
            FIR_ACORRA32X32_SCRATCH_SIZE( N )
            FIR_ACORRAF_SCRATCH_SIZE( N ) bytes
              
  x[N]    input data Q15, Q31 or floating point
  N       length of x
  Output:
  r[N]    output data, Q15, Q31 or floating point

  Restrictions:
  x,r,s   should not overlap
  N       must be non-zero
  s       aligned on an 8-bytes boundary
-------------------------------------------------------------------------*/
#define FIR_ACORRA16X16_SCRATCH_SIZE( N )       ( 2*( 3*(N) + 12+2) )
#define FIR_ACORRA24X24_SCRATCH_SIZE( N )       ( 4*( 2*(N) + 3 ) )
#define FIR_ACORRA32X32_SCRATCH_SIZE( N )       ( 4*( 2*(N) + 3 ) )
#define FIR_ACORRAF_SCRATCH_SIZE( N )           ( 4*(2*N-1) )

void fir_acorra16x16 ( void          * restrict s,
                      int16_t        * restrict r, 
                      const int16_t  * restrict x, int N );
void fir_acorra24x24 ( void * restrict s,
                       f24  * restrict r,
                 const f24  * restrict x,
                 int N );
void fir_acorra32x32 ( void          * restrict s,
                      int32_t        * restrict r,
                      const int32_t  * restrict x, int N );
void fir_acorraf     ( void       * restrict s,
                       float32_t  * restrict r,
                 const float32_t  * restrict x,
                 int N );

/*-------------------------------------------------------------------------
  Linear Autocorrelation 
  Functions estimate the linear auto-correlation of vector x. Returns 
  autocorrelation of length N.

  Precision: 
  16x16   16-bit data, 16-bit outputs
  32x32   32-bit data, 32-bit outputs

  Input:
  s[]       scratch area of
            FIR_LACORRA16X16_SCRATCH_SIZE( N )
            FIR_LACORRA32X32_SCRATCH_SIZE( N ) bytes
            
  x[N]      input data Q15, Q31 
  N         length of x
  Output:
  r[N]      output data, Q15, Q31

  Restrictions:
  x,r,s   should not overlap
  N       >0
  s       aligned on an 8-bytes boundary
-------------------------------------------------------------------------*/
#define FIR_LACORRA16X16_SCRATCH_SIZE( N )       ( 2*(N + 6) )
#define FIR_LACORRA32X32_SCRATCH_SIZE( N )       ( 4*(N + 4) )
void fir_lacorra16x16 (void* restrict s, int16_t  * restrict r, const int16_t  * restrict x, int N);
void fir_lacorra32x32 (void* restrict s, int32_t  * restrict r, const int32_t  * restrict x, int N);

/*-------------------------------------------------------------------------
  Blockwise Adaptive LMS Algorithm for Real Data
  Blockwise LMS algorithm performs filtering of reference samples x[N+M-1],
  computation of error e[N] over a block of input samples r[N] and makes
  blockwise update of IR to minimize the error output.
  Algorithm includes FIR filtering, calculation of correlation between the 
  error output e[N] and reference signal x[N+M-1] and IR taps update based
  on that correlation.
NOTES: 
  1. The algorithm must be provided with the normalization factor, which is
     the power of the reference signal times N - the number of samples in a
     data block. This can be calculated using the vec_power24x24() or 
     vec_power16x16() function. In order to avoid the saturation of the 
     normalization factor, it may be biased, i.e. shifted to the right.
     If it's the case, then the adaptation coefficient must be also shifted
     to the right by the same number of bit positions.
  2. This algorithm consumes less CPU cycles per block than single 
     sample algorithm at similar convergence rate.
  3. Right selection of N depends on the change rate of impulse response:
     on static or slow varying channels convergence rate depends on
     selected mu and M, but not on N.
  4. 16x16 routine may converge slower on small errors due to roundoff 
     errors. In that cases, 16x32 routine will give better results although
     convergence rate on bigger errors is the same.

  Precision: 
  16x16    16-bit coefficients, 16-bit data, 16-bit output
  24x24    24-bit coefficients, 24-bit data, 24-bit output
  16x32    32-bit coefficients, 16-bit data, 16-bit output
  32x32    32-bit coefficients, 32-bit data, 32-bit output
  f        floating point
  Input:
  h[M]     impulse response, Q15, Q31 or floating point
  r[N]	   input data vector (near end). First in time value is in 
           r[0], Q15, Q31 or floating point
  x[N+M-1] reference data vector (far end). First in time value is in x[0],  
           Q15, Q31 or floating point
  norm     normalization factor: power of signal multiplied by N, Q15, Q31  
           or floating point
           Fixed-point format for the 24x24-bit variant: Q(2*x-31-bias)
           Fixed-point format for the 32x16-bit variant: Q(2*x+1-bias)
  mu       adaptation coefficient (LMS step), Q(31-bias) or Q(15-bias)
  N        length of data block
  M        length of h
  Output:
  e[N]     estimated error, Q15, Q31 or floating point
  h[M]     updated impulse response, Q15, Q31 or floating point

  Restriction:
  x,r,h,e  should not overlap
  x,r,h,e  aligned on a 8-bytes boundary
  N,M      multiples of 8 and >0
-------------------------------------------------------------------------*/
void fir_blms16x16 (  int16_t *  restrict e, int16_t * restrict h,
                const int16_t * restrict r,
                const int16_t * restrict x,
                int16_t norm, int16_t   mu,
                int   N,   int   M);
void fir_blms24x24 (  f24 * restrict e, f24 * restrict h,
                const f24 * restrict r,
                const f24 * restrict x,
                f24   norm,f24   mu,
                int   N,   int   M);
void fir_blms16x32 (  int32_t * restrict e, int32_t * restrict h,
                const int16_t * restrict r,
                const int16_t * restrict x,
                int32_t   norm,int16_t   mu,
                int       N,   int       M);
void fir_blms32x32 (  int32_t * restrict e, int32_t * restrict h,
                const int32_t * restrict r,
                const int32_t * restrict x,
                int32_t   norm, int32_t mu,
                int       N,   int       M);
void fir_blmsf      ( float32_t * e, float32_t * h, const float32_t * r,
                const float32_t * x, 
                float32_t norm, float32_t mu, 
                int          N, int       M );


#ifdef __cplusplus
}
#endif

#endif /* __NATUREDSP_SIGNAL_FIR_H__ */
