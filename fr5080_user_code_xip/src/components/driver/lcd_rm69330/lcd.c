#include <string.h>
#include <stdio.h>

#include "co_util.h"
#include "flash.h"
#include "plf.h"
#include "qspi.h"
#include "lcd.h"

#define SINGLE_TRANS_SIZE	        0x100	//1024
#define FLASH_LOAD_BUFFER_SIZE      (SINGLE_TRANS_SIZE*2)

#define GPIO_DATA           (SYSTEM_BASE+0)
#define GPIO_DIR            (SYSTEM_BASE+4)

// 用于控制LCD的cs输出高低
#define LCD_CS_HIGH         (*(volatile uint32_t *)GPIO_DATA |= 0x40)
#define LCD_CS_LOW          do {(*(volatile uint32_t *)GPIO_DATA &= 0xbf);} while(0)

// 用于复位LCD
#define LCD_RST_SET         (*(volatile uint32_t *)GPIO_DATA &= 0xdf)
#define LCD_RST_RELEASE     (*(volatile uint32_t *)GPIO_DATA |= 0x20)

// 用于控制LCD的上下电
#define LCD_PWR_ENABLE		(*(volatile uint32_t *)GPIO_DATA |= 0x10)
#define LCD_PWR_DISABLE		(*(volatile uint32_t *)GPIO_DATA &= 0xef)

enum lcd_interface_mode_t {
    LCD_IF_MODE_SPI_3_WIRE,
    LCD_IF_MODE_SPI_4_WIRE,
    LCD_IF_MODE_QSPI,
};

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

void lcd_rm69330_init(void);

// 涉及到qspi controller的使用，为了避免与XIP冲突，这段代码需要放在iram中
__attribute__((section("iram_section"))) void LCDSPI_InitCMD_QSPI(uint8_t cmd, uint8_t *param, uint8_t len)
{
    uint8_t buffer[8];
    
    lcd_cmd.write_bytes = 3 + len;
    buffer[0] = 0x00;
    buffer[1] = cmd;
    buffer[2] = 0x00;
    memcpy(&buffer[3], param, len);
    
    // 取消CS线的硬件控制
    GLOBAL_INT_DISABLE();
    while(qspi_is_busy());
    qspi_ctrl->config.peri_sel_line = 0xf;

    LCD_CS_LOW;
    qspi_stig_cmd(lcd_cmd, QSPI_STIG_CMD_WRITE, 3 + len, buffer);
    LCD_CS_HIGH;

    *(volatile uint32_t *)&qspi_ctrl->cmd_ctrl = 0;

    // 重新将CS线控制交给硬件
    while(qspi_is_busy());
    qspi_ctrl->config.peri_sel_line = 0x0;
    GLOBAL_INT_RESTORE();
}

void LCDSPI_InitCMD_QSPI_single(uint8_t cmd, uint8_t param)
{
    LCDSPI_InitCMD_QSPI(cmd, &param, 1);
}

// 涉及到qspi controller的使用，为了避免与XIP冲突，这段代码需要放在iram中
__attribute__((section("iram_section"))) void lcd_display(uint8_t *data, int32_t length)
{
	uint8_t *align_data;
	uint32_t *buffer, *dst;
	struct qspi_write_ins_reg_t write_conf = {
		.opcode = 0x12,
		.disable_WEL = 1,
		.addr_type = QSPI_WIRE_TYPE_QIO,
		.data_type = QSPI_WIRE_TYPE_QIO,
		.dummy_cycles = 0,
	};
	uint32_t write_conf_org = *(volatile uint32_t *)&qspi_ctrl->write_conf;

	GLOBAL_INT_DISABLE();
    while(qspi_is_busy());
    qspi_cfg_set_enable(0);
    qspi_ctrl->write_conf = write_conf;
    // 取消CS线的硬件控制
    qspi_ctrl->config.peri_sel_line = 0xf;
    qspi_cfg_set_enable(1);

    // 如果数据来源的起始地址不是4字节对齐的，那么先把前面不对齐的数据发送出去
	align_data = (void *)((uint32_t)data & (~0x03));
	if(align_data != data) {
		uint8_t single_length = data - align_data;
		LCD_CS_LOW;
		memcpy((void *)0x20003c00, (void *)data, single_length);
		length -= single_length;
		while(qspi_is_busy());
		LCD_CS_HIGH;
	}

	// 采用256对齐的访问方式，可以直接用uint32_t的方式访问，速度会快
	buffer = (uint32_t *)align_data;
	while(length >= SINGLE_TRANS_SIZE) {
		dst = (uint32_t *)0x20003c00;
		LCD_CS_LOW;
		for(uint32_t j=0; j<(SINGLE_TRANS_SIZE>>2); j++) {
			*dst++ = *buffer++;
		}
		length -= SINGLE_TRANS_SIZE;
		while(qspi_is_busy());
		LCD_CS_HIGH;
	}

	// 把不是256对齐的剩余数据发送出去
	dst = (uint32_t *)0x20003c00;
	LCD_CS_LOW;
	for(uint32_t j=0; j<(length>>2); j++) {
		*dst++ = *buffer++;
	}
	while(qspi_is_busy());
	LCD_CS_HIGH;

	// 把不是4字节对齐的数据发送出去
	if(length & 0x03) {
		LCD_CS_LOW;
		memcpy((void *)0x20003c00, (void *)buffer, length & 0x03);
		while(qspi_is_busy());
		LCD_CS_HIGH;
	}

	while(qspi_is_busy());
	qspi_cfg_set_enable(0);
	*(volatile uint32_t *)&qspi_ctrl->write_conf = write_conf_org;
	// 重新将CS线控制交给硬件
    qspi_ctrl->config.peri_sel_line = 0x0;
    qspi_cfg_set_enable(1);
    GLOBAL_INT_RESTORE();
}

__attribute__((section("iram_section"))) void lcd_set_windows(uint16_t x_s, uint16_t y_s, uint16_t x_e, uint16_t y_e)
{
#define BYTES_PER_PIX		2
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

	LCDSPI_InitCMD_QSPI(0x2A, axis_x, 4);
	LCDSPI_InitCMD_QSPI(0x2B, axis_y, 4);
	LCDSPI_InitCMD_QSPI(0x2c, NULL, 0);
}

void rm69330_init(void)
{
    *(volatile uint32_t *)GPIO_DATA |= 0xf2;
    *(volatile uint32_t *)GPIO_DIR &= 0x0d;

    // power on and reset LCD controller
    LCD_PWR_ENABLE;
    LCD_CS_HIGH;
    LCD_RST_SET;
    co_delay_100us(200);
    LCD_RST_RELEASE;
    co_delay_100us(200);

    lcd_rm69330_init();
    co_delay_100us(200);
    LCDSPI_InitCMD_QSPI(0x2c, NULL, 0);
}

void rm69330_draw_solid_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
	volatile	uint8_t *p_str = NULL;
	uint32_t 	len = width*height*2;
	uint8_t 	buf[1024];
	uint16_t 	i = 0;

	for(i = 0; i < (1024/2); i++)
	{
		buf[2*i+0] = color>>8;
		buf[2*i+1] = color&0xff;
	}

	lcd_set_windows(x,y,width-1,height-1);
	printf("surplus size=%d\r\n",len);
	while(len >= 1024) {
		len -= 1024;
		lcd_display((void *)buf, SINGLE_TRANS_SIZE);
	}

	if(len) {
		lcd_display((void *)buf, len);
		len = 0;
	}

	printf("x=%d y=%d w=%d h=%d color=%x size=%d\r\n",x,y,width,height,color,len);
}


void rm69330_fill_color(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    rm69330_draw_solid_rect(x,y,width,height,color);
}

void lcd_test_screen(void)
{
    while(1)
    {
		rm69330_fill_color(0,0,390,390,COLOR_MAGENTA);
		co_delay_100us(10000);//1s
		rm69330_fill_color(0,0,390,10,COLOR_R);
		co_delay_100us(10000);//1s
		rm69330_fill_color(0,0,390,390,COLOR_G);
		co_delay_100us(10000);//1s
		rm69330_fill_color(0,0,390,390,COLOR_B);
		co_delay_100us(10000);//1s
		rm69330_fill_color(0,0,390,390,COLOR_WHITE);
		co_delay_100us(10000);//1s
		rm69330_fill_color(0,0,390,390,COLOR_BLACK);
		co_delay_100us(10000);//1s
    }
}
