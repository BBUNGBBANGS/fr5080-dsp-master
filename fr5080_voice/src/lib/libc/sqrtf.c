/* sqrtf function */

#include <xtensa/config/core-isa.h>

#if 0

#include "xxfftype.h"
#include "xxsqrt.h"

#else

/* TENSILICA: sqrtf always included in libgcc, either
   hardware accelerated or emulated by softfloat package */

#include "xmath.h"

extern float __ieee754_sqrtf (float x);

#if !XCHAL_HAVE_FP_SQRT

extern unsigned char  __float_exception_flags;

float sqrtf (float x)
{
  /* softfloat will report errors through __float_exception_flags */
  __float_exception_flags = 0;

  x = __ieee754_sqrtf (x);

  if (__float_exception_flags)
    _Feraise (__float_exception_flags);

  return x;
}

#else /* !XCHAL_HAVE_FP_SQRT */

/* hardware implementation */

float sqrtf (float x)
{
  return __ieee754_sqrtf (x);
}

#endif /* !XCHAL_HAVE_FP_SQRT */

#endif /* 0 */

/*
 * Copyright (c) by P.J. Plauger. All rights reserved.
 * Consult your license regarding permissions and restrictions.
V6.50:1611 */
