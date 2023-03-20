#include "lv_obj_test.h"
#include "lvgl.h"



lv_obj_t * scr;
lv_obj_t * obj1 = NULL;
lv_obj_t * obj2 = NULL;
lv_obj_t * obj3 = NULL;
lv_style_t red_style;

//例程入口函数
void lv_obj_test_start()
{
	scr = lv_scr_act();//获取当前活跃的屏幕对象
	
	//先创建一个红色背景的样式,对于样式相关的代码,看不懂的话,没关系,后面会详细讲解
	
	lv_style_copy(&red_style,&lv_style_plain_color);//先从系统自带的lv_style_plain样式拷贝过来,这样就不用对每个属性进行赋值了
	red_style.body.main_color = LV_COLOR_RED;
	red_style.body.grad_color = LV_COLOR_RED;
	
	//创建一个基本对象1
	obj1 = lv_obj_create(scr,NULL);
	lv_obj_set_pos(obj1,80,80);//设置坐标位置
	lv_obj_set_size(obj1,100,100);//设置大小
	
	//创建一个基本对象2,与对象1进行外部底下居中对齐,同时y轴向上偏移10个像素,
	//目的是为了让obj2有一部分压在obj1上,方便后面演示z轴层级改变的API接口
	obj2 = lv_obj_create(scr,NULL);
	lv_obj_set_size(obj2,50,50);
	lv_obj_set_style(obj2,&red_style);//设置新的样式
	lv_obj_align(obj2,obj1,LV_ALIGN_OUT_BOTTOM_MID,0,-20);

	obj3 = lv_obj_create(scr,obj1);//从obj1拷贝过来,减少一些属性的赋值
	lv_obj_align(obj3,NULL,LV_ALIGN_CENTER,0,0);//传NULL,那么父对象就是参考对象
	lv_obj_set_drag(obj3,true);//设置obj3可以被拖拽
	lv_obj_set_drag_throw(obj3,true);//同时使能obj3的拖拽惯性滑动功能
	lv_obj_set_parent(obj2,obj3);//修改obj2的父对象为obj3
	lv_obj_set_pos(obj2,10,10);//obj2得重新设置一下坐标
	
	lv_obj_set_opa_scale_enable(obj1,true);//先使能obj1的透明度功能
	lv_obj_set_opa_scale(obj1,100);//再设置obj1的透明度为50
}

//按键处理
//注意:请按照先按KEY0键,再按KEY1键,最后按KEY2键的顺序来观察实验现象
#if 0
void key_handler()
{
	u8 key = KEY_Scan(0);
	
	if(key==KEY0_PRES) 
	{
		//z轴层级改变演示,将obj1对象进行置顶,有三种实现方式
		#define Z_LAYER_MODE	1 //1,2,3分别对应三种实现方式
		
		#if(Z_LAYER_MODE==1)
			if(obj1)
				lv_obj_move_foreground(obj1);
		#elif(Z_LAYER_MODE==2)
			lv_obj_move_background(obj2);
		#elif(Z_LAYER_MODE==3)
			lv_obj_set_top(obj1,true);
		#endif
		printf("obj1 is on top layer\r\n");
	}else if(key==KEY1_PRES)
	{
		//动态创建一个对象3,与屏幕居中对齐,然后更改对象2的父亲为对象3
		obj3 = lv_obj_create(scr,obj1);//从obj1拷贝过来,减少一些属性的赋值
		lv_obj_align(obj3,NULL,LV_ALIGN_CENTER,0,0);//传NULL,那么父对象就是参考对象
		lv_obj_set_drag(obj3,true);//设置obj3可以被拖拽
		lv_obj_set_drag_throw(obj3,true);//同时使能obj3的拖拽惯性滑动功能
		lv_obj_set_parent(obj2,obj3);//修改obj2的父对象为obj3
		lv_obj_set_pos(obj2,10,10);//obj2得重新设置一下坐标
		
		lv_obj_set_opa_scale_enable(obj1,true);//先使能obj1的透明度功能
		lv_obj_set_opa_scale(obj1,100);//再设置obj1的透明度为50
		
		//接下来,你可以试一试在屏幕上拖拽obj3
		//设置对象3可以被拖拽,注意拖拽时,手指一定得按在obj3上,
		//不能按在obj2上,否则看不到正确的拖拽现象,虽然obj2是obj3
		//的子对象,但是obj2是没有使能拖拽功能的
	}else if(key==WKUP_PRES)
	{
		if(obj1)
		{
			//删除obj1对象,有2种实现方式
			#define DEL_MODE	1 
			
			#if(DEL_MODE==1)
				lv_obj_del(obj1);
			#elif(DEL_MODE==2)
				lv_obj_del_async(obj1);
			#endif
			obj1 = NULL;
			printf("obj1 is deleted\r\n");
		}
	}
}
#endif









