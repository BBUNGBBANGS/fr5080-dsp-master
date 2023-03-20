#ifndef __LCM_ST7796H_H
#define __LCM_ST7796H_H


#include <string.h>
#include <stdio.h>
#include "co_util.h"
#include "flash.h"
#include "plf.h"
#include "qspi.h"

#if 1

#define SINGLE_TRANS_SIZE	        0x400	//1024
#define FLASH_LOAD_BUFFER_SIZE      (SINGLE_TRANS_SIZE*2)
#define QSPI_ADDR           		0x20000000

#define GPIO_DATA           (SYSTEM_BASE+0)
#define GPIO_DIR            (SYSTEM_BASE+4)

//PB2
#define LCD_DCX_DATA        (*(volatile uint32_t *)GPIO_DATA |= 0x04)	//0000 0100
#define LCD_DCX_CMD         (*(volatile uint32_t *)GPIO_DATA &= 0xfb)	//1111 1011

//PC5
#define LCD_LED_RELEASE     (*(volatile uint32_t *)GPIO_DATA &= 0xdf)	//1101 1111
#define LCD_LED_SET     	(*(volatile uint32_t *)GPIO_DATA |= 0x20)	//0010 0000

//PB3															
#define LCD_RST_SET         (*(volatile uint32_t *)GPIO_DATA &= 0xf7)//(*(volatile uint32_t *)GPIO_DATA &= 0xdf)
#define LCD_RST_RELEASE     (*(volatile uint32_t *)GPIO_DATA |= 0x08)//(*(volatile uint32_t *)GPIO_DATA |= 0x20)

//PB0
#define LCD_CS_HIGH         (*(volatile uint32_t *)GPIO_DATA |= 0x01)	//0000 0001
#define LCD_CS_LOW          (*(volatile uint32_t *)GPIO_DATA &= 0xfe)	//1111 1110

#define LCD_PWR_ENABLE		(*(volatile uint32_t *)GPIO_DATA |= 0x10)
#define LCD_PWR_DISABLE		(*(volatile uint32_t *)GPIO_DATA &= 0xef)


#define	COLOR_DEPTH			16

#define	LCM_PIX_WIDTH		240
#define LCM_PIX_HEIGHT		240

#define COLOR_PIXEL_SIZE	(COLOR_DEPTH/8)

#define COLOR_WHITE			0xffff
#define COLOR_BLACK			0x0000
#define COLOR_R				0xF800
#define COLOR_G				0x07E0
#define COLOR_B				0x001F
#define COLOR_MAGENTA     	0xF81F


extern void lcm_st7796h_init(void);
extern void qspi_driver_init(uint8_t opcode);
extern void lcd_display(uint8_t *data, int32_t length);
extern void lcd_test_screen(void);
extern void lcd_st7796h_init(void);
extern void	lcm_set_windows(uint16_t x_s, uint16_t y_s, uint16_t x_e, uint16_t y_e);
extern void	st7796h_init(void);
extern void	st7796h_fill_color(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
extern void	st7796h_dsp_pic(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *p_data);
//extern void LCD_QSPI_WRITE(uint8_t *buffer, uint32_t length);
extern void lcd_multi_write(uint8_t *buffer, uint32_t length);
#endif  /*	__LCM_ST7796H_H	*/
#endif





