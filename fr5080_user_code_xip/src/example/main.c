/*
 * main.c
 *
 *  Created on: 2018-8-19
 *      Author: Administrator
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <xtensa/xtruntime.h>
#include <xtensa/hal.h>

#include "xa_type_def.h"

#include "ipc.h"
#include "user_def.h"
#include "plf.h"
#include "uart.h"
#include "flash.h"
#include "qspi.h"
#include "audio_algorithm.h"
#include "os_timer.h"
#include "co_mem.h"

#define FLASH_ID_ZB25Q64A           0x17405e
#define	FLASH_ID_25Q64JV            0x1740ef
#define FLASH_ID_DOSI_64M           0x1732f8
#define FLASH_ID_BOYA_128M          0x184068
#define FLASH_ID_PUYA_128M          0x186085
#define FLASH_ID_PUYA_64M           0x176085
#define FLASH_ID_PUYA_32M           0x166085
#define FLASH_ID_PUYA_16M           0x156085
#define FLASH_ID_PUYA_4M            0x136085
#define FLASH_ID_XTX_128M           0x18400B
#define FLASH_ID_UNKNOWN_128M       0x184068
#define FLASH_ID_NORMEM_64M         0x172252
#define FLASH_ID_WINBOND_32M        0x168AEF

void app_ipc_rx_set_user_handler(void *arg);
void app_register_default_task_handler(void *arg);

void init_memory(void);
void app_entry(void);

void sbc_encoder_recv_frame_req(void *);

void mp3_decoder_init(void *arg);
void mp3_decoder_recv_frame(pWORD8 buffer, UWORD32 length);
void mp3_decoder_do_decoder_handler(void *);

void sbc_decoder_recv_frame(uint8_t *buffer, int length);
void sbc_decoder_do_decoder_handler(void *arg);

void audio_init(void *msg);
void audio_algorithm_release(void *msg);
extern void audio_algorithm_ipc_rx_handler(struct task_msg_t *msg);
extern void audio_algorithm_ipc_tx_handler(struct task_msg_t *msg);
extern void app_at_cmd_recv_handler(struct task_msg_t *msg);
extern void os_timer_trigger_handler(struct task_msg_t *msg);

__attribute__((section("entry_point_section"))) const uint32_t entry_point[] = {
    (uint32_t)app_entry,
};

uint8_t btn_sat[2] = {0,0};
uint8_t btn_num = 0xff;

extern struct sbc_info_t sbc_info;

void set_btn(uint8_t btn_num, bool sat)
{
    if(btn_num == 0x01) {
        btn_sat[0] = sat;
    }
    else if(btn_num == 0x02) {
        btn_sat[1] = sat;
    }
}

uint8_t get_btn(uint8_t num)
{
    uint8_t ret = 0;

    if(num == 0) {
        ret = btn_sat[0];
        return ret;
    }
    else if(num == 1) {
        ret = btn_sat[1];
        return ret;
    }
}

void test_flash_read_write(struct task_msg_t *msg)
{
#define TEST_FLASH_READ_WRITE_LENGTH        16
    uint8_t test_data[TEST_FLASH_READ_WRITE_LENGTH];
    uint8_t status[2];
    static uint8_t *ptr = (uint8_t *)0x00184000;
    static uint32_t start_flash_addr = 512*1024;

    printf("TEST flash: dest = %08x, src = %08x\r\n", start_flash_addr, ptr);
    memcpy(test_data, (void *)(start_flash_addr+QSPI1_DAC_ADDRESS), TEST_FLASH_READ_WRITE_LENGTH);
	printf("TEST BEFORE: ");
	for(uint32_t i=0; i<TEST_FLASH_READ_WRITE_LENGTH; i++) {
		printf("%02x ", test_data[i]);
	}
	printf("\r\n");

    qspi_flash_erase(start_flash_addr, 0x1000);
    qspi_flash_write(start_flash_addr, TEST_FLASH_READ_WRITE_LENGTH, (void *)ptr);

    memcpy(test_data, (void *)(start_flash_addr+QSPI1_DAC_ADDRESS), TEST_FLASH_READ_WRITE_LENGTH);
    printf("TEST AFTER: \r\nREAD:  ");
    for(uint32_t i=0; i<TEST_FLASH_READ_WRITE_LENGTH; i++) {
    	printf("%02x ", test_data[i]);
        }
    printf("\r\n");
    printf("CHECK: ");
	for(uint32_t i=0; i<TEST_FLASH_READ_WRITE_LENGTH; i++) {
		printf("%02x ", *ptr++);
    }
	printf("\r\n");

	start_flash_addr += 4096;
	if(start_flash_addr >= 1024*1024) {
		start_flash_addr = 512*1024;
	}
	if((uint32_t)ptr >= 0x00186000) {
		ptr = (uint8_t *)0x00184000;
	}
}

struct os_timer_t test_flash_rw_timer;
void test_flash_rw_timer_function(void *arg)
{
	test_flash_read_write(NULL);
}

struct os_timer_t test_st7796_timer;
void test_st7796_timer_function(void *arg)
{
	static uint8_t st7796_test_counter = 0;
	printf("test_timer_function\r\n");

	if(st7796_test_counter == 0) {
		st7796h_init();
	}

	if(st7796_test_counter == 1) {
		st7796h_draw_solid_rect(0,0,240,280,0xF800);
	}
	else if(st7796_test_counter == 2) {
		st7796h_draw_solid_rect(0,0,240,280,0x07E0);
	}
	else if(st7796_test_counter == 3) {
		st7796h_draw_solid_rect(0,0,240,280,0x001F);
	}

	st7796_test_counter++;
	if(st7796_test_counter >= 4) {
		st7796_test_counter = 1;
	}
}

struct os_timer_t test_rm69330_timer;
void test_rm69330_timer_function(void *arg)
{
	static uint8_t rm69330_test_counter = 0;

	if(rm69330_test_counter == 0) {
		rm69330_init();

		lv_init();
		co_delay_100us(20);
		lv_port_disp_init();
		lv_port_indev_init();
		lv_test_theme_1(lv_theme_night_init(210, NULL));
	}

	if((rm69330_test_counter % 10) == 0){
		lv_task_handler();
	}

    rm69330_test_counter++;
    if(rm69330_test_counter > 250) {
    	rm69330_test_counter = 1;
    }
}

struct os_timer_t test_rm69330_button_timer;
void test_rm69330_button_timer_function(void *arg)
{
	static uint8_t stage = 0;

	if(stage == 1) {
		set_btn(1, 1);
	}
	else if(stage == 2) {
		set_btn(1, 0);
	}
	else if(stage == 3) {
		set_btn(2, 1);
	}
	else if(stage == 4) {
		set_btn(2, 0);
	}

	stage++;
	if(stage > 4) {
		stage = 1;
	}
}
extern const unsigned char gImage_rtkpic[];
extern const unsigned char gImage_rtkpic1[];
struct os_timer_t test_gc9c01_timer;
void test_gc9c01_timer_function(void *arg)
{
	static uint8_t gc9c01_test_counter = 0;

	if(gc9c01_test_counter == 0) {
		lcd_gc9c01_init();
#if 0
		//lcd_gc9c01_set_windows(0, 0, 121, 121);
		lcd_gc9c01_set_windows(186, 202, 186+122-1, 202+122-1);
		lcd_gc9c01_display(gImage_rtkpic, 29768);
#else
		_xtos_interrupt_disable(XCHAL_TIMER1);
		lcd_gc9c01_set_windows(0, 0, 359, 359);
		lcd_gc9c01_display(gImage_rtkpic1, 259200);
#endif
		//while(1);

		lv_init();
		co_delay_100us(20);
		lv_port_disp_init();
		lv_port_indev_init();
		lv_test_theme_1(lv_theme_night_init(210, NULL));
	}

	if((gc9c01_test_counter % 2) == 0){
		lv_task_handler();
	}

	gc9c01_test_counter++;
    if(gc9c01_test_counter > 250) {
    	gc9c01_test_counter = 1;
    }
}

struct os_timer_t test_gc9c01_button_timer;
void test_gc9c01_button_timer_function(void *arg)
{
	static uint8_t stage = 0;

	if(stage == 1) {
		set_btn(1, 1);
	}
	else if(stage == 2) {
		set_btn(1, 0);
	}
	else if(stage == 3) {
		set_btn(2, 1);
	}
	else if(stage == 4) {
		set_btn(2, 0);
	}

	stage++;
	if(stage > 4) {
		stage = 1;
	}
}

static void ipc_rx_user_handler(struct ipc_msg_t *msg, uint8_t chn)
{
    uint8_t channel;

    //printf("ipc_rx_user_handler: msg=%d, length=%d\r\n", msg->format, msg->length);

    switch(msg->format) {
        case IPC_MSG_RAW_FRAME:
            mp3_decoder_recv_frame(ipc_get_buffer_offset(IPC_DIR_MCU2DSP, chn), msg->length);
            break;
        case IPC_MSG_RAW_SBC_FRAME:
            sbc_decoder_recv_frame(ipc_get_buffer_offset(IPC_DIR_MCU2DSP, chn), msg->length);
            break;
        case IPC_MSG_WITHOUT_PAYLOAD:
            switch(msg->length) {
                case IPC_SUB_MSG_DECODER_START:
					{
                        struct task_msg_t *msg = task_msg_alloc(MP3_DECODER_INIT, 1);
                        uint8_t *param = (void *)&msg->param[0];
                        *param = 0;
						task_msg_insert(msg);
					}
                    break;
                case IPC_SUB_MSG_NEED_MORE_SBC:
                	{
						struct task_msg_t *msg = task_msg_alloc(MCU_NEED_MORE_SBC_DATA, 0);
						task_msg_insert(msg);
					}
                    break;
                case IPC_SUB_MSG_REINIT_DECODER:
                    break;
                case IPC_SUB_MSG_NREC_START:
                    printf("audio algorithm start\r\n");
                    {
                        struct task_msg_t *msg;
                        msg = task_msg_alloc(AUDIO_ALGO_CREATE, 0);
                        task_msg_insert(msg);
                    }
                    break;
                case IPC_SUB_MSG_NREC_STOP:
                    printf("audio algorithm release\r\n");
                    {
                        struct task_msg_t *msg;
                        msg = task_msg_alloc(AUDIO_ALGO_DESTROY, 0);
                        task_msg_insert(msg);
                    }
                    break;
                case IPC_SUB_MSG_DECODER_PREP_NEXT:
                    {
                        struct task_msg_t *msg;
                        msg = task_msg_alloc(DECODER_PREPARE_FOR_NEXT, 0);
                        task_msg_insert(msg);
                    }
                    break;
                case IPC_SUB_MSG_DECODER_START_LOCAL:
                    {
                        struct task_msg_t *msg = task_msg_alloc(MP3_DECODER_INIT, 1);
                        uint8_t *param = (void *)&msg->param[0];
                        *param = 1;
                        task_msg_insert(msg);
                    }
                    break;
                default:
                    break;
            }
            break;
		case IPC_MSG_FLASH_OPERATION:
			ota_recv_ipc_msg(chn);
			break;
        case IPC_MSG_SET_SBC_CODEC_PARAM:
        	memcpy(&sbc_info,ipc_get_buffer_offset(IPC_DIR_MCU2DSP, chn),sizeof(struct sbc_info_t));
        	break;
        default:
            break;
    }
}

void decoder_prepare_for_next_handler(void *arg)
{
    mp3_decoder_destroy();
    ipc_msg_send(IPC_MSG_WITHOUT_PAYLOAD, IPC_SUB_MSG_DECODER_PREP_READY, NULL);
}

static struct task_msg_handler_t user_msg_handler[] =
{
    // 用于os_timer，用户不可删除
    {OS_TIMER_TRIGGER,			os_timer_trigger_handler},

    // 与具体应用有关，下面的都是用于示例
    {MP3_DECODER_INIT,			mp3_decoder_init},
    {MP3_DECODER_DO_DECODE,     mp3_decoder_do_decoder_handler},
    {MCU_NEED_MORE_SBC_DATA,	sbc_encoder_recv_frame_req},
    {AUDIO_IPC_DMA_RX,       	audio_algorithm_ipc_rx_handler},
    {AUDIO_IPC_DMA_TX, 	 		audio_algorithm_ipc_tx_handler},
    {TEST_FLASH_READ_WRITE,     test_flash_read_write},
    {AUDIO_ALGO_CREATE,         audio_init},
    {AUDIO_ALGO_DESTROY,        audio_algorithm_release},
    {RECEIVE_AT_COMMAND,        app_at_cmd_recv_handler},
    {DECODER_PREPARE_FOR_NEXT,  decoder_prepare_for_next_handler},
    {SBC_DECODER_DO_DECODE,     sbc_decoder_do_decoder_handler},

    {TASK_ID_DEFAULT,           NULL},
};

__attribute__((section("iram_section"))) void flash_reinit(void)
{
	uint32_t flash_id;
	extern const struct qspi_stig_reg_t read_id_cmd;

	GLOBAL_INT_DISABLE();

	qspi_stig_cmd(read_id_cmd, QSPI_STIG_CMD_READ, 3, (uint8_t *)&flash_id);
	flash_id &= 0xffffff;

	// 将spi controller配置成4线的读
    qspi_flash_init_controller(4, 0);
    // 使能flash的4线，参数0表示用0x01 opcode，1表示用0x31 opcode，不同的flash采用不同的opcode，需要看flash的datasheet
    if((flash_id == FLASH_ID_PUYA_64M)
    	|| (flash_id == FLASH_ID_PUYA_32M)
    	|| (flash_id == FLASH_ID_PUYA_128M)
    	|| (flash_id == FLASH_ID_BOYA_128M)
    	|| (flash_id == FLASH_ID_DOSI_64M)
    	|| (flash_id == FLASH_ID_UNKNOWN_128M)
    	|| (flash_id == FLASH_ID_WINBOND_32M)) {
    	qspi_flash_enable_quad(1);
    }
    else if((flash_id == FLASH_ID_XTX_128M)
    	|| (flash_id == FLASH_ID_PUYA_16M)
    	|| (flash_id == FLASH_ID_PUYA_4M)
    	|| (flash_id == FLASH_ID_25Q64JV)
    	|| (flash_id == FLASH_ID_ZB25Q64A)) {
    	qspi_flash_enable_quad(0);
    }
    else if(flash_id == FLASH_ID_NORMEM_64M) {
        qspi_flash_enhance_drive(0x60);
        qspi_flash_enable_quad(2);
    }
    else {
    	printf("WARNING: please confirm opcode used by this flash(ID: %08x) to enable quad operation.\r\n", flash_id);
    	qspi_flash_enable_quad(0);
    }
    // 将写操作之后的自动poll动作关闭
    qspi_poll_set_disable(1);

    // 将分频比配置成div2
	/*
	 * QSPI_BAUDRATE_DIV_2  - 3
	 * QSPI_BAUDRATE_DIV_4  - 0
	 */
#if 1
    qspi_ctrl->delay.sel_dessert = 5;
	qspi_ctrl->read_cap.delay_capture = 3;
	qspi_cfg_set_baudrate(QSPI_BAUDRATE_DIV_2);
#endif
#if 0
	qspi_ctrl->read_cap.delay_capture = 0;
	qspi_cfg_set_baudrate(QSPI_BAUDRATE_DIV_4);
#endif
    GLOBAL_INT_RESTORE();
}

__attribute__((section("iram_section"))) void flash_load_data(uint8_t *dst, uint8_t *src, uint32_t length)
{
    GLOBAL_INT_DISABLE();
    while(qspi_is_busy());
    qspi_ctrl->delay.sel_dessert = 5;
    qspi_ctrl->read_cap.delay_capture = 0;

    if((((uint32_t)dst | (uint32_t)src) & 0x03) == 0) {
        qspi_cfg_set_baudrate(QSPI_BAUDRATE_DIV_4);
        uint32_t *ptr_src = (uint32_t *)src;
        uint32_t *ptr_dst = (uint32_t *)dst;
        uint32_t trans_length = length >> 2;

        for(uint32_t i=0; i<trans_length; i++) {
            *ptr_dst++ = *ptr_src++;
        }
        length -= (trans_length << 2);
        dst = (uint8_t *)ptr_dst;
        src = (uint8_t *)ptr_src;
    }
    else {
        qspi_cfg_set_baudrate(QSPI_BAUDRATE_DIV_10);
    }
    for(uint32_t i=0; i<length; i++) {
        *dst++ = *src++;
    }

    while(qspi_is_busy());
    qspi_ctrl->delay.sel_dessert = 5;
    qspi_ctrl->read_cap.delay_capture = 3;
    qspi_cfg_set_baudrate(QSPI_BAUDRATE_DIV_2);
    GLOBAL_INT_RESTORE();
}

__attribute__((weak)) void uart_receive_char(uint8_t c)
{
    uart_putc_noint(c);
}

__attribute__((section("iram_section"))) void test_flash_read(void)
{
#define TEST_FLASH_READ_LENGTH          2048
    uint8_t *buffer;
    printf("test read flash start\r\n");
    buffer = pvPortMalloc(TEST_FLASH_READ_LENGTH);
    uint32_t start_addr = 0x20000000;
    uint32_t end_addr = start_addr + 140*1024;
    for(; start_addr < end_addr; start_addr += TEST_FLASH_READ_LENGTH) {
        flash_load_data(buffer, (void *)start_addr, TEST_FLASH_READ_LENGTH);
        for(uint32_t i=0; i<TEST_FLASH_READ_LENGTH;) {
            printf("%02x", buffer[i]);
            i++;
            if((i % 16) == 0) {
                printf("\r\n");
            }
        }
    }
    flash_load_data(buffer, (void *)0x20000002, TEST_FLASH_READ_LENGTH);
    vPortFree(buffer);
    printf("test read flash end\r\n");
}

// 用户程序的入口函数
void app_entry(void)
{
    uint8_t channel;

    init_memory();

    printf("enter app entry: BUILD DATE: %s, TIME: %s\r\n", __DATE__, __TIME__);
    printf("user main v1.0.4\r\n");

    flash_reinit();

    // 注册用户自己的IPC接收处理函数
    app_ipc_rx_set_user_handler(ipc_rx_user_handler);
    // 初始化内部的基于while循环的任务列表
    task_init();
    // 初始化os_timer
    os_timer_engine_init();

    uart_init(BAUD_RATE_115200, uart_receive_char);
    _xtos_interrupt_enable(XCHAL_UART_INTERRUPT);

    //os_timer_init(&test_st7796_timer, test_st7796_timer_function, NULL);
    //os_timer_start(&test_st7796_timer, 1000, OS_TIMER_TYPE_REPEAT);

    //os_timer_init(&test_flash_rw_timer, test_flash_rw_timer_function, NULL);
    //os_timer_start(&test_flash_rw_timer, 10000, OS_TIMER_TYPE_REPEAT);

    //os_timer_init(&test_rm69330_timer, test_rm69330_timer_function, NULL);
    //os_timer_start(&test_rm69330_timer, 10, OS_TIMER_TYPE_REPEAT);
    //os_timer_init(&test_rm69330_button_timer, test_rm69330_button_timer_function, NULL);
    //os_timer_start(&test_rm69330_button_timer, 1000, OS_TIMER_TYPE_REPEAT);

    //os_timer_init(&test_gc9c01_timer, test_gc9c01_timer_function, NULL);
    //os_timer_start(&test_gc9c01_timer, 10, OS_TIMER_TYPE_REPEAT);
    //os_timer_init(&test_gc9c01_button_timer, test_gc9c01_button_timer_function, NULL);
    //os_timer_start(&test_gc9c01_button_timer, 1000, OS_TIMER_TYPE_REPEAT);

    // 创建一个新的消息，并推送到消息列表
    //struct task_msg_t *msg = task_msg_alloc(TEST_LCD_RM69330_DISPLAY, 0);
    //task_msg_insert(msg);

//    test_flash_read();

    /* inform MCU that DSP is ready */
    ipc_msg_send(IPC_MSG_WITHOUT_PAYLOAD, IPC_SUM_MSG_DSP_USER_CODE_READY, NULL);

    // 开启任务调度，进入该函数后不会再出来了
    task_schedule(user_msg_handler, sizeof(user_msg_handler)/sizeof(user_msg_handler[0]));
}
