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
#ifndef __NATUREDSP_SIGNAL_MATOP_H__
#define __NATUREDSP_SIGNAL_MATOP_H__

#include "NatureDSP_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
  Matrix Operations:
  mtx_mpy              Matrix Multiply
  mtx_vecmpy           Matrix by Vector Multiply
===========================================================================*/
/*-------------------------------------------------------------------------
  Matrix Multiply
  These functions compute the expression z = 2^lsh * x * y for the matrices 
  x and y. The columnar dimension of x must match the row dimension of y. 
  The resulting matrix has the same number of rows as x and the same number 
  of columns as y.
  NOTE: lsh factor is not relevant for floating point routines.

  Functions require scratch memory for storing intermediate data. This 
  scratch memory area should be aligned on 8 byte boundary and its size is 
  calculated by macros SCRATCH_MTX_MPY24X24(M,N,P), 
  SCRATCH_MTX_MPY32X32(M,N,P), SCRATCH_MTX_MPY16X16(M,N,P).

  Two versions of functions available: regular version (mtx_mpy32x32,
  mtx_mpy24x24, mtx_mpy16x16, mtx_mpyf) with arbitrary arguments and faster
  version mtx_mpy32x32_fast, mtx_mpy24x24_fast, mtx_mpy16x16_fast, 
  mtx_mpyf_fast) that apply some restrictions.

  Precision:
  32x32 32-bit inputs, 32-bit output 
  24x24 24-bit inputs, 24-bit output
  16x16 16-bit inputs, 16-bit output
  f     floating point

  Input:
  x[M*N]      input matrix x, Q15, Q31 or floating point
  y[N*P]      input matrix y, Q15, Q31 or floating point
  M           number of rows in matrix x and z
  N           number of columns in matrix x and number of rows in matrix y
  P           number of columns in matrices y and z
  lsh         left shift applied to the result (applied to the fixed-
              point functions only) 
  Output:
  z[M*P]      output matrix z, Q15, Q31 or floating point 
  Scratch:
  pScr        size in bytes defined by macros SCRATCH_MTX_MPY32X32,
              SCRATCH_MTX_MPY24X24, SCRATCH_MTX_MPY16X16

  Restrictions:
  For regular routines (mtx_mpy32x32, mtx_mpy24x24, mtx_mpy16x16, mtx_mpyf):
  x,y,z   should not overlap

  For faster routines (mtx_mpy32x32_fast, mtx_mpy24x24_fast, 
                       mtx_mpy16x16_fast, mtx_mpyf_fast):
  x,y,z   should not overlap
  x,y,z   aligned on 8-byte boundary
  M,N,P   multiplies of 4
  lsh     should be in range:
          -31...31 for mtx_mpy32x32, mtx_mpy32x32_fast,
                   mtx_mpy24x24, mtx_mpy24x24_fast
          -15...15 for mtx_mpy16x16, mtx_mpy16x16_fast  

-------------------------------------------------------------------------*/
void mtx_mpy32x32 (  void* pScr,
                     int32_t* restrict z,
               const int32_t* restrict x,
               const int32_t* restrict y,
               int M, int N, int P, int lsh );
void mtx_mpy24x24 (  void* pScr,
                     f24* restrict z,
               const f24* restrict x,
               const f24* restrict y,
               int M, int N, int P, int lsh );
void mtx_mpy16x16 (  void* pScr,
                     int16_t* restrict z,
               const int16_t* restrict x,
               const int16_t* restrict y,
               int M, int N, int P, int lsh );
void mtx_mpy32x32_fast (  int32_t* restrict z,
               const int32_t* restrict x,
               const int32_t* restrict y,
               int M, int N, int P, int lsh );
void mtx_mpy24x24_fast (  f24* restrict z,
               const f24* restrict x,
               const f24* restrict y,
               int M, int N, int P, int lsh );
void mtx_mpy16x16_fast (  int16_t* restrict z,
               const int16_t* restrict x,
               const int16_t* restrict y,
               int M, int N, int P, int lsh );
void mtx_mpyf (      float32_t*  restrict z,
               const float32_t*  restrict x,
               const float32_t*  restrict y,
               int M, int N, int P);
void mtx_mpyf_fast (      float32_t*  restrict z,
               const float32_t*  restrict x,
               const float32_t*  restrict y,
               int M, int N, int P);

#define SCRATCH_MTX_MPY32X32(M,N,P) ( (((N)+1)&(~1))*2*sizeof(int32_t) )
#define SCRATCH_MTX_MPY24X24(M,N,P) ((((N)+1)&(~1))*2*sizeof(f24))
#define SCRATCH_MTX_MPY16X16(M,N,P) ((((N)+3)&(~3))*4*sizeof(int16_t))


/*-------------------------------------------------------------------------
  Matrix by Vector Multiply
  These functions compute the expression z = 2^lsh * x * y for the matrices 
  x and vector y. 
  NOTE: lsh factor is not relevant for floating point routines.

  Two versions of functions available: regular version (mtx_vecmpy32x32, 
  mtx_vecmpy24x24, mtx_vecmpy16x16,mtx_vecmpyf) with arbitrary arguments 
  and faster version (mtx_vecmpy32x32_fast, mtx_vecmpy24x24_fast, 
  mtx_vecmpy16x16_fast, mtx_vecmpyf_fast) that apply some restrictions.

  Precision: 
  32x32 32-bit input, 32-bit output
  24x24 24-bit input, 24-bit output
  16x16 16-bit input, 16-bit output
  f     floating point

  Input:
  x[M*N] input matrix,Q31,Q15 or floating point
  y[N]   input vector,Q31,Q15 or floating point
  M      number of rows in matrix x
  N      number of columns in matrix x
  lsh    additional left shift(applied to the fixed-
         point functions only) 
  Output:
  z[M]   output vector,Q31,Q15 or floating point

  Restriction:
  For regular routines (mtx_vecmpy32x32, mtx_vecmpy24x24,
                        mtx_vecmpy16x16, mtx_vecmpyf)
  x,y,z should not overlap

  For faster routines (mtx_vecmpy32x32_fast, mtx_vecmpy24x24_fast,
                       mtx_vecmpy16x16_fast, mtx_vecmpyf_fast)
  x,y,z   should not overlap
  x,y     aligned on 8-byte boundary
  N, M    multiples of 4
  lsh     should be in range:
          -31...31 for mtx_vecmpy32x32, mtx_vecmpy32x32_fast,
                   mtx_vecmpy24x24, mtx_vecmpy24x24_fast
          -15...15 for mtx_vecmpy16x16, mtx_vecmpy16x16_fast  
-------------------------------------------------------------------------*/
void mtx_vecmpy32x32 (  int32_t* restrict z,
               const int32_t* restrict x,
               const int32_t* restrict y,
               int M, int N, int lsh);
void mtx_vecmpy24x24 (  f24* restrict z,
                  const f24* restrict x,
                  const f24* restrict y,
                  int M, int N, int lsh);
void mtx_vecmpy16x16 (  int16_t* restrict z,
                  const int16_t* restrict x,
                  const int16_t* restrict y,
                  int M, int N, int lsh);
void mtx_vecmpy32x32_fast (  int32_t* restrict z,
               const int32_t* restrict x,
               const int32_t* restrict y,
               int M, int N, int lsh);
void mtx_vecmpy24x24_fast (  f24* restrict z,
                  const f24* restrict x,
                  const f24* restrict y,
                  int M, int N, int lsh);
void mtx_vecmpy16x16_fast ( int16_t* restrict z,
                  const int16_t* restrict x,
                  const int16_t* restrict y,
                  int M, int N, int lsh);
void mtx_vecmpyf (      float32_t* restrict z,
                  const float32_t* restrict x,
                  const float32_t* restrict y,
                  int M, int N);
void mtx_vecmpyf_fast ( float32_t* restrict z,
                  const float32_t* restrict x,
                  const float32_t* restrict y,
                  int M, int N);

#ifdef __cplusplus
}
#endif

#endif//__NATUREDSP_SIGNAL_MATOP_H__
