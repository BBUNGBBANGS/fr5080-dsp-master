// ints-on.S - Interrupt related assembler code - _xtos_ints_on

// Copyright (c) 2004-2015 Tensilica Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <xtensa/coreasm.h>
#include "xtos-internal.h"


/***************************************************************************
 *  _xtos_ints_on() and _xtos_ints_off() are used
 *  to enable and disable interrupts from C code;
 *  they can be called from the application or from a C interrupt handler.
 */


// u32  _xtos_ints_on( u32 mask );
//	Enables a set of interrupts.
//	With INTENABLE virtualizing, does not simply set INTENABLE directly, but rather
//	computes it as a function of the current virtual priority.
//
//	MUST NOT be called when PS.INTLEVEL > XTOS_LOCKLEVEL
//	(otherwise PS.INTLEVEL gets lowered; and operation may be inconsistent
//	 if this is called in the handler of an interrupt of level > LOCKLEVEL).
//
	.text
	.align 4
	.global _xtos_ints_on
	.type _xtos_ints_on,@function
_xtos_ints_on:
	abi_entry
#if XCHAL_HAVE_INTERRUPTS && (XCHAL_HAVE_XEA1 || XCHAL_HAVE_XEA2)
# if XTOS_VIRTUAL_INTENABLE
	movi	a4, _xtos_intstruct
	xtos_lock	a7	// MUST USE highest address register of function to avoid window overflows in critical section
	l32i	a3, a4, XTOS_ENABLED_OFS	// a3 = xtos_enabled
	l32i	a6, a4, XTOS_VPRI_ENABLED_OFS	// a6 = xtos_vpri_enabled
	or	a5, a3, a2			// xtos_enabled | mask
	s32i	a5, a4, XTOS_ENABLED_OFS	// xtos_enabled |= mask
	and	a5, a5, a6			// a5 = xtos_enabled & xtos_vpri_enabled
# else
	xtos_lock	a7	// MUST USE highest address register of function to avoid window overflows in critical section
	rsr.intenable	a3
	//interlock
	or	a5, a3, a2			// INTENABLE | mask
# endif
	wsr.intenable	a5
	xtos_unlock	a7
	mov	a2, a3				// return previous (virtual or real) INTENABLE value
#else /*XCHAL_HAVE_INTERRUPTS*/
	movi	a2, 0		// this config does not have interrupts, so return 0
#endif /*XCHAL_HAVE_INTERRUPTS*/
	abi_return

	.size	_xtos_ints_on, . - _xtos_ints_on

