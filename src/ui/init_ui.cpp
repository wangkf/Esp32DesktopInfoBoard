#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "config/config.h"
#include "config/config_manager.h"
#include "images/images.h"
#include "ui_utils.h"

// 全局主题变量
uint32_t TextColor, bgColor, timeColor,dayColor,chromaKey;
int themeId;

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

// 初始化主题颜色
void initThemeColors() {
  // 获取主题ID (0=黑夜, 1=白天, 2=自动等)
  themeId = ConfigManager::getInstance()->getDisplayTheme();
  
  // 根据主题ID确定颜色值
  switch(themeId) {
    case ConfigManager::THEME_LIGHT: // 白天主题 (1)
      TextColor = 0x000000;      // 黑色文字
      bgColor = 0xFFFFFF; // 白色背景
      timeColor = 0x999999;
      dayColor = 0xFF0000;
      chromaKey = 0x000000;
      break;
    case ConfigManager::THEME_DARK:  // 黑夜主题 (0)
    default:
      TextColor = 0xFFFFFF;      // 白色文字
      bgColor = 0x000000; // 黑色背景
      timeColor = 0x00FF00;
      dayColor = 0x00FF00;
      chromaKey = 0xFFFFFF;
      break;
    case ConfigManager::THEME_AUTO:  // 特殊主题 (2)
      TextColor = 0x000FFF;      // 蓝色文字
      bgColor = 0xCCCCE0; // 淡黄色背景
      timeColor = 0xBBBBBB;
      dayColor = 0x0000FF;
      chromaKey = 0x00ee00;
      break;
  }
}

// 定义LVGL透明键色
#define CONFIG_LV_COLOR_CHROMA_KEY chromaKey // 默认值，将在initThemeColors中实际设置
#include "lv_conf_internal.h"

// 重新应用主题设置
void reapplyTheme() {
  Serial.println("重新应用主题设置...");
  // 重新加载主题颜色
  initThemeColors();
  // 重新设置屏幕背景色
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(bgColor), 0);
  // 重新设置所有标签的文字颜色
  if (news_label) lv_obj_set_style_text_color(news_label, lv_color_hex(TextColor), 0);
  if (calendar_label) lv_obj_set_style_text_color(calendar_label, lv_color_hex(TextColor), 0);
  if (today_date_label) lv_obj_set_style_text_color(today_date_label,lv_color_hex(dayColor), 0);
  if (iciba_label) lv_obj_set_style_text_color(iciba_label, lv_color_hex(TextColor), 0);
  if (astronauts_label) lv_obj_set_style_text_color(astronauts_label, lv_color_hex(TextColor), 0);
  if (mao_select_label) lv_obj_set_style_text_color(mao_select_label, lv_color_hex(TextColor), 0);
  if (toxic_soul_label) lv_obj_set_style_text_color(toxic_soul_label, lv_color_hex(TextColor), 0);
  if (soul_label) lv_obj_set_style_text_color(soul_label, lv_color_hex(TextColor), 0);
  if (note_label) lv_obj_set_style_text_color(note_label, lv_color_hex(TextColor), 0);
  
  // 重新设置所有图像的背景色
  if (calendar_img) lv_obj_set_style_bg_color(calendar_img, lv_color_hex(bgColor), 0);
  if (iciba_img) lv_obj_set_style_bg_color(iciba_img, lv_color_hex(bgColor), 0);
  if (astronauts_img) lv_obj_set_style_bg_color(astronauts_img, lv_color_hex(bgColor), 0);
  if (maoselect_img) lv_obj_set_style_bg_color(maoselect_img, lv_color_hex(bgColor), 0);
  if (toxic_soul_img) lv_obj_set_style_bg_color(toxic_soul_img, lv_color_hex(bgColor), 0);
  if (soul_img) lv_obj_set_style_bg_color(soul_img, lv_color_hex(bgColor), 0);
  
  // 刷新屏幕以应用更改
  lv_obj_invalidate(lv_scr_act());
  
  Serial.println("主题设置重新应用完成");
}
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
  initThemeColors();  // 初始化主题颜色
  initDisplayDriver();   // 初始化显示驱动
//lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(bgColor), 0);
  // 使用通用函数创建所有标签，createLabel函数会根据主题自动设置文字颜色
  news_label =       createLabel(GBFont,     lv_color_hex(TextColor),  0, 85,screenHeight-85);
  calendar_label =   createLabel(GBFont,     lv_color_hex(TextColor), 120, 240);
  today_date_label = createLabel(&lvgl_font_digital_108, lv_color_hex(themeId == ConfigManager::THEME_LIGHT ? 0x000000 : 0xFF0000),  0, 85, 0);
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