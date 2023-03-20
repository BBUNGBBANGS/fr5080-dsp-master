#ifndef _LCD_H
#define _LCD_H

#include <rtthread.h>
#include <rtdef.h>

struct drv_lcd_dsi_device
{
    struct rt_device parent; /* �豸���� */
    struct rt_device_graphic_info lcd_info; /* ͼ���豸����*/
    struct rt_semaphore lcd_lock; /* ˢ��ʹ�õ�����һ֡ͼ��ˢ��֮�����ˢ��һ֡ͼ������������Ҫ��*/
};

int drv_lcd_hw_init(void);

#endif  // _LCD_H
