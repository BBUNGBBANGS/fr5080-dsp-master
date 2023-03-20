/*
 * cpuport.c
 *
 *  Created on: 2018-11-26
 *      Author: owen
 */

#include <stdlib.h>
#include <xtensa/config/core.h>
#include <xtensa/xtruntime.h>

#include "xtensa_rtos.h"

#include "rtdef.h"

volatile rt_ubase_t  rt_interrupt_from_thread = 0;
volatile rt_ubase_t  rt_interrupt_to_thread   = 0;
volatile rt_uint32_t rt_thread_switch_interrupt_flag = 0;

// User exception dispatcher when exiting
void _xt_user_exit(void);

rt_base_t rt_hw_interrupt_disable(void)
{
    return XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);
}

void rt_hw_interrupt_enable(rt_base_t level)
{
    XTOS_RESTORE_JUST_INTLEVEL(level);
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit)
{
    uint32_t * sp, * tp, * pxTopOfStack;
    XtExcFrame * frame;

    #if XCHAL_CP_NUM > 0
        uint32_t * p;
    #endif

    pxTopOfStack = (uint32_t *)stack_addr;

    /* Create interrupt stack frame aligned to 16 byte boundary */
    sp = ( uint32_t * ) ( ( ( uint32_t ) pxTopOfStack - XT_CP_SIZE - XT_STK_FRMSZ ) & ~0xf );

    /* Clear the entire frame (do not use memset() because we don't depend on C library) */
    for( tp = sp; tp <= pxTopOfStack; ++tp )
    {
        *tp = 0;
    }

    frame = ( XtExcFrame * ) sp;

    /* Explicitly initialize certain saved registers */
    frame->pc = ( uint32_t ) tentry;             /* task entrypoint                */
    frame->a0 = 0;                               /* to terminate GDB backtrace     */
    frame->a1 = ( uint32_t ) sp + XT_STK_FRMSZ;  /* physical top of stack frame    */
    frame->exit = ( uint32_t ) _xt_user_exit;    /* user exception exit dispatcher */

    /* Set initial PS to int level 0, EXCM disabled ('rfe' will enable), user mode. */
    /* Also set entry point argument parameter. */
    #ifdef __XTENSA_CALL0_ABI__
        frame->a2 = ( uint32_t ) parameter;
        frame->ps = PS_UM | PS_EXCM;
    #else
        /* + for windowed ABI also set WOE and CALLINC (pretend task was 'call4'd). */
        frame->a6 = ( uint32_t ) parameter;
        frame->ps = PS_UM | PS_EXCM | PS_WOE | PS_CALLINC( 1 );
    #endif

    #ifdef XT_USE_SWPRI
        /* Set the initial virtual priority mask value to all 1's. */
        frame->vpri = 0xFFFFFFFF;
    #endif

    #if XCHAL_CP_NUM > 0
        /* Init the coprocessor save area (see xtensa_context.h) */

        /* No access to TCB here, so derive indirectly. Stack growth is top to bottom.
         * //p = (uint32_t *) xMPUSettings->coproc_area;
         */
        p = ( uint32_t * ) ( ( ( uint32_t ) pxTopOfStack - XT_CP_SIZE ) & ~0xf );
        p[ 0 ] = 0;
        p[ 1 ] = 0;
        p[ 2 ] = ( ( ( uint32_t ) p ) + 12 + XCHAL_TOTAL_SA_ALIGN - 1 ) & -XCHAL_TOTAL_SA_ALIGN;
    #endif

    return (rt_uint8_t *)sp;
}
