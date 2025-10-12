#ifndef CONFIG_H
#define CONFIG_H

// 引入必要的头文件
#include <Arduino.h>

// 天气数据结构体定义
typedef struct {
  char city[30];          // 城市名称
  char date[10];          // 日期
  char time[10];          // 时间
  int Weather_Tmp_Real;   // 实时温度
  int Weather_Tmp_High;   // 最高温度
  int Weather_Tmp_Low;    // 最低温度
  char Wind_Direction[30];// 风向
  char Wind_Power[20];    // 风力
  char humidity[10];      // 湿度
  char ys[60];            // 预警信息
  char fs[60];            // 防晒信息
  char limitnumber[10];   // 限行信息
  char Weather_Type[30];  // 天气类型描述
  int Weather_TypeCode;   // 天气类型代码
  char citycode[20];      // 城市代码
  int day_code[5][2];     // 白天天气代码
  int night_code[5][2];   // 夜晚天气代码
  int tmp_max_data[5][2]; // 最高温度数据
  int tmp_min_data[5][2]; // 最低温度数据
  char Weather_data[5][20]; // 日期数据
} epd_rtc_data;

extern epd_rtc_data epd_rtc;

// WiFi配置信息
#define WIFI_SSID      "wonder"      // 请修改为您的WiFi名称
#define WIFI_PASSWORD  "unclesam"  // 请修改为您的WiFi密码

// NTP服务器配置
#define NTP_SERVER     "ntp.aliyun.com"
#define NTP_TIMEZONE   8  // 北京时间，东八区

// API密钥配置
#define API_KEY        "38c43566fe20217eab8108f8243b5a89" // 天行数据API密钥

// 屏幕状态枚举
enum ScreenState {
  WEATHER_SCREEN,
  NEWS_SCREEN,
  MAO_SELECT_SCREEN,
  TOXIC_SOUL_SCREEN,
  ICBA_SCREEN,
  ASTRONAUTS_SCREEN,
  APRS_SCREEN
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

// 自动换屏设置
const unsigned long AUTO_SCREEN_SWITCH_INTERVAL = 30000; // 30秒，单位毫秒

// NTP相关配置
const int NTP_PACKET_SIZE = 48; // NTP时间戳位于消息的前48字节
const long gmtOffset_sec = NTP_TIMEZONE * 3600; // GMT偏移量（秒）
const int daylightOffset_sec = 0; // 夏令时偏移量（秒）
extern const char* ntpServer; // NTP服务器地址（声明）

// 刷新间隔配置
const long updateInterval = 1000; // 更新间隔（毫秒）
#define DATA_CACHE_INTERVAL 7200000 // 2小时 = 7200000毫秒

// 光线传感器配置
const int DARK_THRESHOLD = 2000; // 降低敏感度，增大阈值
const int VERY_DARK_THRESHOLD = 500; // 几乎无光线阈值
const long brightnessUpdateInterval = 200; // 亮度更新间隔（毫秒）

// 声明全局变量
extern String cityCode; // 城市代码
extern const char* MaoSelect[];
extern const int MaoSelectCount;
extern const char* ToxicSoul[];
extern const int ToxicSoulCount;

// LVGL字体配置
// 注意：这些字体声明应在包含lvgl.h后使用
// 实际的字体定义在其他文件中

// APRS配置
#define APRS_CALLSIGN "BI9ABS"    // APRS呼号
#define APRS_PASSCODE "19129"    // APRS密码
#define APRS_SERVER "asia.aprs2.net"  // APRS服务器
#define APRS_PORT 14580           // APRS端口
#define APRS_RANGE_KM 10          // 搜索范围(公里)
#define APRS_MAX_PACKETS 6       // 最多显示的数据包数量

#endif // CONFIG_H