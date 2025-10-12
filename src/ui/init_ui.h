#ifndef INIT_UI_H
#define INIT_UI_H

#include <lvgl.h>

// LVGL对象声明
extern lv_obj_t* time_label;
extern lv_obj_t* minute_label;
extern lv_obj_t* second_label;
extern lv_obj_t* date_label;
extern lv_obj_t* weekday_label;
extern lv_obj_t* news_label;
extern lv_obj_t* mao_select_label;
extern lv_obj_t* toxic_soul_label;
extern lv_obj_t* iciba_label;
extern lv_obj_t* astronauts_label;

// 函数声明
void initUI();

#endif // INIT_UI_H