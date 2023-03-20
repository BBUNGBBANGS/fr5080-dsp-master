#ifndef _LCD_H
#define _LCD_H

//#define SINGLE_TRANS_SIZE	        0x400	//1024
//#define FLASH_LOAD_BUFFER_SIZE      (SINGLE_TRANS_SIZE*2)
//
//#define GPIO_DATA           (SYSTEM_BASE+0)
//#define GPIO_DIR            (SYSTEM_BASE+4)
//
//#define LCD_DCX_DATA        (*(volatile uint32_t *)GPIO_DATA |= 0x10)
//#define LCD_DCX_CMD         (*(volatile uint32_t *)GPIO_DATA &= 0xef)
//
//#define LCD_CS_HIGH         (*(volatile uint32_t *)GPIO_DATA |= 0x40)
//#define LCD_CS_LOW          do {(*(volatile uint32_t *)GPIO_DATA &= 0xbf); qspi_cfg_set_baudrate(QSPI_BAUDRATE_DIV_10);} while(0)
//
//#define LCD_RST_SET         (*(volatile uint32_t *)GPIO_DATA &= 0xdf)
//#define LCD_RST_RELEASE     (*(volatile uint32_t *)GPIO_DATA |= 0x20)
//
//#define LCD_PWR_ENABLE		(*(volatile uint32_t *)GPIO_DATA |= 0x10)
//#define LCD_PWR_DISABLE		(*(volatile uint32_t *)GPIO_DATA &= 0xef)
//
//#define FLASH_CS_HIGH      	(*(volatile uint32_t *)GPIO_DATA |= 0x02)
//#define FLASH_CS_LOW       	do {(*(volatile uint32_t *)GPIO_DATA &= 0xfd); qspi_cfg_set_baudrate(QSPI_BAUDRATE_DIV_4);} while(0)
//#define FLASH_RW_CS_LOW     do {(*(volatile uint32_t *)GPIO_DATA &= 0xfd); qspi_cfg_set_baudrate(QSPI_BAUDRATE_DIV_4);} while(0)



#define	COLOR_DEPTH			16

#define	LCM_PIX_WIDTH		390
#define LCM_PIX_HEIGHT		390

#define COLOR_PIXEL_SIZE	(COLOR_DEPTH/8)

#define COLOR_WHITE			0xffff
#define COLOR_BLACK			0x0000
#define COLOR_R				0xF800
#define COLOR_G				0x07E0
#define COLOR_B				0x001F
#define COLOR_MAGENTA     	0xF81F


void lcd_init(void);
void lcd_test(void);

void lcd_display(uint8_t *data, int32_t length);
void lcd_test_screen(void);
extern void	lcd_set_windows(uint16_t x_s, uint16_t y_s, uint16_t x_e, uint16_t y_e);
extern void	rm69330_init(void);
extern void	rm69330_fill_color(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
extern void	rm69330_dsp_pic(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *p_data);
//void	lcm_set_windows(uint16_t x_s, uint16_t y_s, uint16_t x_e, uint16_t y_e);
//void	rm69330_init(void);

#endif  // _LCD_H

