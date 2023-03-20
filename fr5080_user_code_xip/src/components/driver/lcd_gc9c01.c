/*
 * lcd_gc9c01.c
 *
 *  Created on: 2018-1-14
 *      Author: Administrator
 */

#include <string.h>
#include <stdio.h>

#include "co_util.h"
#include "flash.h"
#include "plf.h"
#include "qspi.h"
#include "hal_gpio.h"

#define LOWER_QSPI_SPEED			1
#define LOWER_SPEED_CFG             QSPI_BAUDRATE_DIV_6
#define ORIGIN_QSPI_SPEED_CFG       QSPI_BAUDRATE_DIV_4

#define SINGLE_TRANS_SIZE	        0x100	//1024
#define FLASH_LOAD_BUFFER_SIZE      (SINGLE_TRANS_SIZE*2)
#define QSPI_ADDR           		0x20000000

#define GPIO_DATA           	(SYSTEM_BASE+0)
#define GPIO_DIR            	(SYSTEM_BASE+4)

//PC5
#define LCD_LED_RELEASE     	(*(volatile uint32_t *)GPIO_DATA &= 0xdf)	//1101 1111
#define LCD_LED_SET             (*(volatile uint32_t *)GPIO_DATA |= 0x20)	//0010 0000

//PB1
//#define LCD_LED_RELEASE     (*(volatile uint32_t *)GPIO_DATA &= 0xfd)   //1111 1101
//#define LCD_LED_SET         (*(volatile uint32_t *)GPIO_DATA |= 0x02)   //0000 0010

//PB3
#define LCD_RST_SET         	(*(volatile uint32_t *)GPIO_DATA &= 0xf7)
#define LCD_RST_RELEASE     	(*(volatile uint32_t *)GPIO_DATA |= 0x08)

//PB0
#define LCD_CS_HIGH         	(*(volatile uint32_t *)GPIO_DATA |= 0x01)	//0000 0001
#define LCD_CS_LOW          	(*(volatile uint32_t *)GPIO_DATA &= 0xfe)	//1111 1110

#define LCD_POWER_ON()				LCD_LED_SET
#define LCD_POWER_OFF()				LCD_LED_RELEASE

#define LCD_RESET_PIN_LOW()			LCD_RST_RELEASE
#define LCD_RESET_PIN_HIGH()		LCD_RST_SET

#define LCD_CS_PIN_LOW()			LCD_CS_LOW
#define LCD_CS_PIN_HIGH()			LCD_CS_HIGH

#define LCD_HEIGHT 					360//120//240
#define LCD_WIDTH 					360//120//240

#define LCD_XDIFF					0
#define LCD_YDIFF					0	//40

#define LCD_DEBUG(fmt,arg...)   printf("[LCD]"fmt"\r\n",##arg)

static struct qspi_stig_reg_t flash_single_trans = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = 0,
    .enable_mode = 0,
    .enable_cmd_addr = 0,
    .read_bytes = 0,
    .enable_read = 0,
    .opcode = 0xB9,
};

static struct qspi_stig_reg_t lcd_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 1,
    .addr_bytes = 0,
    .enable_mode = 0,
    .enable_cmd_addr = 0,
    .read_bytes = 0,
    .enable_read = 0,
    .opcode = 0x02,
};

extern const struct qspi_stig_reg_t read_status_cmd;
extern const struct qspi_stig_reg_t read_status_h_cmd;
extern const struct qspi_stig_reg_t write_enable_cmd;
extern const struct qspi_stig_reg_t write_status_cmd;
extern const struct qspi_stig_reg_t write_disable_cmd;
extern const struct qspi_stig_reg_t read_id_cmd;
extern const struct qspi_stig_reg_t sector_erase_cmd;

static struct qspi_stig_reg_t lcd_read_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = 2,
    .enable_mode = 0,
    .enable_cmd_addr = 1,
    .read_bytes = 0,
    .enable_read = 1,
    .opcode = 0x03,
};

// 涉及到qspi controller的使用，为了避免与XIP冲突，这段代码需要放在iram中
static __attribute__((section("iram_section"))) void LCD_QSPI_inteface_read(uint8_t cmd, uint8_t *buffer, uint8_t len)
{
    uint32_t address = 0;

    if(len > 8) {
        return;
    }

    lcd_read_cmd.read_bytes = len-1;
    address |= (cmd<<8);

    // 取消CS线的硬件控制
    GLOBAL_INT_DISABLE();
    while(qspi_is_busy());
#if LOWER_QSPI_SPEED == 1
    qspi_cfg_set_baudrate(LOWER_SPEED_CFG);
#endif
    qspi_ctrl->config.peri_sel_line = 0xf;

    qspi_set_cmd_addr(address);

    LCD_CS_LOW;
    qspi_stig_cmd(lcd_read_cmd, QSPI_STIG_CMD_READ, len, buffer);
    LCD_CS_HIGH;

    *(volatile uint32_t *)&qspi_ctrl->cmd_ctrl = 0;

    // 重新将CS线控制交给硬件
    while(qspi_is_busy());
#if LOWER_QSPI_SPEED == 1
    qspi_cfg_set_baudrate(ORIGIN_QSPI_SPEED_CFG);
#endif
    qspi_ctrl->config.peri_sel_line = 0x0;
    GLOBAL_INT_RESTORE();
}

// 涉及到qspi controller的使用，为了避免与XIP冲突，这段代码需要放在iram中
static __attribute__((section("iram_section"))) void LCD_QSPI_inteface(uint8_t cmd, uint8_t *param, uint8_t len)
{
    uint8_t buffer[35];

    lcd_cmd.write_bytes = 3 + len;
    buffer[0] = 0x00;
    buffer[1] = cmd;
    buffer[2] = 0x00;
    memcpy(&buffer[3], param, len);

    // 取消CS线的硬件控制
    GLOBAL_INT_DISABLE();
    while(qspi_is_busy());
#if LOWER_QSPI_SPEED == 1
    qspi_cfg_set_baudrate(LOWER_SPEED_CFG);
#endif
    qspi_ctrl->config.peri_sel_line = 0xf;

    LCD_CS_LOW;
    qspi_stig_cmd(lcd_cmd, QSPI_STIG_CMD_WRITE, 3 + len, buffer);
    LCD_CS_HIGH;

    *(volatile uint32_t *)&qspi_ctrl->cmd_ctrl = 0;

    // 重新将CS线控制交给硬件
    while(qspi_is_busy());
#if LOWER_QSPI_SPEED == 1
    qspi_cfg_set_baudrate(ORIGIN_QSPI_SPEED_CFG);
#endif
    qspi_ctrl->config.peri_sel_line = 0x0;
    GLOBAL_INT_RESTORE();
}

static void LCD_QSPI_single(uint8_t cmd, uint8_t param)
{
    LCD_QSPI_inteface(cmd, &param, 1);
}

static __attribute__((section("iram_section"))) void rtl_gc9c01_qspi_write_buff(uint8_t *buffer, uint32_t length)
{
	uint32_t col_address=buffer[1];
	uint8_t *ptr;
	struct qspi_write_ins_reg_t write_conf = {
		.opcode = 0x02,
		.disable_WEL = 1,
		.addr_type = QSPI_WIRE_TYPE_STAND,
		.data_type = QSPI_WIRE_TYPE_STAND,
		.dummy_cycles = 0,
	};
	struct qspi_device_size_cfg_t size_conf = {
		.addr_bytes = 0,
		.page_bytes = 0x400,
		.block_bytes = 0x10,
		.CS0_size = 0,
		.CS1_size = 0,
		.CS2_size = 0,
		.CS3_size = 0,
	};
	uint32_t write_conf_org = *(volatile uint32_t *)&qspi_ctrl->write_conf;
	uint32_t size_conf_org = *(volatile uint32_t *)&qspi_ctrl->size_cfg;

	GLOBAL_INT_DISABLE();
	while(qspi_is_busy());
#if LOWER_QSPI_SPEED == 1
    qspi_cfg_set_baudrate(LOWER_SPEED_CFG);
#endif
	qspi_cfg_set_enable(0);
	write_conf.opcode = buffer[0];
	qspi_ctrl->write_conf = write_conf;
	qspi_ctrl->size_cfg = size_conf;
    qspi_ctrl->config.peri_sel_line = 0xf;
    qspi_cfg_set_enable(1);

	LCD_CS_LOW;

	length -= 2;
	buffer += 2;

	ptr = (uint8_t *)(col_address+QSPI_ADDR);
    while(length-- != 0) {
        *ptr++ = *buffer++;
    }
    while(qspi_is_busy());
    LCD_CS_HIGH;

    while(qspi_is_busy());
#if LOWER_QSPI_SPEED == 1
    qspi_cfg_set_baudrate(ORIGIN_QSPI_SPEED_CFG);
#endif
    qspi_cfg_set_enable(0);
    *(volatile uint32_t *)&qspi_ctrl->write_conf = write_conf_org;
    *(volatile uint32_t *)&qspi_ctrl->size_cfg = size_conf_org;
    qspi_ctrl->config.peri_sel_line = 0x0;
    qspi_cfg_set_enable(1);

    GLOBAL_INT_RESTORE();
}

static __attribute__((section("iram_section"))) void rtl_gc9c01_qspi_write_cmd(uint8_t cmd)
{
    LCD_QSPI_inteface(cmd, NULL, 0);
}

static __attribute__((section("iram_section"))) void rtl_gc9c01_qspi_cmd_param(uint8_t cmd, uint8_t param)
{
    LCD_QSPI_inteface(cmd, &param, 1);
}

#if 0
// 涉及到qspi controller的使用，为了避免与XIP冲突，这段代码需要放在iram中
__attribute__((section("iram_section"))) void lcd_gc9c01_display(uint8_t *data, int32_t length)
{
#define GC9C01_BUFFER_ADDRESS		0x20003c00
    uint8_t *align_data;
    uint32_t *buffer, *dst;
    struct qspi_write_ins_reg_t write_conf = {
        .opcode = 0x32,
        .disable_WEL = 1,
        .addr_type = QSPI_WIRE_TYPE_STAND,
        .data_type = QSPI_WIRE_TYPE_QIO,
        .dummy_cycles = 0,
    };
    uint32_t write_conf_org = *(volatile uint32_t *)&qspi_ctrl->write_conf;

    GLOBAL_INT_DISABLE();
    while(qspi_is_busy());
#if LOWER_QSPI_SPEED == 1
    qspi_cfg_set_baudrate(LOWER_SPEED_CFG);
#endif
    qspi_cfg_set_enable(0);
    qspi_ctrl->write_conf = write_conf;
    // 取消CS线的硬件控制
    qspi_ctrl->config.peri_sel_line = 0xf;
    qspi_cfg_set_enable(1);

    // 如果数据来源的起始地址不是4字节对齐的，那么先把前面不对齐的数据发送出去
    align_data = (void *)((uint32_t)data & (~0x03));
    if(align_data != data) {
        uint8_t single_length = 4 - ((uint32_t)data) & 0x03;
        LCD_CS_LOW;
        memcpy((void *)GC9C01_BUFFER_ADDRESS, (void *)data, single_length);
        length -= single_length;
        while(qspi_is_busy());
        LCD_CS_HIGH;

        buffer = (uint32_t *)(data + single_length);
    }
    else {
        buffer = (uint32_t *)align_data;
    }

    // 采用256对齐的访问方式，可以直接用uint32_t的方式访问，速度会快
    while(length >= SINGLE_TRANS_SIZE) {
        dst = (uint32_t *)GC9C01_BUFFER_ADDRESS;
        LCD_CS_LOW;
        for(uint32_t j=0; j<(SINGLE_TRANS_SIZE>>2); j++) {
            *dst++ = *buffer++;
        }
        length -= SINGLE_TRANS_SIZE;
        while(qspi_is_busy());
        LCD_CS_HIGH;
        //co_delay_10us(1);
        volatile uint32_t dely_count = 0;
        for(uint32_t x = 0; x < 10;x++)
        {
            dely_count++;
        }
    }

    // 把不是256对齐的剩余数据发送出去
    dst = (uint32_t *)GC9C01_BUFFER_ADDRESS;
    if(length>=4) {
        LCD_CS_LOW;
        for(uint32_t j=0; j<(length>>2); j++) {
            *dst++ = *buffer++;
        }
        while(qspi_is_busy());
        LCD_CS_HIGH;
    }

    // 把不是4字节对齐的数据发送出去
    if(length & 0x03) {
        LCD_CS_LOW;
        memcpy((void *)GC9C01_BUFFER_ADDRESS, (void *)buffer, length & 0x03);
        while(qspi_is_busy());
        LCD_CS_HIGH;
    }

    while(qspi_is_busy());
#if LOWER_QSPI_SPEED == 1
    qspi_cfg_set_baudrate(ORIGIN_QSPI_SPEED_CFG);
#endif
    qspi_cfg_set_enable(0);
    *(volatile uint32_t *)&qspi_ctrl->write_conf = write_conf_org;
    // 重新将CS线控制交给硬件
    qspi_ctrl->config.peri_sel_line = 0x0;
    qspi_cfg_set_enable(1);
    GLOBAL_INT_RESTORE();
}
#else
// 涉及到qspi controller的使用，为了避免与XIP冲突，这段代码需要放在iram中
__attribute__((section("iram_section"))) void lcd_gc9c01_display(uint8_t *data, int32_t length)
{
#define GC9C01_BUFFER_ADDRESS       0x20003c00
    uint8_t *align_data;
    uint32_t *buffer, *dst;
    struct qspi_write_ins_reg_t write_conf = {
        .opcode = 0x32,
        .disable_WEL = 1,
        .addr_type = QSPI_WIRE_TYPE_STAND,
        .data_type = QSPI_WIRE_TYPE_QIO,
        .dummy_cycles = 0,
    };
    struct qspi_device_size_cfg_t size_conf = {
        .addr_bytes = 2,
        .page_bytes = 0x800,
        .block_bytes = 0x10,
        .CS0_size = 0,
        .CS1_size = 0,
        .CS2_size = 0,
        .CS3_size = 0,
    };
    uint32_t size_conf_org = *(volatile uint32_t *)&qspi_ctrl->size_cfg;
    uint32_t write_conf_org = *(volatile uint32_t *)&qspi_ctrl->write_conf;

    GLOBAL_INT_DISABLE();
    while(qspi_is_busy());
#if LOWER_QSPI_SPEED == 1
    qspi_cfg_set_baudrate(LOWER_SPEED_CFG);
#endif
    qspi_cfg_set_enable(0);
    qspi_ctrl->write_conf = write_conf;
    qspi_ctrl->size_cfg = size_conf;
    // 取消CS线的硬件控制
    qspi_ctrl->config.peri_sel_line = 0xf;
    qspi_cfg_set_enable(1);

    while(length >= (0x400+0x16)) {
        uint16_t single_length = 0x400;
        LCD_CS_LOW;
//        memcpy((void *)GC9C01_BUFFER_ADDRESS, (void *)data, single_length);
        uint8_t *dst = (void *)GC9C01_BUFFER_ADDRESS;
        uint8_t *src = (void *)data;
        for(uint16_t i=0; i<single_length; i++) {
            *dst++ = *src++;
        }
        length -= single_length;
        data += single_length;
        while(qspi_is_busy());
        LCD_CS_HIGH;

        volatile uint32_t dely_count = 0;
        for(uint32_t x = 0; x < 10;x++)
        {
            dely_count++;
        }
    }

    if(length) {
        LCD_CS_LOW;
//        memcpy((void *)GC9C01_BUFFER_ADDRESS, (void *)data, length);
        uint8_t *dst = (void *)GC9C01_BUFFER_ADDRESS;
        uint8_t *src = (void *)data;
        for(uint16_t i=0; i<length; i++) {
            *dst++ = *src++;
        }
        while(qspi_is_busy());
        LCD_CS_HIGH;
    }

    while(qspi_is_busy());
#if LOWER_QSPI_SPEED == 1
    qspi_cfg_set_baudrate(ORIGIN_QSPI_SPEED_CFG);
#endif
    qspi_cfg_set_enable(0);
    *(volatile uint32_t *)&qspi_ctrl->write_conf = write_conf_org;
    *(volatile uint32_t *)&qspi_ctrl->size_cfg = size_conf_org;
    // 重新将CS线控制交给硬件
    qspi_ctrl->config.peri_sel_line = 0x0;
    qspi_cfg_set_enable(1);
    GLOBAL_INT_RESTORE();
}
#endif

__attribute__((section("iram_section"))) void lcd_gc9c01_set_windows(uint16_t x_s, uint16_t y_s, uint16_t x_e, uint16_t y_e)
{
    uint8_t axis_x[4] = {0x00,0x00,0x00,0x00};
    uint8_t axis_y[4] = {0x00,0x00,0x00,0x00};

    axis_x[0] = x_s>>8;
    axis_x[1] = x_s&0xff;
    axis_x[2] = x_e>>8;
    axis_x[3] = x_e&0xff;

    axis_y[0] = y_s>>8;
    axis_y[1] = y_s&0xff;
    axis_y[2] = y_e>>8;
    axis_y[3] = y_e&0xff;

    LCD_QSPI_inteface(0x2A, axis_x, 4);
    LCD_QSPI_inteface(0x2B, axis_y, 4);
    LCD_QSPI_inteface(0x2c, NULL, 0);
}

#if 1
#define COLOR_WHITE			0xffff
#define COLOR_BLACK			0x0000
#define COLOR_R				0xF800
#define COLOR_G				0x07E0
#define COLOR_B				0x001F
#define COLOR_MAGENTA     	0xF81F
void am_fill_color(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    am_draw_solid_rect(x,y,width,height,color);
}

void am_test_screen(void)
{
    while(1)
    {
		am_fill_color(0,0,360,360,COLOR_MAGENTA);
		co_delay_100us(10000);//1s
		am_fill_color(0,0,360,360,COLOR_R);
		co_delay_100us(10000);//1s
		am_fill_color(0,0,360,360,COLOR_G);
		co_delay_100us(10000);//1s
		am_fill_color(0,0,360,360,COLOR_B);
		co_delay_100us(10000);//1s
		am_fill_color(0,0,360,360,COLOR_WHITE);
		co_delay_100us(10000);//1s
		am_fill_color(0,0,360,360,COLOR_BLACK);
		co_delay_100us(10000);//1s
    }
}
#endif

static void lcd_gpio_init(void)
{
	hal_gpio_init(GPIO_BIT_0,GPIO_OUT);
	hal_gpio_init(GPIO_BIT_1,GPIO_INPUT);
	hal_gpio_init(GPIO_BIT_3,GPIO_OUT);
	hal_gpio_init(GPIO_BIT_5,GPIO_OUT);
}

static void lcm_reset(void)
{
    LCD_RST_RELEASE;
    co_delay_100us(1200);
    LCD_RST_SET;
    co_delay_100us(3500);
    LCD_RST_RELEASE;
    co_delay_100us(1200);
}

static void lcm_gc9c01_init(void)
{
    uint8_t dat[45] = {0x02, 0x00};

    rtl_gc9c01_qspi_write_cmd(0xfe);
    rtl_gc9c01_qspi_write_cmd(0xef);// internal reg enable
    rtl_gc9c01_qspi_cmd_param(0x80, 0x11);
    rtl_gc9c01_qspi_cmd_param(0x81, 0x70);//reg_en for 7C\7D\7E
    rtl_gc9c01_qspi_cmd_param(0x82, 0x09);//reg_en for 90\93
    rtl_gc9c01_qspi_cmd_param(0x83, 0x03);//reg_en for 98\99
    rtl_gc9c01_qspi_cmd_param(0x84, 0x20);//reg_en for B5
    rtl_gc9c01_qspi_cmd_param(0x85, 0x42);//reg_en for B9\BE
    rtl_gc9c01_qspi_cmd_param(0x86, 0xfc);//reg_en for C2~7
    rtl_gc9c01_qspi_cmd_param(0x87, 0x09);//reg_en for C8\CB
    rtl_gc9c01_qspi_cmd_param(0x89, 0x10);//reg_en for EC
    rtl_gc9c01_qspi_cmd_param(0x8A, 0x4f);//reg_en for F0~3\F6
    rtl_gc9c01_qspi_cmd_param(0x8C, 0x59);//reg_en for 60\63\64\66
    rtl_gc9c01_qspi_cmd_param(0x8D, 0x51);//reg_en for 68\6C\6E
    rtl_gc9c01_qspi_cmd_param(0x8E, 0xae);//reg_en for A1~3\A5\A7
    rtl_gc9c01_qspi_cmd_param(0x8F, 0xf3);//reg_en for AC~F\A8\A9

    rtl_gc9c01_qspi_cmd_param(0x36, 0x00);
    rtl_gc9c01_qspi_cmd_param(0x3a, 0x05);// 565 frame
    rtl_gc9c01_qspi_cmd_param(0xEC, 0x77);//2COL

    dat[2] = 0x74;
    dat[3] = 0x00;
    dat[4] = 0x01;
    dat[5] = 0x80;
    dat[6] = 0x00;
    dat[7] = 0x00;
    dat[8] = 0x00;
    dat[9] = 0x00;
    rtl_gc9c01_qspi_write_buff(dat, 10);//rtn 60Hz

    rtl_gc9c01_qspi_cmd_param(0x98, 0x3E);//bvdd 3x
    rtl_gc9c01_qspi_cmd_param(0x99, 0x3E);//bvee -2x

    rtl_gc9c01_qspi_cmd_param(0xC3, 0x2A);//VBP
    rtl_gc9c01_qspi_cmd_param(0xC4, 0x18);//VBN

    dat[2] = 0xA1;
    dat[3] = 0x00;
    dat[4] = 0x01;
    dat[5] = 0x04;
    rtl_gc9c01_qspi_write_buff(dat, 6);

    dat[2] = 0xA2;
    dat[3] = 0x00;
    dat[4] = 0x01;
    dat[5] = 0x04;
    rtl_gc9c01_qspi_write_buff(dat, 6);

    rtl_gc9c01_qspi_cmd_param(0xA9, 0x1C);//IREF

    dat[2] = 0xA5;
    dat[3] = 0x00;
    dat[4] = 0x11;//VDDMA
    dat[5] = 0x09;//VDDML
    rtl_gc9c01_qspi_write_buff(dat, 6);

    rtl_gc9c01_qspi_cmd_param(0xB9, 0x8A);//RTERM
    rtl_gc9c01_qspi_cmd_param(0xA8, 0x5E);//VBG_BUF, DVDD
    rtl_gc9c01_qspi_cmd_param(0xA7, 0x40);
    rtl_gc9c01_qspi_cmd_param(0xAF, 0x73);//VDDSOU ,VDDGM
    rtl_gc9c01_qspi_cmd_param(0xAE, 0x44);//VREE,VRDD
    rtl_gc9c01_qspi_cmd_param(0xAD, 0x38);//VRGL ,VDDSF
    rtl_gc9c01_qspi_cmd_param(0xA3, 0x5D);
    rtl_gc9c01_qspi_cmd_param(0xC2, 0x02);//VREG_VREF
    rtl_gc9c01_qspi_cmd_param(0xC5, 0x11);//VREG1A
    rtl_gc9c01_qspi_cmd_param(0xC6, 0x0E);//VREG1B
    rtl_gc9c01_qspi_cmd_param(0xC7, 0x13);//VREG2A
    rtl_gc9c01_qspi_cmd_param(0xC8, 0x0D);//VREG2B

    rtl_gc9c01_qspi_cmd_param(0xCB, 0x02);//bvdd ref_ad

    dat[2] = 0x7C;
    dat[3] = 0x00;
    dat[4] = 0xB6;
    dat[5] = 0x26;
    rtl_gc9c01_qspi_write_buff(dat, 6);

    rtl_gc9c01_qspi_cmd_param(0xAC, 0x24);//VGLO

    rtl_gc9c01_qspi_cmd_param(0xF6, 0x80);//EPF=2
    //*********************校准结束*************************//
    //gip start
    dat[2] = 0xB5;
    dat[3] = 0x00;
    dat[4] = 0x09;//VFP
    dat[5] = 0x09;//VBP
    rtl_gc9c01_qspi_write_buff(dat, 6);

    dat[2] = 0x60;
    dat[3] = 0x00;
    dat[4] = 0x38;
    dat[5] = 0x0B;
    dat[6] = 0x5B;
    dat[7] = 0x56;
    rtl_gc9c01_qspi_write_buff(dat, 8);//STV1&2

    dat[2] = 0x63;
    dat[3] = 0x00;
    dat[4] = 0x3A;
    dat[5] = 0xE0;//DE
    dat[6] = 0x5B;//MAX=0x61
    dat[7] = 0x56;//MAX=0x61
    rtl_gc9c01_qspi_write_buff(dat, 8);

    dat[2] = 0x64;
    dat[3] = 0x00;
    dat[4] = 0x38;
    dat[5] = 0x0D;
    dat[6] = 0x72;
    dat[7] = 0xDD;
    dat[8] = 0x5B;
    dat[9] = 0x56;
    rtl_gc9c01_qspi_write_buff(dat, 10);//CLK_group1

    dat[2] = 0x66;
    dat[3] = 0x00;
    dat[4] = 0x38;
    dat[5] = 0x11;
    dat[6] = 0x72;
    dat[7] = 0xE1;
    dat[8] = 0x5B;
    dat[9] = 0x56;
    rtl_gc9c01_qspi_write_buff(dat, 10);//CLK_group1

    dat[2] = 0x68;
    dat[3] = 0x00;
    dat[4] = 0x3B;//FLC12 FREQ
    dat[5] = 0x08;
    dat[6] = 0x08;
    dat[7] = 0x00;
    dat[8] = 0x08;
    dat[9] = 0x29;
    dat[10] = 0x5B;
    rtl_gc9c01_qspi_write_buff(dat, 11);//FLC&FLV 1~2

    dat[2] = 0x6E;
    dat[3] = 0x00;
    dat[4] = 0x00;
    dat[5] = 0x00;
    dat[6] = 0x00;
    dat[7] = 0x07;
    dat[8] = 0x01;
    dat[9] = 0x13;
    dat[10] = 0x11;
    dat[11] = 0x0B;
    dat[12] = 0x09;
    dat[13] = 0x16;
    dat[14] = 0x15;
    dat[15] = 0x1D;
    dat[16] = 0x1E;
    dat[17] = 0x00;
    dat[18] = 0x00;
    dat[19] = 0x00;
    dat[20] = 0x00;
    dat[21] = 0x00;
    dat[22] = 0x00;
    dat[23] = 0x1E;
    dat[24] = 0x1D;
    dat[25] = 0x15;
    dat[26] = 0x16;
    dat[27] = 0x0A;
    dat[28] = 0x0C;
    dat[29] = 0x12;
    dat[30] = 0x14;
    dat[31] = 0x02;
    dat[32] = 0x08;
    dat[33] = 0x00;
    dat[34] = 0x00;
    dat[35] = 0x00;
    rtl_gc9c01_qspi_write_buff(dat, 36);//gip end

    rtl_gc9c01_qspi_cmd_param(0xBE, 0x11);//SOU_BIAS_FIX

    dat[2] = 0x6C;
    dat[3] = 0x00;
    dat[4] = 0xCC;
    dat[5] = 0x0C;
    dat[6] = 0xCC;
    dat[7] = 0x84;
    dat[8] = 0xCC;
    dat[9] = 0x04;
    dat[10] = 0x50;
    rtl_gc9c01_qspi_write_buff(dat, 11);//precharge GATE

    rtl_gc9c01_qspi_cmd_param(0x7D, 0x72);
    rtl_gc9c01_qspi_cmd_param(0x7E, 0x38);

    dat[2] = 0x70;
    dat[3] = 0x00;
    dat[4] = 0x02;
    dat[5] = 0x03;
    dat[6] = 0x09;
    dat[7] = 0x05;
    dat[8] = 0x0C;
    dat[9] = 0x06;
    dat[10] = 0x09;
    dat[11] = 0x05;
    dat[12] = 0x0C;
    dat[13] = 0x06;
    rtl_gc9c01_qspi_write_buff(dat, 14);

    dat[2] = 0x90;
    dat[3] = 0x00;
    dat[4] = 0x06;
    dat[5] = 0x06;
    dat[6] = 0x05;
    dat[7] = 0x06;
    rtl_gc9c01_qspi_write_buff(dat, 8);

    dat[2] = 0x93;
    dat[3] = 0x00;
    dat[4] = 0x45;
    dat[5] = 0xFF;
    dat[6] = 0x00;
    rtl_gc9c01_qspi_write_buff(dat, 7);

    dat[2] = 0xF0;
    dat[3] = 0x00;
    dat[4] = 0x45;
    dat[5] = 0x09;
    dat[6] = 0x08;
    dat[7] = 0x08;
    dat[8] = 0x26;
    dat[9] = 0x2A;
    rtl_gc9c01_qspi_write_buff(dat, 10);//gamma start

    dat[2] = 0xF1;
    dat[3] = 0x00;
    dat[4] = 0x43;
    dat[5] = 0x70;
    dat[6] = 0x72;
    dat[7] = 0x36;
    dat[8] = 0x37;
    dat[9] = 0x6F;
    rtl_gc9c01_qspi_write_buff(dat, 10);

    dat[2] = 0xF2;
    dat[3] = 0x00;
    dat[4] = 0x45;
    dat[5] = 0x09;
    dat[6] = 0x08;
    dat[7] = 0x08;
    dat[8] = 0x26;
    dat[9] = 0x2A;
    rtl_gc9c01_qspi_write_buff(dat, 10);

    dat[2] = 0xF3;
    dat[3] = 0x00;
    dat[4] = 0x43;
    dat[5] = 0x70;
    dat[6] = 0x72;
    dat[7] = 0x36;
    dat[8] = 0x37;
    dat[9] = 0x6F;
    rtl_gc9c01_qspi_write_buff(dat, 10);//gamma end

    //rtl_gc9c01_qspi_cmd_param(0x35, 0x00);

    rtl_gc9c01_qspi_write_cmd(0xfe);

    rtl_gc9c01_qspi_write_cmd(0xee);

    rtl_gc9c01_qspi_write_cmd(0x11);
    co_delay_100us(120);
    rtl_gc9c01_qspi_write_cmd(0x29);
//    rtl_gc9c01_qspi_write_cmd(0x2C);

	LCD_POWER_ON();
}

// LCD 屏初始化
void lcd_gc9c01_init(void)
{
	LCD_DEBUG("lcm init entry1\r\n");
	lcd_gpio_init();
	LCD_DEBUG("lcm init entry2\r\n");
    LCD_LED_RELEASE;
    LCD_CS_HIGH;
    LCD_LED_SET;

    lcm_reset();
    lcm_gc9c01_init();
    LCD_DEBUG("lcm init back\r\n");
}
