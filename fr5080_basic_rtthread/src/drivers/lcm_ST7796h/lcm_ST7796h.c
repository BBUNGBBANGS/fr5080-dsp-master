/*
 * lcm_st7796h.c
 *
 *  Created on: 2018-7-3
 *      Author: dyh
 */

#include "lcm_ST7796h.h"
#include "qspi.h"
#include "plf.h"
#include "hal_gpio.h"

enum lcd_interface_mode_t {
    LCD_IF_MODE_SPI_3_WIRE,
    LCD_IF_MODE_SPI_4_WIRE,
    LCD_IF_MODE_QSPI,
};

static struct qspi_stig_reg_t flash_single_trans = {
    .enable_bank 		= 0,
    .dummy_cycles 		= 0,
    .write_bytes 		= 0,
    .enable_write 		= 0,
    .addr_bytes 		= 0,
    .enable_mode 		= 0,
    .enable_cmd_addr 	= 0,
    .read_bytes 		= 0,
    .enable_read 		= 0,
    .opcode 			= 0xB9,
};

static struct qspi_stig_reg_t lcd_cmd = {
    .enable_bank 		= 0,
    .dummy_cycles 		= 0,
    .write_bytes 		= 0,
    .enable_write 		= 0,
    .addr_bytes 		= 0,
    .enable_mode 		= 0,
    .enable_cmd_addr 	= 0,
    .read_bytes 		= 0,
    .enable_read 		= 0,
    .opcode 			= 0x02,
};

static struct qspi_stig_reg_t lcd_data = {
    .enable_bank 		= 0,
    .dummy_cycles 		= 0,
    .write_bytes 		= 0,
    .enable_write 		= 1,
    .addr_bytes 		= QSPI_STIG_ADDR_BYTES_1,//设置地址字节长度1字节
    .enable_mode 		= 0,
    .enable_cmd_addr 	= 1,
    .read_bytes 		= 0,
    .enable_read 		= 0,
    .opcode 			= 0x02,
};

void lcm_restart_qspi_init(uint8_t opcode,uint8_t len)
{
	GLOBAL_INT_DISABLE();

    while(qspi_is_busy());

    qspi_write_set_address_lenth(len);
	qspi_write_set_opcode(opcode);

	// 将写数据的格式配置成符合st7796的格式
    qspi_write_set_address_type(QSPI_WIRE_TYPE_STAND);
    qspi_write_set_data_type(QSPI_WIRE_TYPE_STAND);
    qspi_write_set_dummy_cycles(0);
    qspi_write_set_wel_dis(1);
    //qspi_poll_set_disable(1);

    GLOBAL_INT_RESTORE();
}

// 涉及到qspi controller的使用，为了避免与XIP冲突，这段代码需要放在iram中
void LCDSPI_InitCMD(uint8_t *cmd, uint8_t len)
{
    GLOBAL_INT_DISABLE();
    while(qspi_is_busy());
    qspi_cs_control(0);//qspi_ctrl->config.peri_sel_line = 0xf;

    LCD_CS_LOW;
    LCD_DCX_CMD;
    lcd_cmd.opcode = cmd[0];
    qspi_stig_cmd(lcd_cmd, QSPI_STIG_CMD_WRITE, len-1, &cmd[1]);
    LCD_CS_HIGH;

    *(volatile uint32_t *)&qspi_ctrl->cmd_ctrl = 0;

    while(qspi_is_busy());
    qspi_cs_control(1);//qspi_ctrl->config.peri_sel_line = 0x0;
    GLOBAL_INT_RESTORE();
}

void LCDSPI_WRITE(uint8_t *buf, uint32_t len)
{
    GLOBAL_INT_DISABLE();
    while(qspi_is_busy());
    qspi_cs_control(0);//qspi_ctrl->config.peri_sel_line = 0xf;

	LCD_CS_LOW;
	LCD_DCX_DATA;
	lcd_data.opcode = buf[0];
	qspi_set_cmd_addr(buf[1]);
	qspi_stig_cmd(lcd_data, QSPI_STIG_CMD_WRITE, len-2,&buf[2]);
	LCD_CS_HIGH;

    while(qspi_is_busy());
    qspi_cs_control(1);//qspi_ctrl->config.peri_sel_line = 0x0;
    GLOBAL_INT_RESTORE();
}

void LCD_QSPI_WRITE(uint8_t *buffer, uint32_t length)
{
	uint32_t col_address=buffer[1];
	uint8_t val = buffer[0];

	GLOBAL_INT_DISABLE();
	while(qspi_is_busy());
	qspi_cfg_set_enable(0);
	lcm_restart_qspi_init(buffer[0],0x00);
    qspi_ctrl->config.peri_sel_line = 0xf;
    qspi_cfg_set_enable(1);

    LCD_DCX_DATA;
	LCD_CS_LOW;

	length--;
	length--;
	*buffer++;
	*buffer++;

#if 0
	memcpy((uint8_t *)(col_address+QSPI_ADDR), buffer, length);
#else
    while(length != 0) {
        *(uint8_t *)(col_address+QSPI_ADDR) = *buffer++;
        length--;
        col_address++;
    }
#endif
    while(qspi_is_busy());
    LCD_CS_HIGH;

    while(qspi_is_busy());
    qspi_cfg_set_enable(0);
    lcm_restart_qspi_init(0x02,0x02);
    qspi_ctrl->config.peri_sel_line = 0x0;
    qspi_cfg_set_enable(1);
    GLOBAL_INT_RESTORE();
}

//写一个字节的cmd
void LCDSPI_InitCMD_test(uint8_t cmd, uint8_t type)
{
    GLOBAL_INT_DISABLE();
    while(qspi_is_busy());
    qspi_cs_control(0);//qspi_ctrl->config.peri_sel_line = 0xf;

	LCD_CS_LOW;
	if(type == 0)//cmd
		LCD_DCX_CMD;
	else		//data
		LCD_DCX_DATA;

    lcd_cmd.opcode = cmd;
    lcd_cmd.addr_bytes = 0;
    qspi_stig_cmd(lcd_cmd, QSPI_STIG_CMD_EXE,0, 0);
    LCD_CS_HIGH;

    while(qspi_is_busy());
    qspi_cs_control(1);//qspi_ctrl->config.peri_sel_line = 0x0;
    GLOBAL_INT_RESTORE();
}

void LCDSPI_InitDAT(uint8_t data)
{
    GLOBAL_INT_DISABLE();
    while(qspi_is_busy());
    qspi_cs_control(0);//qspi_ctrl->config.peri_sel_line = 0xf;

	LCD_CS_LOW;
    LCD_DCX_DATA;
    lcd_cmd.opcode = data;
    qspi_stig_cmd(lcd_cmd, QSPI_STIG_CMD_WRITE, 0, 0);
    LCD_CS_HIGH;

    while(qspi_is_busy());
    qspi_cs_control(1);//qspi_ctrl->config.peri_sel_line = 0x0;
    GLOBAL_INT_RESTORE();
}

void lcm_set_windows(uint16_t x_s, uint16_t y_s, uint16_t x_e, uint16_t y_e)
{
	#define BYTES_PER_PIX		2
	uint8_t axis_x[4] = {0x00,0x00,0x00,0x00};
	uint8_t axis_y[4] = {0x00,0x00,0x00,0x00};

	axis_x[0] = x_s>>8;
	axis_x[1] = x_s&0xff;
	axis_x[2] = x_e>>8;
	axis_x[3] = x_e&0xff;

	axis_y[0] = (y_s+20)>>8;
	axis_y[1] = (y_s+20)&0xff;
	axis_y[2] = (y_e+20)>>8;
	axis_y[3] = (y_e+20)&0xff;

	LCDSPI_InitCMD_test(0x2A,0);
	LCDSPI_WRITE(axis_x,4);
	LCDSPI_InitCMD_test(0x2B,0);
	LCDSPI_WRITE(axis_y,4);
	LCDSPI_InitCMD_test(0x2c,0);
}

void lcd_gpio_init(void)
{
	hal_gpio_init(GPIO_BIT_0,GPIO_OUT);
	hal_gpio_init(GPIO_BIT_1,GPIO_INPUT);
	hal_gpio_init(GPIO_BIT_2,GPIO_OUT);
	hal_gpio_init(GPIO_BIT_3,GPIO_OUT);
	hal_gpio_init(GPIO_BIT_5,GPIO_OUT);
}

void lcm_reset(void)
{
    LCD_RST_SET;
    co_delay_100us(3500);
    LCD_RST_RELEASE;
    co_delay_100us(200);
}

void st7796h_init(void)
{
	printf("lcm init entry1\r\n");
	lcd_gpio_init();
	printf("lcm init entry2\r\n");
    LCD_LED_RELEASE;
    LCD_CS_HIGH;
    LCD_LED_SET;

    lcm_reset();
    lcm_st7796h_init();
    printf("lcm init back\r\n");

    printf("%08x.\r\n", *(uint32_t *)&qspi_ctrl->write_conf);
}

void st7796h_draw_solid_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
	volatile	uint8_t *p_str = NULL;
	uint32_t 	len = width*height*2;
#define 	FILL_SIZE	200//256//512
	static uint8_t 	buf[FILL_SIZE];

	memset(buf,0,sizeof(buf));
	
	for(uint16_t i = 0; i < (FILL_SIZE/2); i++)	
	{
		buf[2*i+0] = (uint8_t)((color) >>8);
		buf[2*i+1] = (uint8_t)((color) & 0x00ff);
	}

	lcm_set_windows(x,y,x+width-1,y+height-1);
	LCD_DCX_DATA;
	co_delay_10us(10);
	
	GLOBAL_INT_DISABLE();
	while(hal_gpio_read(1) == 0);
	while(len >= (FILL_SIZE)) {
		len -= FILL_SIZE;//FILL_SIZE;
		LCD_QSPI_WRITE(buf,FILL_SIZE);

	}

	if(len) {
		LCD_QSPI_WRITE(buf,len);
		len = 0;
	}
	GLOBAL_INT_RESTORE();
}

void st7796h_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *buffer)
{
    volatile	uint8_t *p_str = NULL;
    uint32_t 	len = width*height*2;
#define     FILL_SIZE	256

    lcm_set_windows(x,y,x+width-1,y+height-1);
    LCD_DCX_DATA;
    co_delay_10us(10);

    //while(hal_gpio_read(1) == 0);
    while(len >= (FILL_SIZE)) {
        LCD_QSPI_WRITE(buffer,FILL_SIZE);
        buffer += FILL_SIZE;
        len -= FILL_SIZE;//FILL_SIZE;
    }

    if(len) {
        LCD_QSPI_WRITE(buffer,len);
        len = 0;
    }
}

void lcd_multi_write(uint8_t *buffer, uint32_t length)
{
	LCD_DCX_DATA;
	co_delay_10us(10);
	while(length)
	{
		if(length >= 512)
		{
			LCD_QSPI_WRITE(buffer,512);
			length	-= 512;
			buffer 	+= 512;
		}
		else
		{
			LCD_QSPI_WRITE(buffer,length);
			length = 0;
		}
	}
}


void st7796h_fill_color(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
	st7796h_draw_solid_rect(x,y,width,height,color);
}

#if 0
void lcd_test_screen(void)
{
#define	LCM_PIX_W	240
#define LCM_PIX_H	240
	st7796h_fill_color(0,0,LCM_PIX_W,LCM_PIX_H,COLOR_BLACK);
	co_delay_100us(50000);
    {
#if 1
		st7796h_fill_color(0,0,LCM_PIX_W,LCM_PIX_H,COLOR_R);
		co_delay_100us(50000);//1s
		st7796h_fill_color(0,0,LCM_PIX_W,LCM_PIX_H,COLOR_G);
		co_delay_100us(50000);//1s
		st7796h_fill_color(0,0,LCM_PIX_W,LCM_PIX_H,COLOR_B);
		co_delay_100us(50000);//1s
#endif
    }
}
#endif

void lcm_st7796h_init(void)
{
#if 1
	LCDSPI_InitCMD_test(0xfe,0);
	LCDSPI_InitCMD_test(0xef,0);

	LCDSPI_InitCMD_test(0x36,0);
	LCDSPI_InitCMD_test(0x48,1);//0x48 bgr 0x40 rgb
	LCDSPI_InitCMD_test(0x3a,0);
	LCDSPI_InitCMD_test(0x55,1);//0x05

	LCDSPI_InitCMD_test(0x85,0);
	LCDSPI_InitCMD_test(0x80,1);
	LCDSPI_InitCMD_test(0x86,0);
	LCDSPI_InitCMD_test(0xf8,1);
	LCDSPI_InitCMD_test(0x87,0);
	LCDSPI_InitCMD_test(0x79,1);
	LCDSPI_InitCMD_test(0x89,0);
	LCDSPI_InitCMD_test(0x13,1);

	LCDSPI_InitCMD_test(0x8b,0);
	LCDSPI_InitCMD_test(0x80,1);

	LCDSPI_InitCMD_test(0x8d,0);
	LCDSPI_InitCMD_test(0x33,1);
	LCDSPI_InitCMD_test(0x8e,0);
	LCDSPI_InitCMD_test(0x8f,1);
	LCDSPI_InitCMD_test(0x8f,0);
	LCDSPI_InitCMD_test(0x73,1);

	//inversion
	LCDSPI_InitCMD_test(0xe8,0);
	LCDSPI_InitCMD_test(0x16,1);//41Hz
	LCDSPI_InitCMD_test(0x00,1);

	LCDSPI_InitCMD_test(0xec,0);
	LCDSPI_InitCMD_test(0x13,1);
	LCDSPI_InitCMD_test(0x02,1);
	LCDSPI_InitCMD_test(0x88,1);

	//cobD??ив?
	LCDSPI_InitCMD_test(0xbf,0);
	LCDSPI_InitCMD_test(0x1c,1);

	LCDSPI_InitCMD_test(0xad,0);
	LCDSPI_InitCMD_test(0x4a,1);

	LCDSPI_InitCMD_test(0xae,0);
	LCDSPI_InitCMD_test(0x44,1);

	LCDSPI_InitCMD_test(0xac,0);
	LCDSPI_InitCMD_test(0x44,1);


	LCDSPI_InitCMD_test(0xc3,0);
	LCDSPI_InitCMD_test(0x58,1);//58

	LCDSPI_InitCMD_test(0xc4,0);
	LCDSPI_InitCMD_test(0x4E,1);//4E

	LCDSPI_InitCMD_test(0xc9,0);
	LCDSPI_InitCMD_test(0x08,1);


	LCDSPI_InitCMD_test(0xff,0);
	LCDSPI_InitCMD_test(0x62,1);


	LCDSPI_InitCMD_test(0x99,0);
	LCDSPI_InitCMD_test(0x3e,1);
	LCDSPI_InitCMD_test(0x9d,0);
	LCDSPI_InitCMD_test(0x4b,1);
	LCDSPI_InitCMD_test(0x98,0);
	LCDSPI_InitCMD_test(0x3e,1);
	LCDSPI_InitCMD_test(0x9c,0);
	LCDSPI_InitCMD_test(0x4b,1);


	LCDSPI_InitCMD_test(0xf0,0);
	LCDSPI_InitCMD_test(0x45,1);
	LCDSPI_InitCMD_test(0x0A,1);
	LCDSPI_InitCMD_test(0x0A,1);
	LCDSPI_InitCMD_test(0x06,1);
	LCDSPI_InitCMD_test(0x05,1);
	LCDSPI_InitCMD_test(0x2E,1);

	LCDSPI_InitCMD_test(0xf2,0);
	LCDSPI_InitCMD_test(0x45,1);
	LCDSPI_InitCMD_test(0x09,1);
	LCDSPI_InitCMD_test(0x0A,1);
	LCDSPI_InitCMD_test(0x0b,1);
	LCDSPI_InitCMD_test(0x05,1);
	LCDSPI_InitCMD_test(0x2E,1);

	LCDSPI_InitCMD_test(0xf1,0);
	LCDSPI_InitCMD_test(0x45,1);
	LCDSPI_InitCMD_test(0x8F,1);
	LCDSPI_InitCMD_test(0x8f,1);
	LCDSPI_InitCMD_test(0x3B,1);
	LCDSPI_InitCMD_test(0x3F,1);
	LCDSPI_InitCMD_test(0x7f,1);

	LCDSPI_InitCMD_test(0xf3,0);
	LCDSPI_InitCMD_test(0x45,1);
	LCDSPI_InitCMD_test(0x8f,1);
	LCDSPI_InitCMD_test(0x8f,1);
	LCDSPI_InitCMD_test(0x3B,1);
	LCDSPI_InitCMD_test(0x3F,1);
	LCDSPI_InitCMD_test(0x7f,1);
	
	LCDSPI_InitCMD_test(0x35,0); //TE
	LCDSPI_InitCMD_test(0x00,1);

	LCDSPI_InitCMD_test(0x11,0);
	co_delay_100us(1200);
	LCDSPI_InitCMD_test(0x29,0);
	LCDSPI_InitCMD_test(0x2c,0);

#else
	LCDSPI_InitCMD_test(0x11,0);

	co_delay_100us(1200);          //ms                         

	LCDSPI_InitCMD_test(0x36,0);
	LCDSPI_InitCMD_test(0x48,1);

	LCDSPI_InitCMD_test(0x3A,0);
	LCDSPI_InitCMD_test(0x05,1);

	LCDSPI_InitCMD_test(0xF0,0);
	LCDSPI_InitCMD_test(0xC3,1);

	LCDSPI_InitCMD_test(0xF0,0);
	LCDSPI_InitCMD_test(0x96,1);

	LCDSPI_InitCMD_test(0xB1,0);	//frame rate
	LCDSPI_InitCMD_test(0x60,1);
	LCDSPI_InitCMD_test(0x1e,1);	//40HZ

	LCDSPI_InitCMD_test(0xB4,0);
	LCDSPI_InitCMD_test(0x01,1);

	LCDSPI_InitCMD_test(0xB7,0);
	LCDSPI_InitCMD_test(0xC6,1);

	LCDSPI_InitCMD_test(0xC0,0);
	LCDSPI_InitCMD_test(0xF0,1);
	LCDSPI_InitCMD_test(0x35,1);

	LCDSPI_InitCMD_test(0xC1,0);
	LCDSPI_InitCMD_test(0x15,1);

	LCDSPI_InitCMD_test(0xC2,0);
	LCDSPI_InitCMD_test(0xAF,1);

	LCDSPI_InitCMD_test(0xC3,0);
	LCDSPI_InitCMD_test(0x09,1);

	LCDSPI_InitCMD_test(0xC5,0);     //VCOM
	LCDSPI_InitCMD_test(0x12,1);

	LCDSPI_InitCMD_test(0xC6,0);
	LCDSPI_InitCMD_test(0x00,1);

	LCDSPI_InitCMD_test(0xE8,0);
	LCDSPI_InitCMD_test(0x40,1);
	LCDSPI_InitCMD_test(0x8A,1);
	LCDSPI_InitCMD_test(0x00,1);
	LCDSPI_InitCMD_test(0x00,1);
	LCDSPI_InitCMD_test(0x29,1);
	LCDSPI_InitCMD_test(0x19,1);
	LCDSPI_InitCMD_test(0xA5,1);
	LCDSPI_InitCMD_test(0x33,1);

	LCDSPI_InitCMD_test(0xE0,0);
	LCDSPI_InitCMD_test(0xD0,1);
	LCDSPI_InitCMD_test(0x07,1);
	LCDSPI_InitCMD_test(0x0C,1);
	LCDSPI_InitCMD_test(0x09,1);
	LCDSPI_InitCMD_test(0x09,1);
	LCDSPI_InitCMD_test(0x15,1);
	LCDSPI_InitCMD_test(0x33,1);
	LCDSPI_InitCMD_test(0x43,1);
	LCDSPI_InitCMD_test(0x4A,1);
	LCDSPI_InitCMD_test(0x36,1);
	LCDSPI_InitCMD_test(0x12,1);
	LCDSPI_InitCMD_test(0x13,1);
	LCDSPI_InitCMD_test(0x2E,1);
	LCDSPI_InitCMD_test(0x33,1);

	LCDSPI_InitCMD_test(0xE1,0);
	LCDSPI_InitCMD_test(0xD0,1);
	LCDSPI_InitCMD_test(0x0B,1);
	LCDSPI_InitCMD_test(0x0F,1);
	LCDSPI_InitCMD_test(0x0D,1);
	LCDSPI_InitCMD_test(0x0C,1);
	LCDSPI_InitCMD_test(0x07,1);
	LCDSPI_InitCMD_test(0x33,1);
	LCDSPI_InitCMD_test(0x33,1);
	LCDSPI_InitCMD_test(0x49,1);
	LCDSPI_InitCMD_test(0x38,1);
	LCDSPI_InitCMD_test(0x14,1);
	LCDSPI_InitCMD_test(0x14,1);
	LCDSPI_InitCMD_test(0x2F,1);
	LCDSPI_InitCMD_test(0x35,1);

	//LCDSPI_InitCMD_test(0x35,0);	//TE
	//LCDSPI_InitCMD_test(0x00,1);

	LCDSPI_InitCMD_test(0x21,0);

	LCDSPI_InitCMD_test(0xF0,0);
	LCDSPI_InitCMD_test(0x3C,1);

	LCDSPI_InitCMD_test(0xF0,0);
	LCDSPI_InitCMD_test(0x69,1);

	co_delay_100us(1200);                

	LCDSPI_InitCMD_test(0x29,0);
#endif

}
