#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <xtensa/tie/xt_interrupt.h>

#include <xtensa/xtruntime.h>
#include <xtensa/hal.h>

#include "tasks.h"

#include "plf.h"
#include "ipc.h"

static void (*user_ipc_rx)(struct ipc_msg_t *msg, uint8_t chn) = NULL;

static void app_ipc_rx(struct ipc_msg_t *msg, uint8_t chn)
{
    uint8_t channel;
    struct ipc_msg_load_code_t *load_code;
    struct ipc_msg_exec_user_code_t *exec_user_code;
    uint32_t length, *src, *dst;

    switch(msg->format) {
        case IPC_MSG_LOAD_CODE:
            load_code = (struct ipc_msg_load_code_t *)ipc_get_buffer_offset(IPC_DIR_MCU2DSP, msg->tog);
            length = (msg->length - 4) >> 2;
            dst = (void *)load_code->dest;
            src = (void *)&load_code->data[0];
            for(uint32_t i=0; i<length; i++) {
                *dst++ = *src++;
            }
            channel = ipc_alloc_channel(0);
            ipc_insert_msg(channel, IPC_MSG_LOAD_CODE_DONE, 0, ipc_free_channel);
            break;
        case IPC_MSG_EXEC_USER_CODE:
            exec_user_code = (struct ipc_msg_exec_user_code_t *)ipc_get_buffer_offset(IPC_DIR_MCU2DSP, msg->tog);
            void (*func)(void) = (void *)*exec_user_code->entry_ptr;
            func();
            break;
        default:
            if(user_ipc_rx) {
                user_ipc_rx(msg, chn);
            }
            break;
    }

    ipc_clear_msg(chn);
}

__attribute__((section("need_kept"))) void app_ipc_rx_set_user_handler(void *arg)
{
    user_ipc_rx = arg;
}

__attribute__((section("need_kept"))) void app_ipc_ready(void)
{
    uint8_t channel;
    channel = ipc_alloc_channel(0);
    ipc_insert_msg(channel, IPC_MSG_DSP_READY, 0, NULL);
}

static struct task_msg_handler_t app_boot_msg_handler[] =
{
    {TASK_ID_DEFAULT,       NULL},
};

__attribute__((section("need_kept"))) void app_register_default_task_handler(void *arg)
{
    for(uint32_t i=0; i<sizeof(app_boot_msg_handler)/sizeof(app_boot_msg_handler[0]); i++) {
        if(app_boot_msg_handler[i].id == TASK_ID_DEFAULT) {
            app_boot_msg_handler[i].func = arg;
            break;
        }
    }
}

void app_ipc_init(void)
{
    ipc_init(IPC_MSGIN00_INT|IPC_MSGIN01_INT
                |IPC_MSGIN10_INT|IPC_MSGIN11_INT
                |IPC_MSGOUT00_INT|IPC_MSGOUT01_INT
                |IPC_MSGOUT10_INT|IPC_MSGOUT11_INT, app_ipc_rx);

    _xtos_interrupt_enable(XCHAL_IPC_INTERRUPT);
    GLOBAL_INT_START();

    app_ipc_ready();
    
    task_schedule(app_boot_msg_handler, sizeof(app_boot_msg_handler)/sizeof(app_boot_msg_handler[0]));
}
