#include "lv_bar_test.h"
#include "lvgl.h"


lv_style_t bar_bg_style;
lv_style_t bar_indic_style;



void lv_bar_test_start()
{
	lv_obj_t * scr = lv_scr_act();


	lv_style_copy(&bar_bg_style,&lv_style_plain_color);
	bar_bg_style.body.main_color = LV_COLOR_MAKE(0xBB,0xBB,0xBB);
	bar_bg_style.body.grad_color = LV_COLOR_MAKE(0xBB,0xBB,0xBB);
	bar_bg_style.body.radius = LV_RADIUS_CIRCLE;

	lv_style_copy(&bar_indic_style,&lv_style_plain_color);
	bar_indic_style.body.main_color = LV_COLOR_MAKE(0x5F,0xB8,0x78);
	bar_indic_style.body.grad_color = LV_COLOR_MAKE(0x5F,0xB8,0x78);
	bar_indic_style.body.radius = LV_RADIUS_CIRCLE;
	bar_indic_style.body.padding.left = 0;
	bar_indic_style.body.padding.top = 0;
	bar_indic_style.body.padding.right = 0;
	bar_indic_style.body.padding.bottom = 0;
	


	lv_obj_t * bar1 = lv_bar_create(scr, NULL);
	lv_obj_set_size(bar1,180,16);
	lv_obj_set_pos(bar1,120,120);
	lv_bar_set_style(bar1,LV_BAR_STYLE_BG,&bar_bg_style);
	//lv_bar_set_style(bar1,LV_BAR_STYLE_INDIC,&bar_indic_style);
	//lv_bar_set_anim_time(bar1,1000);
	//lv_bar_set_value(bar1,100,LV_ANIM_ON);


//	lv_obj_t * bar2 = lv_bar_create(scr, bar1);
//	lv_obj_set_size(bar2,16,180);
//	lv_obj_align(bar2,bar1,LV_ALIGN_OUT_BOTTOM_LEFT,0,10);
//	lv_bar_set_range(bar2,100,200);
//	lv_bar_set_anim_time(bar2,1000);
//	lv_bar_set_value(bar2,180,LV_ANIM_ON);
}
