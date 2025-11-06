#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "config/config.h"
#include "config/config_manager.h"
#include "images/images.h"
#include "ui_utils.h"
#include "ui_manager.h"

// 使用UIManager单例替代全局变量
// UI元素全局变量声明
lv_obj_t* news_label;
lv_obj_t* calendar_label;
lv_obj_t* today_date_label;
lv_obj_t* iciba_label;
lv_obj_t* astronauts_label;
lv_obj_t* mao_select_label;
lv_obj_t* toxic_soul_label;
lv_obj_t* soul_label;
lv_obj_t* note_label;
lv_obj_t* calendar_img;
lv_obj_t* iciba_img;
lv_obj_t* astronauts_img;
lv_obj_t* maoselect_img;
lv_obj_t* toxic_soul_img;
lv_obj_t* soul_img;

// 初始化LVGL显示驱动 - 现在由UIManager管理

// 提供兼容的函数接口，内部调用UIManager
void initThemeColors() {
  UIManager::getInstance()->initThemeColors();
}

// 提供兼容的函数接口，内部调用UIManager
void reapplyTheme() {
  UIManager::getInstance()->reapplyTheme();
}

// 声明外部TFT对象
extern TFT_eSPI tft;

// 定义时间颜色变量
uint32_t timeColor = TFT_WHITE;

// 初始化UI元素
void initUI() {
  Serial.println("初始化UI元素..."); 
  
  // 1. 初始化UIManager和LVGL核心功能
    UIManager::getInstance()->init();
    
    // 2. 初始化主题颜色
    initThemeColors();
    
    // 3. 设置屏幕背景色 - 使用UIManager获取颜色值
    lv_obj_set_style_bg_color(lv_scr_act(), UIManager::getInstance()->getColorValue(COLOR_BG), 0);
    
    // 获取当前主题ID
    int themeId = ConfigManager::getInstance()->getDisplayTheme();
  
  // 获取UIManager实例
    UIManager* uiManager = UIManager::getInstance();
    
    // 使用通用函数创建所有标签，并注册到UIManager
    news_label = createLabel(GBFont, uiManager->getColorValue(COLOR_TEXT), 0, 85, screenHeight-85);
    uiManager->registerElement("news_label", news_label, LABEL);
    
    calendar_label = createLabel(GBFont, uiManager->getColorValue(COLOR_TEXT), 120, 240);
    uiManager->registerElement("calendar_label", calendar_label, LABEL);
    
    today_date_label = createLabel(BIGFont,
                                  uiManager->getColorValue( COLOR_DAY), 
                                  0, 85, 0);
    uiManager->registerElement("today_date_label", today_date_label, LABEL);
    
    iciba_label = createLabel(GBFont, uiManager->getColorValue(COLOR_TEXT), 0, 85, screenHeight-85);
    uiManager->registerElement("iciba_label", iciba_label, LABEL);
    
    astronauts_label = createLabel(GBFont, uiManager->getColorValue(COLOR_TEXT), 0, 85, screenHeight-85);
    uiManager->registerElement("astronauts_label", astronauts_label, LABEL);
    
    mao_select_label = createLabel(GBFont, uiManager->getColorValue(COLOR_TEXT), 0, 220);
    uiManager->registerElement("mao_select_label", mao_select_label, LABEL);
    
    toxic_soul_label = createLabel(GBFont, uiManager->getColorValue(COLOR_TEXT), 0, 85, screenHeight-85);
    uiManager->registerElement("toxic_soul_label", toxic_soul_label, LABEL);
    
    soul_label = createLabel(GBFont, uiManager->getColorValue(COLOR_TEXT), 0, 85, screenHeight-85);
    uiManager->registerElement("soul_label", soul_label, LABEL);
    
    note_label = createLabel(GBFont, uiManager->getColorValue(COLOR_TEXT), 0, 110);
    uiManager->registerElement("note_label", note_label, LABEL);
    
    // 使用通用函数创建所有图像，并注册到UIManager
    calendar_img = createImage(&calendar, 120, 120, 0, 360, uiManager->getColorValue(COLOR_BG));
    uiManager->registerElement("calendar_img", calendar_img, IMAGE);
    
    iciba_img = createImage(&iciba, 80, 80, 0, 400, uiManager->getColorValue(COLOR_BG));
    uiManager->registerElement("iciba_img", iciba_img, IMAGE);
    
    astronauts_img = createImage(&astronauts, 320, 80, 0, 400, uiManager->getColorValue(COLOR_BG));
    uiManager->registerElement("astronauts_img", astronauts_img, IMAGE);
    
    maoselect_img = createImage(&maoselect, 320, 120, 0, 80, uiManager->getColorValue(COLOR_BG));
    uiManager->registerElement("maoselect_img", maoselect_img, IMAGE);
    
    toxic_soul_img = createImage(&taxicsoul, 320, 160, 0, 320, uiManager->getColorValue(COLOR_BG));
    uiManager->registerElement("toxic_soul_img", toxic_soul_img, IMAGE);
    
    soul_img = createImage(&soul, 320, 120, 0, 360, uiManager->getColorValue(COLOR_BG));
    uiManager->registerElement("soul_img", soul_img, IMAGE);
  
  Serial.println("UI元素初始化完成");
}

// 兼容函数 - 初始化LVGL界面（用于旧代码调用）
void init_ui() {
  initUI();
}