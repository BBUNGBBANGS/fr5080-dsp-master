#include "lv_image_test.h"
#include "lvgl.h"

//例程入口函数
void lv_image_test_start()
{
	LV_IMG_DECLARE(TEST_UI1);
	lv_obj_t * scr = lv_disp_get_scr_act(NULL);//获取屏幕对象

	lv_obj_t * image = lv_img_create(scr, NULL);

	lv_img_set_src(image, &TEST_UI1);

	lv_obj_align(image, NULL, LV_ALIGN_CENTER,0,0);
	
}










