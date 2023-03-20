/*
 * plf.h
 *
 *  Created on: 2018-7-3
 *      Author: Administrator
 */

#ifndef _PLF_H
#define _PLF_H

#include <xtensa\tie\xt_core.h>

/** @brief Enable interrupts globally in the system.
 * This macro must be used when the initialization phase is over and the interrupts
 * can start being handled by the system.
 */
#define GLOBAL_INT_START()      XT_RSIL(0)

/** @brief Disable interrupts globally in the system.
 * This macro must be used when the system wants to disable all the interrupt
 * it could handle.
 */
#define GLOBAL_INT_STOP()       XT_RSIL(XCHAL_NUM_INTLEVELS)

/** @brief Disable interrupts globally in the system.
 * This macro must be used in conjunction with the @ref GLOBAL_INT_RESTORE macro since this
 * last one will close the brace that the current macro opens.  This means that both
 * macros must be located at the same scope level.
 */
#define GLOBAL_INT_DISABLE()    do {    \
                                    uint32_t tmp_ps;        \
                                    tmp_ps = XT_RSIL(XCHAL_NUM_INTLEVELS);

/** @brief Restore interrupts from the previous global disable.
 * @sa GLOBAL_INT_DISABLE
 */
#define GLOBAL_INT_RESTORE()        XT_WSR_PS(tmp_ps);  \
                                    XT_RSYNC();     \
                                } while(0);

#define QSPI0_DAC_ADDRESS      	0x10000000
#define QSPI1_DAC_ADDRESS      	0x20000000
#define IPC_BASE                0x50000000
#define UART_BASE               0x50010000
#define SYSTEM_BASE             0x50020000
#define QSPI0_BASE             	0x50030000
#define QSPI1_BASE             	0x50040000
#define TUBE_BASE               0x500F8888

#define XCHAL_TIMER0                    0
#define XCHAL_TIMER1                    1
#define XCHAL_TIMER2                    2
#define XCHAL_IPC_INTERRUPT             3
#define XCHAL_IPC_DMA_RX_INTERRUPT      4
#define XCHAL_IPC_DMA_TX_INTERRUPT      5
#define XCHAL_UART_INTERRUPT            6

#endif /* _PLF_H */
