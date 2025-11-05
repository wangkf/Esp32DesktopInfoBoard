#ifndef CONFIG_H
#define CONFIG_H
// 引入必要的头文件
#include <Arduino.h>
#include "lvgl.h"
// 声明全局字体
extern const lv_font_t lvgl_font_digital_24;
extern const lv_font_t lvgl_font_digital_48;
extern const lv_font_t lvgl_font_digital_108;
extern const lv_font_t lvgl_font_digital_64;
// 字体定义 - 方便后续更换字体
extern const lv_font_t lvgl_font_yahei_16;
#define GBFont &lvgl_font_yahei_16
// 软件版本定义
#define SOFTWARE_VERSION "0.1.6"
// NTP服务器配置
#define NTP_SERVER     "ntp.aliyun.com"
// 屏幕状态枚举
enum ScreenState {
  NEWS_SCREEN = 1,
  CALENDAR_SCREEN,
  MAO_SELECT_SCREEN,
  TOXIC_SOUL_SCREEN,
  ICIBA_SCREEN,
  ASTRONAUTS_SCREEN,
  SOUL_SCREEN,
  NOTE_SCREEN
};

// 主题ID枚举
enum ThemeID {
  THEME_LIGHT = 0,    // 浅色主题
  THEME_DARK = 1,     // 深色主题
  THEME_SPECIAL = 2   // 特殊主题（预留，可根据需要扩展）
};
// 按钮事件类型
enum ButtonEvent {
  NONE,
  SHORT_PRESS,
  DOUBLE_CLICK,
  TRIPLE_CLICK,
  LONG_PRESS
};
// 硬件引脚配置
const int BUTTON_PIN = 18; // 按键连接到引脚18
const int LIGHT_SENSOR_PIN = 35; // 光线传感器引脚
const int SCREEN_BRIGHTNESS_PIN = 22; // 屏幕亮度控制引脚
// 屏幕配置
const uint32_t screenWidth = 320;
const uint32_t screenHeight = 480;
// 按钮相关配置
const unsigned long DEBOUNCE_DELAY = 50; // 消抖延迟(毫秒)
const unsigned long SHORT_PRESS_THRESHOLD = 200; // 短按阈值(毫秒)
const unsigned long LONG_PRESS_THRESHOLD = 1000; // 长按阈值(毫秒)
const unsigned long MULTI_CLICK_THRESHOLD = 300; // 多击检测时间窗口(毫秒)
extern const char* ntpServer; // NTP服务器地址（声明）
const long updateInterval = 15; // 更新间隔（秒）
const long brightnessUpdateInterval = 200; // 亮度更新间隔（毫秒）
// 声明全局变量
extern const char* MaoSelect[];
extern const int MaoSelectCount;
extern const char* ToxicSoul[];
extern const int ToxicSoulCount;
#endif // CONFIG_H