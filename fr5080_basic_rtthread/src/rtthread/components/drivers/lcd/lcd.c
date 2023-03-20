#include <rtthread.h>
#include <rtdef.h>

#include "lcd.h"

#include "qspi.h"
#include "lcm_ST7796h.h"

/* LCD ��Ϣ��صĺ궨�� */
#define LCD_HEIGHT              280
#define LCD_WIDTH               240
#define LCD_BITS_PER_PIXEL      16
#define LCD_PIXEL_FORMAT        RTGRAPHIC_PIXEL_FORMAT_RGB565

struct drv_lcd_dsi_device _lcd;

uint32_t framebuffer[(LCD_HEIGHT*LCD_WIDTH*LCD_BITS_PER_PIXEL/8)/sizeof(uint32_t)];
static volatile int m_refresh_flag = 0, m_refreshing = 0; /* ��Ҫˢ�º�����ˢ�±�־λ���䶼�� volatile ���Σ���ʾ `�ױ�` */

void test_qspi_reinit_st7796(void)
{
    // �ȴ�qspi controller���ڿ���״̬���ڽ��к�������
    while(qspi_is_busy());

    // ��д���ݵĸ�ʽ���óɷ���st7796�ĸ�ʽ
    qspi_write_set_address_type(QSPI_WIRE_TYPE_STAND);
    qspi_write_set_data_type(QSPI_WIRE_TYPE_STAND);
    qspi_write_set_dummy_cycles(0);
    qspi_write_set_wel_dis(1);
    qspi_poll_set_disable(1);

    qspi_ctrl->size_cfg.page_bytes = 0x400;

    // ����Ƶ�����ó�div2
    /*
     * QSPI_BAUDRATE_DIV_2  - 3
     * QSPI_BAUDRATE_DIV_4  - 0
     */
    qspi_ctrl->read_cap.delay_capture = 0;
    qspi_cfg_set_baudrate(QSPI_BAUDRATE_DIV_4);
}

static rt_err_t drv_lcd_init(struct rt_device *device)
{
    rt_err_t result = RT_EOK;
    
    struct drv_lcd_dsi_device *lcd = (struct drv_lcd_dsi_device *)device; /* ����ַ���� parent ���Կ���ʹ��ǿ������ת�� */

    printf("drv_lcd_init\r\n");
    test_qspi_reinit_st7796();
    st7796h_init();
    
    return result;
}

static rt_err_t drv_lcd_control(struct rt_device *device, int cmd, void *args)
{
    struct drv_lcd_dsi_device *lcd = (struct drv_lcd_dsi_device *)device;

    printf("drv_lcd_control: cmd is %d\r\n", cmd);
    switch (cmd)
    {
        case RTGRAPHIC_CTRL_RECT_UPDATE:
            {
                /* ��־λ��λ����ʾ��Ҫˢ���Դ� �����ˢ�¶�����ˢ���ص���ִ�� */
                m_refresh_flag = 1;
                struct rt_device_rect_info *rect_info = (void *)args;
                printf("RTGRAPHIC_CTRL_RECT_UPDATE: %d, %d, %d, %d\r\n", rect_info->x, rect_info->y, rect_info->width, rect_info->height);
                #if 1
                st7796h_draw(rect_info->x, rect_info->y, rect_info->width, rect_info->height, framebuffer);
                #else
                lcm_set_windows(rect_info->x, 
                                    rect_info->y,
                                    rect_info->x+rect_info->width-1,
                                    rect_info->y+rect_info->height-1);
                LCD_QSPI_WRITE(framebuffer, rect_info->width * rect_info->height * 2);
                #endif
            }
            break;
        case RTGRAPHIC_CTRL_GET_INFO:
            {
                /* ��ȡ LCD ��Ϣ */
                struct rt_device_graphic_info *info = (struct rt_device_graphic_info *)args;
                RT_ASSERT(info != RT_NULL);
                
                info->pixel_format = lcd->lcd_info.pixel_format;
                info->bits_per_pixel = lcd->lcd_info.bits_per_pixel;
                info->width = lcd->lcd_info.width;
                info->height = lcd->lcd_info.height;
                info->framebuffer = lcd->lcd_info.framebuffer;
            }
            break;
        case RTGRAPHIC_CTRL_POWERON:
            {
            /* �򿪱�������� */
            //turnon_backlight();
            }
            break;
        case RTGRAPHIC_CTRL_POWEROFF:
            {
            /* �رձ�������� */
            //turnoff_backlight();
            }
            break;
    }
    
    return RT_EOK;
}

int drv_lcd_hw_init(void)
{
    rt_err_t result;
    struct rt_device *device;
    
    /* ��ʼ���� */
    result = rt_sem_init(&_lcd.lcd_lock, "lcd_lock", 0, RT_IPC_FLAG_FIFO);
    
    /* ���� LCD ����Ϣ */
    _lcd.lcd_info.height = LCD_HEIGHT;
    _lcd.lcd_info.width = LCD_WIDTH;
    _lcd.lcd_info.bits_per_pixel = LCD_BITS_PER_PIXEL;
    _lcd.lcd_info.pixel_format = LCD_PIXEL_FORMAT;
    
    /* �����Դ��ַ */
    _lcd.lcd_info.framebuffer = (void *)&framebuffer[0];

    device = &_lcd.parent;
    /* �豸����Ϊͼ���豸 */
    device->type = RT_Device_Class_Graphic;
    /* �����豸�Ĳ������� */
    device->init = drv_lcd_init;
    device->control = drv_lcd_control;
    
    /* ע�� LCD �豸��ϵͳ�� */
    rt_device_register(device, "lcd", RT_DEVICE_FLAG_WRONLY);

    return result;
}

#if defined(PKG_USING_GUIENGINE)
#include <rtgui/driver.h>
int graphic_device_init(void)
{
    rt_err_t result;
    struct rt_device *device;
    
    /* �ҵ�ע�ᵽϵͳ�е� lcd �豸 */
    device = rt_device_find("lcd");
    if (device)
    {
        /* �� lcd �豸����Ϊ gui �����ͼ����ʾ�豸 */
        result = rtgui_graphic_set_device(device);
    }
    return 0;
}
INIT_ENV_EXPORT(graphic_device_init);
#endif

