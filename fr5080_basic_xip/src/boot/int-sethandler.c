
/* int-sethandler.c - register an interrupt handler in XTOS */

/*
 * Copyright (c) 1999-2017 Cadence Design Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stddef.h>
#include <stdint.h>

#include <xtensa/config/core.h>
#include "xtos-internal.h"


#if XCHAL_HAVE_INTERRUPTS
/*
 *  Table of interrupt handlers.
 *  NOTE:  if the NSA/NSAU instructions are configured, then to save
 *  a few cycles in the interrupt dispatcher code, the
 *  xtos_interrupt_table[] array is filled in reverse.
 *  IMPORTANT:  Use the MAPINT() macro defined in xtos-internal.h to index entries in this array.
 */
extern XtosIntHandlerEntry      xtos_interrupt_table[XCHAL_NUM_INTERRUPTS];
extern void                     xtos_unhandled_interrupt(void *arg);
#endif


_xtos_handler _xtos_set_interrupt_handler_arg( int32_t n, _xtos_handler f, void *arg )
{
#if XCHAL_HAVE_INTERRUPTS
    XtosIntHandlerEntry *entry;
    _xtos_handler old;
    _xtos_handler ret;

    if( (n < 0) || (n >= XCHAL_NUM_INTERRUPTS) ) {
        ret = NULL;     /* invalid interrupt number */
    }
    else if( (int32_t) Xthal_intlevel[n] > XTOS_LOCKLEVEL ) {
        ret = NULL;     /* priority level too high to safely handle in C */
    }
    else {
        entry = xtos_interrupt_table + MAPINT(n);
        old = entry->handler;
        if (f != NULL) {
            entry->handler = f;
            entry->u.varg  = arg;
        } else {
            entry->handler = &xtos_unhandled_interrupt;
            entry->u.narg  = n;
        }
        ret = (old == &xtos_unhandled_interrupt) ? NULL : old;
    }

    return ret;
#else
    return NULL;
#endif
}


_xtos_handler _xtos_set_interrupt_handler( int32_t n, _xtos_handler f )
{
    return _xtos_set_interrupt_handler_arg( n, f, (void *) n );
}

