#include "lv_line_test.h"
#include "lvgl.h"
//#include "key.h"

const lv_point_t line_points[] = {  {10, 20}, {70, 50}, {120, 10}, {140, 60}, {180, 10}};
#define LINE_POINTS_NUM         (sizeof(line_points)/sizeof(line_points[0]))

const lv_point_t line_points1[] = {  {10, 60}, {70, 120}};
#define LINE_POINTS_NUM1         (sizeof(line_points1)/sizeof(line_points1[0]))

lv_obj_t * line1;
lv_obj_t * line2;

void lv_line_test_start()
{
	lv_obj_t * scr = lv_scr_act();
	lv_obj_t * scr1 = lv_scr_act();

	static lv_style_t line_style;
	lv_style_copy(&line_style, &lv_style_plain);
	line_style.line.color = LV_COLOR_RED;
	line_style.line.width = 4;
	line_style.line.rounded = 1;

	static lv_style_t line_style2;
	lv_style_copy(&line_style2, &lv_style_plain);
	line_style2.line.color = LV_COLOR_MAGENTA;
	line_style2.line.width = 4;
	line_style2.line.rounded = 1;


	line1 = lv_line_create(scr, NULL);
	lv_obj_set_pos(line1,120,120);
	lv_line_set_auto_size(line1,true);
	lv_line_set_points(line1, line_points, LINE_POINTS_NUM);
	lv_line_set_style(line1, LV_LINE_STYLE_MAIN, &line_style);

	line2 = lv_line_create(scr1, NULL);
	lv_obj_set_pos(line2,200,200);
	lv_line_set_auto_size(line2,true);
	lv_line_set_points(line2, line_points1, LINE_POINTS_NUM1);
	lv_line_set_style(line2, LV_LINE_STYLE_MAIN, &line_style2);
}



#if 0
void key_handler()
{
	u8 key = KEY_Scan(0);
	
	if(key==KEY0_PRES)
	{
		lv_line_set_y_invert(line1,!lv_line_get_y_invert(line1));//来回取反
	}
}
#endif


