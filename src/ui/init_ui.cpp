#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "config/config.h"
#include "lv_conf_internal.h"
#include "../images/images.h"
#include "ui_utils.h"
// 声明全局字体
extern const lv_font_t lvgl_font_digital_24;
extern const lv_font_t lvgl_font_digital_48;
extern const lv_font_t lvgl_font_digital_108;
extern const lv_font_t lvgl_font_digital_64;
// LVGL对象定义
lv_obj_t* mao_select_label = nullptr;
lv_obj_t* toxic_soul_label = nullptr;
lv_obj_t* toxic_soul_img = nullptr;    // 毒鸡汤背景图像
lv_obj_t* soul_label = nullptr;        // 禅语哲言标签
lv_obj_t* soul_img = nullptr;          // 禅语背景图像
lv_obj_t* maoselect_img = nullptr;     // 毛泽东选集背景图像
lv_obj_t* iciba_label = nullptr;
lv_obj_t* iciba_img = nullptr;
lv_obj_t* astronauts_label = nullptr;
lv_obj_t* astronauts_img = nullptr;    // 宇航员背景图像
lv_obj_t* news_label = nullptr;
lv_obj_t* calendar_label = nullptr;
lv_obj_t* calendar_img = nullptr;      // 日历背景图像
lv_obj_t* today_date_label = nullptr;
lv_obj_t* note_label = nullptr;
// 从config.h中获取屏幕尺寸，不再重复定义；缓冲区设置 - 使用双缓冲区以提高显示性能
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
  
// 使用通用函数创建所有标签
news_label =       createLabel(GBFont,     lv_color_hex(0xFFFFFF),  0, 85,screenHeight-85,  lv_color_hex(0xdddddd));
calendar_label =   createLabel(GBFont,     lv_color_hex(0xFFFFFF), 120, 240);
today_date_label = createLabel(&lvgl_font_digital_108, lv_color_hex(0xFF0000),  0, 85, 0, lv_color_hex(0x000000), LV_OPA_TRANSP, false);
iciba_label =      createLabel(GBFont,     lv_color_hex(0xFFFFFF),  0, 85, screenHeight-85, lv_color_hex(0x3E92F2));
astronauts_label = createLabel(GBFont,     lv_color_hex(0xFFFFFF),  0, 85,screenHeight-85,  lv_color_hex(0x000080));
mao_select_label = createLabel(GBFont,     lv_color_hex(0xFFFFFF),  0, 220);  
toxic_soul_label = createLabel(GBFont,     lv_color_hex(0xFFFFFF),  0, 85, screenHeight-85, lv_color_hex(0xFFCF03));
soul_label =       createLabel(GBFont,     lv_color_hex(0xFFFFFF),  0, 85,screenHeight-85,  lv_color_hex(0xE49E00));
// 创建留言板标签
note_label =       createLabel(GBFont, lv_color_hex(0xFFFFFF),  0, 110);

// 使用通用函数创建所有图像
calendar_img   = createImage(&calendar,  120, 120,   0, 360);
iciba_img      = createImage(&iciba,     80, 80,    0, 400);
astronauts_img = createImage(&astronauts,320, 80,   0, 400);
maoselect_img  = createImage(&maoselect, 320, 120,  0, 80);
toxic_soul_img = createImage(&taxicsoul, 320, 160,  0, 320);
soul_img       = createImage(&soul,      320, 120,  0, 360);
Serial.println("UI元素初始化完成");
}