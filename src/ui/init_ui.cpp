#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "config/config.h"
#include "config/config_manager.h"
#include "images/images.h"
#include "ui_utils.h"

// 全局主题变量
uint32_t TextColor, bgColor, chromaKey;
int themeId;

// 初始化主题颜色
void initThemeColors() {
  // 获取主题ID (0=黑夜, 1=白天, 2=自动等)
  themeId = ConfigManager::getInstance()->getDisplayTheme();
  
  // 根据主题ID确定颜色值
  switch(themeId) {
    case ConfigManager::THEME_LIGHT: // 白天主题 (1)
      TextColor = 0x000000;      // 黑色文字
      bgColor = 0xFFFFFF; // 白色背景
      chromaKey = 0x00ee00;       // 黑色透明键
      break;
    case ConfigManager::THEME_DARK:  // 黑夜主题 (0)
    default:
      TextColor = 0xFFFFFF;      // 白色文字
      bgColor = 0x000000; // 黑色背景
      chromaKey = 0xFFeeFF;       // 白色透明键
      break;
    case ConfigManager::THEME_AUTO:  // 特殊主题 (2)
      TextColor = 0x000000;      // 黑色文字
      bgColor = 0xFFFFE0; // 淡黄色背景
      chromaKey = 0x00ee00;       // 黑色透明键
      break;
  }
}

// 定义LVGL透明键色
#define CONFIG_LV_COLOR_CHROMA_KEY chromaKey // 默认值，将在initThemeColors中实际设置
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
  //tft.fillScreen(isLightTheme ? 0xFFFFFF : 0x000000);
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
  
  // 初始化主题颜色
  initThemeColors();
  
  // 初始化显示驱动
  initDisplayDriver(); 
  
  // 绘制固定灰色背景（0,0,240,85）区域，不随主题变化
  // 使用RGB565颜色值0x8080表示灰色
  tft.fillRect(0, 0, 240, 85, 0x8080);
  
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(bgColor), 0);
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