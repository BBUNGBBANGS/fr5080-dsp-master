#include "lv_gauge_test.h"
#include "lvgl.h"
#include <stdio.h>


static lv_style_t gauge_style;
static lv_obj_t * gauge1;
static lv_obj_t * label1;

lv_color_t needle_colors[2];
int16_t speed_val = 0;
int16_t speed_val1 = 0;


void task_cb1(lv_task_t * task)
{
	static uint8_t is_add_dir = 1;
	char buff[40];

	if(1)
	//if(is_add_dir)
	{
			speed_val += 5;
			if(speed_val>=100)
			{
				speed_val  = 0;
				is_add_dir = 0;
				speed_val1 += 5;
				if(speed_val1 >= 100)
				{
					speed_val1 = 0;
				}
			}
	}

	lv_gauge_set_value(gauge1,0,speed_val);
	lv_gauge_set_value(gauge1,1,speed_val1);
	if(speed_val<60)
			sprintf(buff,"#5FB878 %d km/h#",speed_val);
	else if(speed_val<90)
			sprintf(buff,"#FFB800 %d km/h#",speed_val);
	else
			sprintf(buff,"#FF0000 %d km/h#",speed_val);
	lv_label_set_text(label1,buff);
}




void lv_gauge_test_start()
{
	lv_obj_t * scr = lv_scr_act();


	lv_style_copy(&gauge_style, &lv_style_pretty_color);
	gauge_style.body.main_color = LV_COLOR_MAKE(0x5F,0xB8,0x78);
	gauge_style.body.grad_color =  LV_COLOR_MAKE(0xFF,0xB8,0x00);
	gauge_style.body.padding.left = 10;
	gauge_style.body.padding.inner = 8;
	gauge_style.body.border.color = LV_COLOR_MAKE(0x33,0x33,0x33);
	gauge_style.line.width = 3;
	gauge_style.text.color = LV_COLOR_BLACK;
	gauge_style.line.color = LV_COLOR_RED;


	gauge1 = lv_gauge_create(scr, NULL);
	lv_obj_set_size(gauge1,300,300);
	lv_gauge_set_style(gauge1,LV_GAUGE_STYLE_MAIN,&gauge_style);
	lv_gauge_set_range(gauge1,0,100);
	needle_colors[0] = LV_COLOR_BLUE;
	needle_colors[1] = LV_COLOR_PURPLE;
	lv_gauge_set_needle_count(gauge1,sizeof(needle_colors)/sizeof(needle_colors[0]),needle_colors);
	lv_gauge_set_value(gauge1,0,speed_val);
	lv_gauge_set_value(gauge1,1,speed_val1);
	lv_gauge_set_critical_value(gauge1,90);
	lv_gauge_set_scale(gauge1,360,60,12);
	lv_obj_align(gauge1,NULL,LV_ALIGN_CENTER,0,0);


	label1 = lv_label_create(scr,NULL);
	lv_label_set_long_mode(label1,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(label1,80);
	lv_label_set_align(label1,LV_LABEL_ALIGN_CENTER);
	lv_label_set_style(label1,LV_LABEL_STYLE_MAIN,&lv_style_pretty);
	lv_label_set_body_draw(label1,true);
	lv_obj_align(label1,gauge1,LV_ALIGN_CENTER,0,60);
	lv_label_set_text(label1,"0 km/h");
	lv_label_set_recolor(label1,true);

	lv_task_create(task_cb1,1000,LV_TASK_PRIO_MID,NULL);
}


