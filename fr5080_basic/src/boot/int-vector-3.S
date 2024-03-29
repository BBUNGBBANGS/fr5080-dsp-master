// int-vector.S  -  Interrupt Vector Template (for levels > 1)
// $Id: //depot/rel/Foxhill/dot.8/Xtensa/OS/xtos/int-vector.S#1 $

// Copyright (c) 2003-2017 Tensilica Inc.
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


// To use this template file, define a macro called _INTERRUPT_LEVEL
// to be the interrupt level of the vector, include "xtos-internal.h",
// then include this file.  We use the same template for both high-level
// and medium-level interrupts, but not debug level.


#define _ASMLANGUAGE
#include "xtos-internal.h"
#include <xtensa/coreasm.h>
#include <xtensa/config/system.h>

#define _INTERRUPT_LEVEL        3

#if XCHAL_HAVE_INTERRUPTS && (_INTERRUPT_LEVEL <= XCHAL_NUM_INTLEVELS) && (_INTERRUPT_LEVEL != XCHAL_DEBUGLEVEL)

	.begin	literal_prefix	LABEL(.Level,InterruptVector)
	.section		LABEL(.Level,InterruptVector.text), "ax"
	.align 4
	.global	LABEL(_Level,Vector)
LABEL(_Level,Vector):
	//  Medium and high priority interrupt vector:

# if HAVE_XSR
	//  With XSR, we can use this vector which has the advantage of being ROMable
	//  without requiring the handler to also be in ROM; however, it requires
	//  initializing the EXCSAVEn register (see reset-vector.S) and a slightly
	//  different save/restore sequence in the handler:
	hw_erratum_487_fix
	xchgsr	excsave _INTERRUPT_LEVEL a2
	jx	a2

	//  Pull-in the real handler by reference, to ensure the reset vector gets it:
	.global	LABEL(_Level,FromVector)

# else
	writesr	excsave	_INTERRUPT_LEVEL a2
	movi	a2, LABEL(_Level,FromVector)
	jx	a2
# endif

	.size	LABEL(_Level,Vector), . - LABEL(_Level,Vector)
	.text
	.end	literal_prefix

#endif /* interrupt at that level */

