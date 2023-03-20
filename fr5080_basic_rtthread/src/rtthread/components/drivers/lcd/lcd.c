#include <rtthread.h>
#include <rtdef.h>

#include "lcd.h"

#include "qspi.h"
#include "lcm_ST7796h.h"

/* LCD 信息相关的宏定义 */
#define LCD_HEIGHT              280
#define LCD_WIDTH               240
#define LCD_BITS_PER_PIXEL      16
#define LCD_PIXEL_FORMAT        RTGRAPHIC_PIXEL_FORMAT_RGB565

struct drv_lcd_dsi_device _lcd;

uint32_t framebuffer[(LCD_HEIGHT*LCD_WIDTH*LCD_BITS_PER_PIXEL/8)/sizeof(uint32_t)];
static volatile int m_refresh_flag = 0, m_refreshing = 0; /* 需要刷新和正在刷新标志位，其都用 volatile 修饰，表示 `易变` */

void test_qspi_reinit_st7796(void)
{
    // 等待qspi controller处于空闲状态后，在进行后续操作
    while(qspi_is_busy());

    // 将写数据的格式配置成符合st7796的格式
    qspi_write_set_address_type(QSPI_WIRE_TYPE_STAND);
    qspi_write_set_data_type(QSPI_WIRE_TYPE_STAND);
    qspi_write_set_dummy_cycles(0);
    qspi_write_set_wel_dis(1);
    qspi_poll_set_disable(1);

    qspi_ctrl->size_cfg.page_bytes = 0x400;

    // 将分频比配置成div2
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
    
    struct drv_lcd_dsi_device *lcd = (struct drv_lcd_dsi_device *)device; /* 基地址都是 parent 所以可以使用强制类型转换 */

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
                /* 标志位置位，表示需要刷新显存 具体的刷新动作在刷屏回调中执行 */
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
                /* 获取 LCD 信息 */
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
            /* 打开背光如果有 */
            //turnon_backlight();
            }
            break;
        case RTGRAPHIC_CTRL_POWEROFF:
            {
            /* 关闭背光如果有 */
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
    
    /* 初始化锁 */
    result = rt_sem_init(&_lcd.lcd_lock, "lcd_lock", 0, RT_IPC_FLAG_FIFO);
    
    /* 配置 LCD 的信息 */
    _lcd.lcd_info.height = LCD_HEIGHT;
    _lcd.lcd_info.width = LCD_WIDTH;
    _lcd.lcd_info.bits_per_pixel = LCD_BITS_PER_PIXEL;
    _lcd.lcd_info.pixel_format = LCD_PIXEL_FORMAT;
    
    /* 设置显存地址 */
    _lcd.lcd_info.framebuffer = (void *)&framebuffer[0];

    device = &_lcd.parent;
    /* 设备类型为图形设备 */
    device->type = RT_Device_Class_Graphic;
    /* 保存设备的操作方法 */
    device->init = drv_lcd_init;
    device->control = drv_lcd_control;
    
    /* 注册 LCD 设备到系统中 */
    rt_device_register(device, "lcd", RT_DEVICE_FLAG_WRONLY);

    return result;
}

#if defined(PKG_USING_GUIENGINE)
#include <rtgui/driver.h>
int graphic_device_init(void)
{
    rt_err_t result;
    struct rt_device *device;
    
    /* 找到注册到系统中的 lcd 设备 */
    device = rt_device_find("lcd");
    if (device)
    {
        /* 将 lcd 设备设置为 gui 引擎的图形显示设备 */
        result = rtgui_graphic_set_device(device);
    }
    return 0;
}
INIT_ENV_EXPORT(graphic_device_init);
#endif

