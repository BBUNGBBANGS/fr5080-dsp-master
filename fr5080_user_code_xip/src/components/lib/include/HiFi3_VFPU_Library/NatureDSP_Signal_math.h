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
#ifndef __NATUREDSP_SIGNAL_MATH_H__
#define __NATUREDSP_SIGNAL_MATH_H__

#include "NatureDSP_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
  Matematics:
  vec_recip           Reciprocal on Q31/Q15 Numbers
  vec_divide          Division of Q31/Q15 Numbers
  vec_log             Logarithm 
  vec_antilog         Antilogarithm         
  vec_sqrt            Square Root
  vec_rsqrt	          Reciprocal Square Root
  vec_sine,vec_cosine Sine/Cosine
  vec_tan             Tangent 
  vec_atan            Arctangent 
  vec_atan2           Full Quadrant Arctangent
  vec_tanh            Hyperbolic Tangent
  vec_sigmoid         Sigmoid
  vec_softmax         Softmax
  vec_int2float       Integer to Float Conversion 
  vec_float2int       Float to Integer Conversion 
===========================================================================*/
/*-------------------------------------------------------------------------
  Reciprocal on Q31/Q15 Numbers
  These routines return the fractional and exponential portion of the 
  reciprocal of a vector x of Q31 or Q15 numbers. Since the reciprocal is 
  always greater than 1, it returns fractional portion frac in Q(31-exp) 
  or Q(15-exp) format and exponent exp so true reciprocal value in the 
  Q0.31/Q0.15 may be found by shifting fractional part left by exponent 
  value.

  Mantissa accuracy is 1 LSB, so relative accuracy is:
  vec_recip16x16, scl_recip16x16                   6.2e-5 
  vec_recip24x24, scl_recip32x32, scl_recip24x24   2.4e-7 
  vec_recip32x32                                   9.2e-10

  Precision: 
  32x32  32-bit input, 32-bit output. 
  24x24  24-bit input, 24-bit output. 
  16x16  16-bit input, 16-bit output. 

  Input:
  x[N]    input data, Q31 or Q15
  N       length of vectors

  Output:
  frac[N] fractional part of result, Q(31-exp) or Q(15-exp)
  exp[N]  exponent of result 

  Restriction:
  x,frac,exp should not overlap

  PERFORMANCE NOTE:
  for optimum performance follow rules:
  frac,x   - aligned on 8-byte boundary
  N   - multiple of 4 and >4

  Scalar versions:
  ----------------
  Return packed value: 
  scl_recip24x24(),scl_recip32x32():
  bits 23:0 fractional part
  bits 31:24 exponent
  scl_recip16x16():
  bits 15:0 fractional part
  bits 31:16 exponent
-------------------------------------------------------------------------*/
void vec_recip32x32 (
                  int32_t * restrict frac, 
                  int16_t *exp, 
                  const int32_t * restrict x, 
                  int N);
void vec_recip24x24 (
                  f24 * restrict frac, 
                  int16_t *exp, 
                  const f24 * restrict x, 
                  int N);
void vec_recip16x16 (
                  int16_t * restrict frac, 
                  int16_t *exp, 
                  const int16_t * restrict x, 
                  int N);
uint32_t scl_recip32x32 (int32_t x);
uint32_t scl_recip24x24 (f24 x);
uint32_t scl_recip16x16 (int16_t x);

/*-------------------------------------------------------------------------
  Division of Q31/Q15 Numbers
  These routines perform pair-wise division of vectors written in Q31 or Q15 
  format. They return the fractional and exponential portion of the division 
  result. Since the division may generate result greater than 1, it returns 
  fractional portion frac in Q(31-exp) or Q(15-exp) format and exponent 
  exp so true division result in the Q0.31 may be found by shifting 
  fractional part left by exponent value.
  For division to 0, the result is not defined 

  Two versions of routines are available: regular versions (vec_divide32x32, 
  vec_divide24x24, vec_divide16x16) work with arbitrary arguments, faster 
  versions (vec_divide32x32_fast, vec_divide24x24_fast, vec_divide16x16_fast) 
  apply some restrictions.

  Mantissa accuracy is 2 LSB, so relative accuracy is:
  vec_divide16x16, scl_divide16x16                   1.2e-4 
  vec_divide24x24, scl_divide32x32, scl_divide24x24  4.8e-7 
  vec_divide32x32                                    1.8e-9

  Precision: 
  32x32  32-bit inputs, 32-bit output. 
  24x24  24-bit inputs, 24-bit output. 
  16x16  16-bit inputs, 16-bit output. 

  Input:
  x[N]    nominator,Q31 or Q15
  y[N]    denominator,Q31 or Q15
  N       length of vectors
  Output:
  frac[N] fractional parts of result, Q(31-exp) or Q(15-exp)
  exp[N]  exponents of result 

  Restriction:
  For regular versions (vec_divide32x32, vec_divide24x24, 
  vec_divide16x16) :
  x,y,frac,exp should not overlap

  For faster versions (vec_divide32x3_fast, vec_divide24x24_fast, 
  vec_divide16x16_fast) :
  x,y,frac,exp should not overlap
  x, y, frac to be aligned by 8-byte boundary, N - multiple of 4.

  Scalar versions:
  ----------------
  Return packed value: 
  scl_divide24x24(),scl_divide32x32():
  bits 23:0 fractional part
  bits 31:24 exponent
  scl_divide16x16():
  bits 15:0 fractional part
  bits 31:16 exponent
-------------------------------------------------------------------------*/
void vec_divide32x32 
                (int32_t * restrict frac, 
                 int16_t *exp,
                 const int32_t * restrict x, 
                 const int32_t * restrict y, int N);
void vec_divide24x24 
                (    f24 * restrict frac, 
                 int16_t * exp,
                 const f24 * restrict x, 
                 const f24 * restrict y, int N) ;
void vec_divide16x16 
                (int16_t * restrict frac, 
                 int16_t *exp,
                 const int16_t * restrict x, 
                 const int16_t * restrict y, int N);
void vec_divide32x32_fast 
                (int32_t * restrict frac, 
                 int16_t * exp,
                 const int32_t * restrict x, 
                 const int32_t * restrict y, int N);
void vec_divide24x24_fast 
                (    f24 * restrict frac, 
                 int16_t * exp,
                 const f24 * restrict x, 
                 const f24 * restrict y, int N) ;
void vec_divide16x16_fast 
                (int16_t * restrict frac, 
                 int16_t * exp,
                 const int16_t * restrict x, 
                 const int16_t * restrict y, int N);

uint32_t scl_divide32x32(int32_t x,int32_t y);
uint32_t scl_divide24x24(f24     x,f24     y);
uint32_t scl_divide16x16(int16_t x,int16_t y);

/*-------------------------------------------------------------------------
  Logarithm:
  Different kinds of logarithm (base 2, natural, base 10). Fixed point 
  functions represent results in Q25 format or return 0x80000000 on negative 
  of zero input.

  Precision:
  32x32  32-bit inputs, 32-bit outputs
  24x24  24-bit inputs, 24-bit outputs
  f      floating point

  Accuracy :
  vec_log2_32x32,scl_log2_32x32  , vec_log2_24x24,scl_log2_24x24     730 (2.2e-5)
  vec_logn_32x32,scl_logn_32x32  , vec_logn_24x24,scl_logn_24x24     510 (1.5e-5)
  vec_log10_32x32,scl_log10_32x32, vec_log10_24x24,scl_log10_24x24   230 (6.9e-6)
  floating point                                                     2 ULP

  NOTES:
  1.  Although 32 and 24 bit functions provide the same accuracy, 32-bit 
      functions have better input/output resolution (dynamic range)
  2.  Scalar Floating point functions are compatible with standard ANSI C routines 
      and set errno and exception flags accordingly.
  3.  Floating point functions limit the range of allowable input values:
      A) If x<0, the result is set to NaN. In addition, scalar floating point
         functions assign the value EDOM to errno and raise the "invalid" 
         floating-point exception.
      B) If x==0, the result is set to minus infinity. Scalar floating  point
         functions assign the value ERANGE to errno and raise the "divide-by-zero"
         floating-point exception.

  Input:
  x[N]  input data, Q16.15 or floating point 
  N     length of vectors
  Output:
  y[N]  result, Q25 or floating point 

  Restriction:
  x,y should not overlap

  Scalar versions:
  ----------------
  return result in Q25 or floating point
-------------------------------------------------------------------------*/
void vec_log2_32x32 (int32_t * restrict y,const int32_t * restrict x, int N);
void vec_logn_32x32 (int32_t * restrict y,const int32_t * restrict x, int N);
void vec_log10_32x32(int32_t * restrict y,const int32_t * restrict x, int N);
void vec_log2_24x24 (f24 * restrict y,const f24 * restrict x, int N);
void vec_logn_24x24 (f24 * restrict y,const f24 * restrict x, int N);
void vec_log10_24x24(f24 * restrict y,const f24 * restrict x, int N);
void vec_log2f     (float32_t * restrict y,const float32_t * restrict x, int N);
void vec_lognf     (float32_t * restrict y,const float32_t * restrict x, int N);
void vec_log10f    (float32_t * restrict y,const float32_t * restrict x, int N);
int32_t scl_log2_32x32 (int32_t x);
int32_t scl_logn_32x32 (int32_t x);
int32_t scl_log10_32x32(int32_t x);
f24 scl_log2_24x24 (f24 x);
f24 scl_logn_24x24 (f24 x);
f24 scl_log10_24x24(f24 x);
float32_t scl_log2f (float32_t x);
float32_t scl_lognf (float32_t x);
float32_t scl_log10f(float32_t x);

/*-------------------------------------------------------------------------
  Antilogarithm
  These routines calculate antilogarithm (base2, natural and base10). 
  Fixed-point functions accept inputs in Q25 and form outputs in Q16.15 
  format and return 0x7FFFFFFF in case of overflow and 0 in case of 
  underflow.

  Precision:
  32x32  32-bit inputs, 32-bit outputs. Accuracy: 8*e-6*y+1LSB
  24x24  24-bit inputs, 24-bit outputs. Accuracy: 8*e-6*y+1LSB
  f      floating point: Accuracy: 2 ULP
  NOTE:
  1.  Although 32 and 24 bit functions provide the similar accuracy, 32-bit
      functions have better input/output resolution (dynamic range).
  2.  Floating point functions are compatible with standard ANSI C routines 
      and set errno and exception flags accordingly.

  Input:
  x[N]  input data,Q25 or floating point 
  N     length of vectors
  Output:
  y[N]  output data,Q16.15 or floating point  

  Restriction:
  x,y should not overlap

  Scalar versions:
  ----------------
  fixed point functions return result in Q16.15

  PERFORMANCE NOTE:
  for optimum performance follow rules:
  x,y - aligned on 8-byte boundary
  N   - multiple of 2
-------------------------------------------------------------------------*/
void vec_antilog2_32x32 (int32_t * restrict y, const int32_t* restrict x, int N);
void vec_antilogn_32x32 (int32_t * restrict y, const int32_t* restrict x, int N);
void vec_antilog10_32x32(int32_t * restrict y, const int32_t* restrict x, int N);
void vec_antilog2_24x24 (f24 * restrict y, const f24* restrict x, int N);
void vec_antilogn_24x24 (f24 * restrict y, const f24* restrict x, int N);
void vec_antilog10_24x24(f24 * restrict y, const f24* restrict x, int N);
void vec_antilog2f (float32_t * restrict y, const float32_t* restrict x, int N);
void vec_antilognf (float32_t * restrict y, const float32_t* restrict x, int N);
void vec_antilog10f(float32_t * restrict y, const float32_t* restrict x, int N);
int32_t scl_antilog2_32x32 (int32_t x);
int32_t scl_antilogn_32x32 (int32_t x);
int32_t scl_antilog10_32x32(int32_t x);
f24 scl_antilog2_24x24 (f24 x);
f24 scl_antilogn_24x24 (f24 x);
f24 scl_antilog10_24x24(f24 x);
float32_t scl_antilog2f (float32_t x);
float32_t scl_antilognf (float32_t x);
float32_t scl_antilog10f(float32_t x);

/*-------------------------------------------------------------------------
  Square Root
  These routines calculate square root.
  NOTE: functions return 0x80000000 on negative argument for 32-bit outputs
  or 0x8000 for 16-bit outputs.
  Two versions of functions available: regular version (vec_sqrt16x16,
  vec_sqrt24x24, vec_sqrt32x32, vec_sqrt64x32) with arbitrary arguments and
  faster version (vec_sqrt24x24_fast, vec_sqrt32x32_fast) that apply some restrictions.

  Precision: 
  16x16  16-bit inputs, 16-bit output. Accuracy: 2LSB
  24x24  24-bit inputs, 24-bit output. Accuracy: (2.6e-7*y+1LSB)
  32x32  32-bit inputs, 32-bit output. Accuracy: (2.6e-7*y+1LSB)
  64x32  64-bit inputs, 32-bit output. Accuracy: 2LSB

  Input:
  x[N]  input data, Q15, Q31, Q63 
  N     length of vectors
  Output:
  y[N]  output data, Q15, Q31

  Restriction:
  Regular versions (vec_sqrt16x16, vec_sqrt24x24, vec_sqrt32x32, vec_sqrt64x32):
  x,y - should not overlap

  Faster versions (vec_sqrt24x24_fast, vec_sqrt32x32_fast):
  x,y - should not overlap
  x,y - aligned on 8-byte boundary
  N   - multiple of 2

  Scalar versions:
  ----------------
  return result, Q15, Q31
-------------------------------------------------------------------------*/
void vec_sqrt16x16(int16_t* restrict y, const int16_t* restrict x, int N);
void vec_sqrt24x24 (f24 * restrict y, const f24 * restrict x, int N);
void vec_sqrt24x24_fast (f24 * restrict y, const f24 * restrict x, int N);
void vec_sqrt32x32 (int32_t* restrict y, const int32_t* restrict x, int N);
void vec_sqrt32x32_fast (int32_t* restrict y, const int32_t* restrict x, int N);
void vec_sqrt64x32(int32_t* restrict y, const int64_t* restrict x, int N);
int16_t scl_sqrt16x16(int16_t x);
f24 scl_sqrt24x24(f24 x);
int32_t scl_sqrt32x32(int32_t x);
int32_t scl_sqrt64x32(int64_t x);

/*-------------------------------------------------------------------------
  Reciprocal Square Root
  These routines return the fractional and exponential portion of the 
  reciprocal square root of a vector x of Q31 or Q15 numbers. Since the 
  reciprocal square root is always greater than 1, they return fractional 
  portion frac in Q(31-exp) or Q(15-exp) format and exponent exp so true 
  reciprocal value in the Q0.31/Q0.15 may be found by shifting fractional 
  part left by exponent value.

  Mantissa accuracy is 1 LSB, so relative accuracy is:
  vec_rsqrt16x16, scl_rsqrt16x16	6.2e-5
  scl_rsqrt32x32	                2.4e-7
  vec_rsqrt32x32	                9.2e-10

  Precision: 
  16x16  16-bit inputs, 16-bit output. Accuracy: 2LSB
  32x32  32-bit inputs, 32-bit output. Accuracy: (2.6e-7*y+1LSB)

  Input:
  x[N]     input data, Q15, Q31 
  N        length of vectors
  Output:
  frac[N]  fractional part of result, Q(31-exp) or Q(15-exp)
  exp[N]   exponent of result 

  Restriction:
  x, fract, exp - should not overlap

  Scalar versions:
  ----------------
  Returned packed value: 
  scl_rsqrt32x32():
  bits 23…0 fractional part
  bits 31…24 exponent
  scl_rsqrt16x16():
  bits 15…0 fractional part
  bits 31…16 exponent

-------------------------------------------------------------------------*/
void vec_rsqrt32x32 (
                  int32_t * frac, 
                  int16_t * exp,
            const int32_t *  x, 
            int N);
void vec_rsqrt16x16 (
                  int16_t * frac, 
                  int16_t * exp,
            const int16_t *  x, 
            int N);
uint32_t scl_rsqrt32x32(int32_t x);
uint32_t scl_rsqrt16x16(int16_t x);

/*-------------------------------------------------------------------------
  Sine/Cosine 
  Fixed-point functions calculate sin(pi*x) or cos(pi*x) for numbers written 
  in Q31 or Q15 format. Return results in the same format. 
  Floating point functions compute sin(x) or cos(x)
  Two versions of functions available: regular version (vec_sine24x24, 
  vec_cosine24x24, vec_sine32x32, vec_cosine32x32, , vec_sinef, vec_cosinef) 
  with arbitrary arguments and faster version (vec_sine24x24_fast, 
  vec_cosine24x24_fast, vec_sine32x32_fast, vec_cosine32x32_fast) that 
  apply some restrictions.
  NOTE:
  1.  Scalar floating point functions are compatible with standard ANSI C
      routines and set errno and exception flags accordingly
  2.  Floating point functions limit the range of allowable input values:
      [-102940.0, 102940.0] Whenever the input value does not belong to this
      range, the result is set to NaN.

  Precision: 
  24x24  24-bit inputs, 24-bit output. Accuracy: 74000(3.4e-5)
  32x32  32-bit inputs, 32-bit output. Accuracy: 1700 (7.9e-7)
  f      floating point. Accuracy 2 ULP

  Input:
  x[N]  input data,Q31 or floating point
  N     length of vectors
  Output:
  y[N]  output data,Q31 or floating point

  Restriction:
  Regular versions (vec_sine24x24, vec_cosine24x24, vec_sine32x32, 
  vec_cosine32x32, vec_sinef, vec_cosinef):
  x,y - should not overlap

  Faster versions (vec_sine24x24_fast, vec_cosine24x24_fast, 
  vec_sine32x32_fast, vec_cosine32x32_fast):
  x,y - should not overlap
  x,y - aligned on 8-byte boundary
  N   - multiple of 2

  Scalar versions:
  ----------------
  return result in Q31 or floating point
-------------------------------------------------------------------------*/
void vec_sine24x24 (  f24 * restrict y,
                const f24 * restrict x,
                int N);
void vec_sine24x24_fast (  f24 * restrict y,
                const f24 * restrict x,
                int N);
void vec_cosine24x24 (f24 * restrict y,
                const f24 * restrict x,
                int N);
void vec_cosine24x24_fast (f24 * restrict y,
                const f24 * restrict x,
                int N);
void vec_sine32x32 (  int32_t * restrict y,
                const int32_t * restrict x,
                int N);
void vec_sine32x32_fast (  int32_t * restrict y,
                const int32_t * restrict x,
                int N);
void vec_cosine32x32 (int32_t * restrict y,
                const int32_t * restrict x,
                int N);
void vec_cosine32x32_fast (int32_t * restrict y,
                const int32_t * restrict x,
                int N);
void vec_sinef     (  float32_t * restrict y,
                const float32_t * restrict x,
                int N);
void vec_cosinef(     float32_t * restrict y,
                const float32_t * restrict x,
                int N);
f24 scl_sine24x24   (f24 x);
f24 scl_cosine24x24 (f24 x);
int32_t scl_sine32x32   (int32_t x);
int32_t scl_cosine32x32 (int32_t x);
float32_t scl_sinef   (float32_t x);
float32_t scl_cosinef (float32_t x);

/*-------------------------------------------------------------------------
  Tangent 
  Fixed point functions calculate tan(pi*x) for number written in Q31. 
  Floating point functions compute tan(x)
  
  Precision: 
  24x24  24-bit inputs, 32-bit outputs. Accuracy: (1.3e-4*y+1LSB) 
                                        if abs(y)<=464873(14.19 in Q15) 
                                        or abs(x)<pi*0.4776
  32x32  32-bit inputs, 32-bit outputs. Accuracy: (1.3e-4*y+1LSB)
                                        if abs(y)<=464873(14.19 in Q15) 
                                        or abs(x)<pi*0.4776
  f      floating point.                Accuracy: 2 ULP

  NOTE:
  1.  Scalar floating point function is compatible with standard ANSI C routines 
      and set errno and exception flags accordingly
  2.  Floating point functions limit the range of allowable input values: [-9099, 9099]
      Whenever the input value does not belong to this range, the result is set to NaN.

  Input:
  x[N]   input data,Q31 or floating point
  N      length of vectors
  Output:
  y[N]   result, Q16.15 or floating point

  Restriction:
  x,y - should not overlap

  PERFORMANCE NOTE:
  for optimum performance follow rules:
  x,z - aligned on 8-byte boundary
  N   - multiple of 2

  Scalar versions:
  ----------------
  Return result, Q16.15 or floating point
-------------------------------------------------------------------------*/
void vec_tan24x24 (
              int32_t* restrict   y, 
              const f24 *restrict x,
              int N);
void vec_tan32x32 ( 
              int32_t* restrict   y, 
              const int32_t *restrict x,
              int N);
void vec_tanf ( 
                    float32_t *restrict y, 
              const float32_t *restrict x,
              int N);
int32_t scl_tan24x24 (f24 x);
int32_t scl_tan32x32 (int32_t x);
float32_t scl_tanf (float32_t x);

/*-------------------------------------------------------------------------
  Arctangent 
  Functions calculate arctangent of number. Fixed point functions 
  scale output to pi so it is always in range -0x20000000 : 0x20000000 
  which corresponds to the real phases +pi/4 and represent input and output 
  in Q31
  NOTE:
  1.  Scalar floating point function is compatible with standard ANSI C
      routines and sets errno and exception flags accordingly

  Accuracy:
  24 bit version: 74000 (3.4e-5) 
  32 bit version: 42    (2.0e-8)
  floating point: 2 ULP

  Precision: 
  24x24  24-bit inputs, 24-bit output. 
  32x32  32-bit inputs, 32-bit output.
  f      floating point

  Input:
  x[N]   input data, Q31 or floating point
  N      length of vectors
  Output:
  z[N]   result, Q31 or floating point

  Restriction:
  x,z should not overlap

  PERFORMANCE NOTE:
  for optimum performance follow rules:
  x,z - aligned on 8-byte boundary
  N   - multiple of 2

  Scalar versions:
  ----------------
  return result, Q31 or floating point
-------------------------------------------------------------------------*/
void vec_atan24x24 (f24 * restrict z, 
              const f24 * restrict x, 
              int N );
void vec_atan32x32 (int32_t * restrict z, 
              const int32_t * restrict x, 
              int N );
void vec_atanf (    float32_t * restrict z, 
              const float32_t * restrict x, 
              int N );
f24 scl_atan24x24 (f24 x);
int32_t scl_atan32x32 (int32_t x);
float32_t scl_atanf (float32_t x);

/*-------------------------------------------------------------------------
  Full-Quadrant Arc Tangent
  The functions compute the arc tangent of the ratios y[N]/x[N] and store the
  result to output vector z[N]. 
  Floating point functions output is in radians. Fixed point functions
  scale its output by pi.

  NOTE:
  1. Scalar floating point function is compatible with standard ANSI C routines and set 
     errno and exception flags accordingly
  2. Scalar floating point function assigns EDOM to errno whenever y==0 and x==0.

  Accuracy:
  24 bit version: 768 (3.57e-7)
  floating point: 2 ULP

  Special cases:
       y    |   x   |  result   |  extra conditions    
    --------|-------|-----------|---------------------
     +/-0   | -0    | +/-pi     |
     +/-0   | +0    | +/-0      |
     +/-0   |  x    | +/-pi     | x<0
     +/-0   |  x    | +/-0      | x>0
     y      | +/-0  | -pi/2     | y<0
     y      | +/-0  |  pi/2     | y>0
     +/-y   | -inf  | +/-pi     | finite y>0
     +/-y   | +inf  | +/-0      | finite y>0
     +/-inf | x     | +/-pi/2   | finite x
     +/-inf | -inf  | +/-3*pi/4 | 
     +/-inf | +inf  | +/-pi/4   |

  Input:
    y[N]  vector of numerator values, Q31 or floating point
    x[N]  vector of denominator values, Q31 or floating point
    N     length of vectors
  Output:
    z[N]  results, Q31 or floating point
  
---------------------------------------------------------------------------*/
void vec_atan2_24x24(f24 * z, const f24 * y, const f24 * x, int N);
void vec_atan2f (float32_t * z, const float32_t * y, const float32_t * x,  int N );
f24 scl_atan2_24x24(f24 re, f24 im);
float32_t scl_atan2f (float32_t y, float32_t x);

/*-------------------------------------------------------------------------
  Hyperbolic Tangent
  The functions compute the hyperbolic tangent of input argument. 32-bit
  fixed-point functions accept inputs in Q6.25 and form outputs in Q16.15
  format.

  Precision:
  32x32  32-bit inputs, 32-bit output. Accuracy: 2 LSB.

  Input:
  x[N]   input data, Q6.25  
  N      length of vectors
  Output:
  y[N]   result, Q16.15 

  Restriction:
  x,y should not overlap

  Scalar versions:
  ----------------
  return result, Q16.15 
-------------------------------------------------------------------------*/
void vec_tanh32x32(int32_t * y, const int32_t * x, int N);
int32_t scl_tanh32x32(int32_t x);

/*-------------------------------------------------------------------------
  Sigmoid
  The functions compute the sigmoid of input argument. 32-bit fixed-point 
  functions accept inputs in Q6.25 and form outputs in Q16.15 format.

  Precision:
  32x32  32-bit inputs, 32-bit output. Accuracy: 2 LSB.

  Input:
  x[N]   input data, Q6.25
  N      length of vectors
  Output:
  y[N]   result, Q16.15

  Restriction:
  x,y should not overlap

  Scalar versions:
  ----------------
  return result, Q16.15
-------------------------------------------------------------------------*/
void vec_sigmoid32x32(int32_t * y, const int32_t * x, int N);
int32_t scl_sigmoid32x32(int32_t x);

/*-------------------------------------------------------------------------
  Softmax
  The function computes the softmax (normalized exponential function) of 
  input data. 32-bit fixed-point functions accept inputs in Q6.25 and form 
  outputs in Q16.15 format. 

  Precision:
  32x32  32-bit inputs, 32-bit output. Accuracy: 2 LSB (see Note below)

  Note: Accuracy of function may depend on amount of data and their 
  distribution. Given accuracy is achieved for N=2 for any pair of data 
  from input domain.

  Input:
  x[N]   input data, Q6.25
  N      length of vectors
  Output:
  y[N]   result, Q16.15

  Restriction:
  x,y should not overlap

-------------------------------------------------------------------------*/
void vec_softmax32x32(int32_t * y, const int32_t * x, int N);

/*-------------------------------------------------------------------------
  Integer to float conversion
  Routine converts integer to float and scales result up by 2^t.

  Precision: 
  f     32-bit input, floating point output

  Input:
  x[N]  Input data, integer
  t     scale factor
  N     length of vector
  Output:
  y[N]  Conversion results, floating point

  Restriction:
  t should be in range -126...126
-------------------------------------------------------------------------*/
void   vec_int2float ( float32_t  * restrict y, const int32_t  * restrict x, int t, int N);
float32_t scl_int2float (int32_t x, int t);

/*-------------------------------------------------------------------------
  Float to integer conversion
  routine scale floating point input down by 2^t and convert it to integer 
  with saturation

  Precision: 
  f     single precision floating point

  Input:
  x[N]  Input data, floating point
  t     scale factor
  N     length of vector
  Output:
  y[N]  Conversion results, integer

  Restriction:
  t should be in range -126...126
-------------------------------------------------------------------------*/
void   vec_float2int (  int32_t * restrict y, const float32_t * restrict x, int t, int N);
int32_t scl_float2int (float32_t x, int t);

#ifdef __cplusplus
}
#endif

#endif//__NATUREDSP_SIGNAL_MATH_H__
