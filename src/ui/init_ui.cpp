#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "config/config.h"
#include "lv_conf_internal.h"

// 声明全局字体
extern lv_font_t lvgl_font_digital_24;
extern lv_font_t lvgl_font_digital_48;
extern lv_font_t lvgl_font_digital_108;
extern lv_font_t lvgl_font_digital_64;
// LVGL对象定义
lv_obj_t* mao_select_label = nullptr;
lv_obj_t* toxic_soul_label = nullptr;
lv_obj_t* soul_label = nullptr;        // 禅语哲言标签
lv_obj_t* iciba_label = nullptr;
lv_obj_t* astronauts_label = nullptr;
lv_obj_t* news_label = nullptr;
lv_obj_t* calendar_label = nullptr;
lv_obj_t* today_date_label = nullptr;
lv_obj_t* note_label = nullptr;

// 从config.h中获取屏幕尺寸，不再重复定义
// 缓冲区设置 - 使用双缓冲区以提高显示性能
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];
static lv_color_t buf2[screenHeight * 10];

// TFT对象
TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight);

// LVGL显示回调函数
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

// 初始化LVGL显示驱动
void initDisplayDriver() {
  // 初始化显示屏
  tft.init();
  tft.setRotation(0); // 设置为正常方向（0度）
  tft.fillScreen(TFT_BLACK);

  // 初始化LVGL
  lv_init();

  // 配置显示缓冲区 - 使用双缓冲区
  lv_disp_draw_buf_init(&draw_buf, buf, buf2, screenWidth * 10);

  // 配置显示驱动
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.flush_cb = my_disp_flush;
  lv_disp_drv_register(&disp_drv);
}

// 初始化UI元素
void initUI() {
  Serial.println("初始化UI元素...");

  // 初始化显示驱动
  initDisplayDriver();

  // 设置背景颜色
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
  // 创建毛泽东选集标签
  mao_select_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(mao_select_label, &lvgl_font_song_16, 0);
  lv_obj_set_style_text_color(mao_select_label, lv_color_hex(0xFFFFFF), 0); // 白色文字以在黑色背景上可见
  lv_obj_set_width(mao_select_label, screenWidth - 20);
  lv_obj_align(mao_select_label, LV_ALIGN_TOP_MID, 0, 110);
  lv_label_set_long_mode(mao_select_label, LV_LABEL_LONG_WRAP);
  lv_obj_add_flag(mao_select_label, LV_OBJ_FLAG_HIDDEN);

  // 创建毒鸡汤标签
  toxic_soul_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(toxic_soul_label, &lvgl_font_song_16, 0);
  lv_obj_set_style_text_color(toxic_soul_label, lv_color_hex(0xFFFFFF), 0); // 白色文字以在黑色背景上可见
  lv_obj_set_width(toxic_soul_label, screenWidth - 20);
  lv_obj_align(toxic_soul_label, LV_ALIGN_TOP_MID, 0, 110);
  lv_label_set_long_mode(toxic_soul_label, LV_LABEL_LONG_WRAP);
  lv_obj_add_flag(toxic_soul_label, LV_OBJ_FLAG_HIDDEN);
  
  // 创建禅语哲言标签
  soul_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(soul_label, &lvgl_font_song_16, 0);
  lv_obj_set_style_text_color(soul_label, lv_color_hex(0xFFFFFF), 0); // 白色文字以在黑色背景上可见
  lv_obj_set_width(soul_label, screenWidth - 20);
  lv_obj_align(soul_label, LV_ALIGN_TOP_MID, 0, 110);
  lv_label_set_long_mode(soul_label, LV_LABEL_LONG_WRAP);
  lv_obj_add_flag(soul_label, LV_OBJ_FLAG_HIDDEN);

  // 创建金山词霸标签
  iciba_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(iciba_label, &lvgl_font_song_16, 0);
  lv_obj_set_style_text_color(iciba_label, lv_color_hex(0xFFFFFF), 0); // 白色文字以在黑色背景上可见
  lv_obj_set_width(iciba_label, screenWidth - 20);
  lv_obj_align(iciba_label, LV_ALIGN_TOP_MID, 0, 110);
  lv_label_set_long_mode(iciba_label, LV_LABEL_LONG_WRAP);
  lv_obj_add_flag(iciba_label, LV_OBJ_FLAG_HIDDEN);

  // 创建宇航员标签
  astronauts_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(astronauts_label, &lvgl_font_song_16, 0);
  lv_obj_set_style_text_color(astronauts_label, lv_color_hex(0xFFFFFF), 0); // 白色文字以在黑色背景上可见
  lv_obj_set_width(astronauts_label, screenWidth - 20);
  lv_obj_align(astronauts_label, LV_ALIGN_TOP_MID, 0, 110);
  lv_label_set_long_mode(astronauts_label, LV_LABEL_LONG_WRAP);
  lv_obj_add_flag(astronauts_label, LV_OBJ_FLAG_HIDDEN);

  // 创建新闻标签
  news_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(news_label, &lvgl_font_song_16, 0);
  lv_obj_set_style_text_color(news_label, lv_color_hex(0xFFFFFF), 0); // 白色文字以在黑色背景上可见
  lv_obj_set_width(news_label, screenWidth - 20);
  lv_obj_align(news_label, LV_ALIGN_TOP_MID, 0, 110);
  lv_label_set_long_mode(news_label, LV_LABEL_LONG_WRAP);
  lv_obj_add_flag(news_label, LV_OBJ_FLAG_HIDDEN);

  // 创建当日日期大字体标签（日历页上方居中）
  today_date_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(today_date_label, &lvgl_font_digital_108, 0); // 使用已定义的字体
  lv_obj_set_style_text_color(today_date_label, lv_color_hex(0xFFFFFF), 0); // 白色文字以在黑色背景上可见
  lv_obj_align(today_date_label, LV_ALIGN_TOP_MID, 0, 110); // 调整位置到顶部居中
  lv_obj_add_flag(today_date_label, LV_OBJ_FLAG_HIDDEN);

  // 创建日历标签（日历页右下方，位于config_label上方靠右边框）
  calendar_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(calendar_label, &lvgl_font_song_16, 0);
  lv_obj_set_style_text_color(calendar_label, lv_color_hex(0xFFFFFF), 0); // 白色文字以在黑色背景上可见
  lv_obj_set_width(calendar_label, screenWidth - 20);
  lv_obj_align(calendar_label, LV_ALIGN_BOTTOM_RIGHT, -20, -50); // 调整位置到右下角
  lv_label_set_long_mode(calendar_label, LV_LABEL_LONG_WRAP);
  lv_obj_add_flag(calendar_label, LV_OBJ_FLAG_HIDDEN);

  // 创建留言板标签
  note_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(note_label, &lvgl_font_song_16, 0);
  lv_obj_set_style_text_color(note_label, lv_color_hex(0xFFFFFF), 0); // 白色文字以在黑色背景上可见
  lv_obj_set_width(note_label, screenWidth - 20);
  lv_obj_align(note_label, LV_ALIGN_TOP_MID, 0, 110);
  lv_label_set_long_mode(note_label, LV_LABEL_LONG_WRAP);
  lv_obj_add_flag(note_label, LV_OBJ_FLAG_HIDDEN);

  Serial.println("UI元素初始化完成");
}