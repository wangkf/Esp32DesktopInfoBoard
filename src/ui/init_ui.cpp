#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "config/config.h"
#include "config/config_manager.h"
#include "images/images.h"
#include "ui_utils.h"
bool isLightTheme = ConfigManager::getInstance()->getDisplayTheme();
#ifdef isLightTheme
  #define CONFIG_LV_COLOR_CHROMA_KEY 0x000000
#else
  #define CONFIG_LV_COLOR_CHROMA_KEY 0xFFFFFF
#endif
#include "lv_conf_internal.h"
// LVGL对象定义
lv_obj_t* mao_select_label = nullptr;
lv_obj_t* toxic_soul_label = nullptr;
lv_obj_t* toxic_soul_img = nullptr;
lv_obj_t* soul_label = nullptr;
lv_obj_t* soul_img = nullptr;
lv_obj_t* maoselect_img = nullptr;
lv_obj_t* iciba_label = nullptr;
lv_obj_t* iciba_img = nullptr;
lv_obj_t* astronauts_label = nullptr;
lv_obj_t* astronauts_img = nullptr;
lv_obj_t* news_label = nullptr;
lv_obj_t* calendar_label = nullptr;
lv_obj_t* calendar_img = nullptr;
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
  // 获取主题配置

  tft.fillScreen(isLightTheme ? 0xFFFFFF : 0x000000);
  lv_init();  // 初始化LVGL
  // 配置显示缓冲区 - 使用双缓冲区
  lv_disp_draw_buf_init(&draw_buf, buf, buf2, screenWidth * 10);
  static lv_disp_drv_t disp_drv; // 配置显示驱动
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
  // 获取主题配置 - 强制使用亮色主题进行测试
  bool isLightTheme = ConfigManager::getInstance()->getDisplayTheme(); // 强制使用亮色主题
  // 设置背景颜色
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(isLightTheme ? 0xFFFFFF : 0x000000), 0);
  uint32_t TextColor = isLightTheme ? 0x000000 : 0xFFFFFF;
  uint32_t bgColor = isLightTheme ? 0x000000 : 0xFFFFFF;  
// 使用通用函数创建所有标签，createLabel函数会根据主题自动设置文字颜色
news_label =       createLabel(GBFont,     lv_color_hex(TextColor),  0, 85,screenHeight-85);
calendar_label =   createLabel(GBFont,     lv_color_hex(TextColor), 120, 240);
today_date_label = createLabel(&lvgl_font_digital_108, lv_color_hex(isLightTheme ? 0x000000 : 0xFF0000),  0, 85, 0);
iciba_label =      createLabel(GBFont,     lv_color_hex(TextColor),  0, 85, screenHeight-85);
astronauts_label = createLabel(GBFont,     lv_color_hex(TextColor),  0, 85,screenHeight-85);
mao_select_label = createLabel(GBFont,     lv_color_hex(TextColor),  0, 220);  
toxic_soul_label = createLabel(GBFont,     lv_color_hex(TextColor),  0, 85, screenHeight-85);
soul_label =       createLabel(GBFont,     lv_color_hex(TextColor),  0, 85,screenHeight-85);
note_label =       createLabel(GBFont, lv_color_hex(TextColor),  0, 110);
// 使用通用函数创建所有图像
calendar_img   = createImage(&calendar,  120, 120,   0, 360,lv_color_hex(bgColor));
iciba_img      = createImage(&iciba,     80, 80,    0, 400,lv_color_hex(bgColor));
astronauts_img = createImage(&astronauts,320, 80,   0, 400,lv_color_hex(bgColor));
maoselect_img  = createImage(&maoselect, 320, 120,  0, 80,lv_color_hex(bgColor));
toxic_soul_img = createImage(&taxicsoul, 320, 160,  0, 320,lv_color_hex(bgColor));
soul_img       = createImage(&soul,      320, 120,  0, 360,lv_color_hex(bgColor));
Serial.println("UI元素初始化完成");
}