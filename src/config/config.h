#ifndef CONFIG_H
#define CONFIG_H

// 引入必要的头文件
#include <Arduino.h>

// WiFi配置信息已移至ConfigManager类中管理
// 从data/config.json文件中读取

// NTP服务器配置
#define NTP_SERVER     "ntp.aliyun.com"

// 屏幕状态枚举
enum ScreenState {
  MAO_SELECT_SCREEN = 1,
  TOXIC_SOUL_SCREEN,
  ICIBA_SCREEN,
  ASTRONAUTS_SCREEN
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

// 自动换屏设置（暂未使用）
// const unsigned long AUTO_SCREEN_SWITCH_INTERVAL = 30000; // 30秒，单位毫秒
extern const char* ntpServer; // NTP服务器地址（声明）

// 刷新间隔配置（暂未使用）
// const long updateInterval = 1000; // 更新间隔（毫秒）
// #define DATA_CACHE_INTERVAL 7200000 // 2小时 = 7200000毫秒

// 光线传感器配置（暂未使用）
// const int DARK_THRESHOLD = 2000; // 降低敏感度，增大阈值
// const int VERY_DARK_THRESHOLD = 500; // 几乎无光线阈值
const long brightnessUpdateInterval = 200; // 亮度更新间隔（毫秒）

// 声明全局变量
extern const char* MaoSelect[];
extern const int MaoSelectCount;
extern const char* ToxicSoul[];
extern const int ToxicSoulCount;

// LVGL字体配置
// 注意：这些字体声明应在包含lvgl.h后使用
// 实际的字体定义在其他文件中
#endif // CONFIG_H