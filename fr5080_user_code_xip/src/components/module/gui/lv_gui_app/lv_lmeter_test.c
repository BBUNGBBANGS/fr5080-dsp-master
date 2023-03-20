#include "lv_lmeter_test.h"
#include "lvgl.h"
#include <stdio.h>


lv_style_t main_style;
lv_obj_t * lmeter1;
lv_obj_t * label1;

uint16_t lmeter_value = 0;

#if 0
void task_cb(lv_task_t *task)
{
	char buff[10];
	lmeter_value += 5;
	if(lmeter_value>100)
			lmeter_value = 0;
	lv_obj_align(lmeter1,NULL,LV_ALIGN_CENTER,0,0);
	lv_lmeter_set_value(lmeter1,lmeter_value);
//	sprintf(buff,"%d%%",lmeter_value);
//	lv_label_set_text(label1,buff);
}
#endif


void lv_lmeter_test_start()
{

#if 1
	lv_obj_t * scr = lv_scr_act();

	lv_style_copy(&main_style,&lv_style_plain_color);
	main_style.body.main_color 		= LV_COLOR_GREEN;
	main_style.body.grad_color 		= LV_COLOR_RED;
	main_style.line.color 			= LV_COLOR_SILVER;
	main_style.line.width 			= 2;
	main_style.body.padding.left 	= 16;


	lmeter1 = lv_lmeter_create(scr,NULL);
	printf("\r\n lmeter1 = %d \r\n", lmeter1);
	lv_obj_set_size(lmeter1,180,180);
	lv_obj_align(lmeter1,NULL,LV_ALIGN_CENTER,0,0);
	lv_lmeter_set_range(lmeter1,0,100);
	lv_lmeter_set_value(lmeter1,0);
	lv_lmeter_set_scale(lmeter1,240,31);
	lv_lmeter_set_style(lmeter1,LV_LMETER_STYLE_MAIN,&main_style);

//	label1 = lv_label_create(scr,NULL);
//	lv_obj_align(label1,lmeter1,LV_ALIGN_CENTER,0,0);
//	lv_obj_set_auto_realign(label1,true);
//	lv_label_set_text(label1,"0%");
//	lv_task_create(task_cb,1000,LV_TASK_PRIO_MID,NULL);
#endif

}


