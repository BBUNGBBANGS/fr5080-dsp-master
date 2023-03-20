#ifndef _LCD_H
#define _LCD_H

#include <rtthread.h>
#include <rtdef.h>

struct drv_lcd_dsi_device
{
    struct rt_device parent; /* 设备基类 */
    struct rt_device_graphic_info lcd_info; /* 图形设备属性*/
    struct rt_semaphore lcd_lock; /* 刷屏使用的锁，一帧图像刷完之后才能刷另一帧图像，所以这里需要锁*/
};

int drv_lcd_hw_init(void);

#endif  // _LCD_H
