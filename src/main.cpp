#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <OneButton.h>
#include <ArduinoJson.h>
#include <esp32-hal-ledc.h>

// 配置文件
#include "config.h"

// 自定义库
#include "screen_manager.h"
#include "time_manager.h"
#include "data_manager.h"
#include "weather_manager.h"
#include "button_manager.h"
#include "net_http.h"
#include "weather_display.h"

// LED控制配置
#define LED_CHANNEL    0       // LED通道
#define LED_FREQ       5000    // LED频率
#define LED_RESOLUTION 8       // LED分辨率
#define LED_PIN        22      // LED引脚

// 屏幕尺寸 - 使用config.h中定义的变量
// #define SCREEN_WIDTH   320     // 屏幕宽度
// #define SCREEN_HEIGHT  480     // 屏幕高度
// 注意：屏幕宽度和高度已在config.h中定义为screenWidth和screenHeight

// 字体定义（从lv_conf.h中引用）
extern const lv_font_t lvgl_font_song_16;

// LVGL相关全局变量
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * screenHeight / 10 ];

// 全局对象声明
TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight);

// 外部变量声明
lv_obj_t* weather_table;
lv_obj_t* weather_label;
lv_obj_t* news_label;
// zhu_quote_label 和 mao_select_label 合并为一个变量
lv_obj_t* mao_select_label;
// chicken_soup_label 和 toxic_soul_label 合并为一个变量
lv_obj_t* toxic_soul_label;
lv_obj_t* iciba_label;
lv_obj_t* astronauts_label;
lv_obj_t* aprs_label;
lv_obj_t* brightness_bar;
lv_obj_t* aprs_table;

// 亮度调节
int brightness_value = 255; // 亮度值
int sensor_value = 0;       // 光线传感器值

// 自动换屏定时器
unsigned long lastScreenChangeTime = 0;
bool autoScreenChangeEnabled = true;

// 系统初始化标志
bool systemInitialized = false;

/**
 * 显示刷新回调函数
 */
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( (uint16_t*)&color_p->full, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp );
}

/**
 * 初始化硬件
 */
void initHardware() {
    // 初始化串口通信
    Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }
    
    // 初始化PSRAM
    if (psramInit()) {
        Serial.println("PSRAM initialized");
    } else {
        Serial.println("PSRAM initialization failed");
    }
    
    // 初始化TFT屏幕
tft.begin();
tft.setRotation(0); // 设置屏幕旋转方向（0为正常方向）
tft.fillScreen(TFT_BLACK); // 使用黑色背景以便文本显示清晰

// 初始化LVGL
lv_init();

// 初始化显示缓冲区
lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 10);

// 注册显示设备驱动
static lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.hor_res = screenWidth;
disp_drv.ver_res = screenHeight;
disp_drv.flush_cb = my_disp_flush;
disp_drv.draw_buf = &draw_buf;
lv_disp_drv_register(&disp_drv);
    
    // 初始化光线传感器引脚
    pinMode(LIGHT_SENSOR_PIN, INPUT);
    
    // 初始化PWM引脚用于亮度控制
    ledcSetup(LED_CHANNEL, LED_FREQ, LED_RESOLUTION);
    ledcAttachPin(LED_PIN, LED_CHANNEL);
    ledcWrite(LED_CHANNEL, brightness_value);
}

/**
 * 初始化UI元素
 */
void initUI() {
    
    // 创建天气表格
    weather_table = lv_table_create(lv_scr_act());
    lv_table_set_col_cnt(weather_table, 3);
    lv_table_set_row_cnt(weather_table, 5);
    // 设置列宽
    lv_table_set_col_width(weather_table, 0, 80);
    lv_table_set_col_width(weather_table, 1, 120);
    lv_table_set_col_width(weather_table, 2, 120);
    lv_obj_align(weather_table, LV_ALIGN_TOP_MID, 0, 100); // 顶部离屏幕顶部100px
    lv_obj_set_style_radius(weather_table, 10, 0); // 设置圆角
    lv_obj_set_style_bg_color(weather_table, lv_color_hex(0xFFFFFF), 0); // 白色背景
    lv_obj_set_style_bg_opa(weather_table, 100, 0); // 设置背景透明度
    
    // 设置表格样式
    lv_obj_set_style_text_font(weather_table, &lvgl_font_song_16, 0);
    
    // 不默认隐藏，让screen_manager控制显示/隐藏
    
    // 创建新闻标签
news_label = lv_label_create(lv_scr_act());
lv_label_set_text(news_label, "加载中...");
lv_obj_set_width(news_label, screenWidth - 20);
lv_obj_set_height(news_label, screenHeight - 120); // 固定高度，顶部100px，留出20px边距
lv_label_set_long_mode(news_label, LV_LABEL_LONG_WRAP);
lv_obj_set_style_text_font(news_label, &lvgl_font_song_16, 0);
lv_obj_align(news_label, LV_ALIGN_TOP_LEFT, 10, 100); // 顶部离屏幕顶部100px
lv_obj_set_style_radius(news_label, 10, 0); // 设置圆角
lv_obj_set_style_bg_color(news_label, lv_color_hex(0xFFFFFF), 0); // 白色背景
lv_obj_set_style_bg_opa(news_label, 100, 0); // 设置背景透明度
    
    // 不默认隐藏，让screen_manager控制显示/隐藏
    
    // 创建毛主席语录标签
    mao_select_label = lv_label_create(lv_scr_act());
    lv_label_set_text(mao_select_label, "加载中...");
    lv_obj_set_width(mao_select_label, screenWidth - 20);
    lv_obj_set_height(mao_select_label, screenHeight - 120); // 固定高度，顶部100px，留出20px边距
    lv_label_set_long_mode(mao_select_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(mao_select_label, &lvgl_font_song_16, 0);
    lv_obj_align(mao_select_label, LV_ALIGN_TOP_LEFT, 10, 100); // 顶部离屏幕顶部100px
    lv_obj_set_style_radius(mao_select_label, 10, 0); // 设置圆角
    lv_obj_set_style_bg_color(mao_select_label, lv_color_hex(0xFFFFFF), 0); // 白色背景
    lv_obj_set_style_bg_opa(mao_select_label, 100, 0); // 设置背景透明度
    
    // 不默认隐藏，让screen_manager控制显示/隐藏
    
    // 创建心灵鸡汤标签
    toxic_soul_label = lv_label_create(lv_scr_act());
    lv_label_set_text(toxic_soul_label, "加载中...");
    lv_obj_set_width(toxic_soul_label, screenWidth - 20);
    lv_obj_set_height(toxic_soul_label, screenHeight - 120); // 固定高度，顶部100px，留出20px边距
    lv_label_set_long_mode(toxic_soul_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(toxic_soul_label, &lvgl_font_song_16, 0);
    lv_obj_align(toxic_soul_label, LV_ALIGN_TOP_LEFT, 10, 100); // 顶部离屏幕顶部100px
    lv_obj_set_style_radius(toxic_soul_label, 10, 0); // 设置圆角
    lv_obj_set_style_bg_color(toxic_soul_label, lv_color_hex(0xFFFFFF), 0); // 白色背景
    lv_obj_set_style_bg_opa(toxic_soul_label, 100, 0); // 设置背景透明度
    
    // 不默认隐藏，让screen_manager控制显示/隐藏
    
    // 创建ICBA标签
    iciba_label = lv_label_create(lv_scr_act());
    lv_label_set_text(iciba_label, "加载中...");
    lv_obj_set_width(iciba_label, screenWidth - 20);
    lv_obj_set_height(iciba_label, screenHeight - 120); // 固定高度，顶部100px，留出20px边距
    lv_label_set_long_mode(iciba_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(iciba_label, &lvgl_font_song_16, 0);
    lv_obj_align(iciba_label, LV_ALIGN_TOP_LEFT, 10, 100); // 顶部离屏幕顶部100px
    lv_obj_set_style_radius(iciba_label, 10, 0); // 设置圆角
    lv_obj_set_style_bg_color(iciba_label, lv_color_hex(0xFFFFFF), 0); // 白色背景
    lv_obj_set_style_bg_opa(iciba_label, 100, 0); // 设置背景透明度
    
    // 不默认隐藏，让screen_manager控制显示/隐藏
    
    // 创建宇航员标签
    astronauts_label = lv_label_create(lv_scr_act());
    lv_label_set_text(astronauts_label, "加载中...");
    lv_obj_set_width(astronauts_label, screenWidth - 20);
    lv_obj_set_height(astronauts_label, screenHeight - 120); // 固定高度，顶部100px，留出20px边距
    lv_label_set_long_mode(astronauts_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(astronauts_label, &lvgl_font_song_16, 0);
    lv_obj_align(astronauts_label, LV_ALIGN_TOP_LEFT, 10, 100); // 顶部离屏幕顶部100px
    lv_obj_set_style_radius(astronauts_label, 10, 0); // 设置圆角
    lv_obj_set_style_bg_color(astronauts_label, lv_color_hex(0xFFFFFF), 0); // 白色背景
    lv_obj_set_style_bg_opa(astronauts_label, 100, 0); // 设置背景透明度
    
    // 不默认隐藏，让screen_manager控制显示/隐藏
    
    // 创建APRS标签
    aprs_label = lv_label_create(lv_scr_act());
    lv_label_set_text(aprs_label, "加载中...");
    lv_obj_set_width(aprs_label, screenWidth - 20);
    lv_obj_set_height(aprs_label, screenHeight - 120); // 固定高度，顶部100px，留出20px边距
    lv_label_set_long_mode(aprs_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(aprs_label, &lvgl_font_song_16, 0);
    lv_obj_align(aprs_label, LV_ALIGN_TOP_LEFT, 10, 100); // 顶部离屏幕顶部100px
    lv_obj_set_style_radius(aprs_label, 10, 0); // 设置圆角
    lv_obj_set_style_bg_color(aprs_label, lv_color_hex(0xFFFFFF), 0); // 白色背景
    lv_obj_set_style_bg_opa(aprs_label, 100, 0); // 设置背景透明度
    
    // 不默认隐藏，让screen_manager控制显示/隐藏
}

/**
 * 初始化WiFi和NTP
 */
void initWiFiAndNTP() {
    // 连接WiFi
    setupWiFi();
    
    // 设置NTP服务器
    configTime(8 * 3600, 0, NTP_SERVER, NTP_SERVER);
}

/**
 * 更新亮度
 */
void updateBrightness() {
    // 读取光线传感器值
    sensor_value = analogRead(LIGHT_SENSOR_PIN);
    
    // 将传感器值映射到亮度值（0-255）
    // 注意：传感器值越高表示环境越亮，我们需要根据环境亮度调整屏幕亮度
    brightness_value = map(sensor_value, 0, 4095, 255, 50); // 环境越暗，屏幕越亮；环境越亮，屏幕越暗
    
    // 限制亮度值范围
    brightness_value = constrain(brightness_value, 50, 255);
    
    // 设置屏幕亮度
    ledcWrite(LED_CHANNEL, brightness_value);
}

/**
 * 处理按钮事件
 */
void handleButtonEvents() {
    // 使用按钮管理器检查按钮事件
    ButtonEvent event = ButtonManager::getInstance()->check();
    
    switch (event) {
        case SHORT_PRESS:
            // 短按：切换屏幕
            ScreenManager::getInstance()->toggleScreen();
            lastScreenChangeTime = millis(); // 重置自动换屏计时器
            break;
            
        case DOUBLE_CLICK:
            // 双击：刷新数据
            DataManager::getInstance()->forceRefreshAllData();
            ScreenManager::getInstance()->refreshCurrentScreenData();
            break;
            
        case TRIPLE_CLICK:
            // 三击：切换自动换屏功能
            autoScreenChangeEnabled = !autoScreenChangeEnabled;
            break;
            
        case LONG_PRESS:
            // 长按：特殊功能（例如：打开设置菜单等）
            break;
            
        default:
            break;
    }
}

/**
 * 处理自动换屏
 */
void handleAutoScreenChange() {
    if (autoScreenChangeEnabled && 
        millis() - lastScreenChangeTime >= AUTO_SCREEN_SWITCH_INTERVAL) {
        // 自动切换到下一个屏幕
        ScreenManager::getInstance()->toggleScreen();
        lastScreenChangeTime = millis();
    }
}

/**
 * 初始化系统
 */
void initSystem() {
    if (systemInitialized) {
        return;
    }
    
    // 初始化硬件
    initHardware();
    
    // 初始化UI元素
    initUI();
    
    // 初始化WiFi和NTP
    initWiFiAndNTP();
    
    // 初始化所有管理器
    ScreenManager::getInstance()->init();
    TimeManager::getInstance()->init();
    DataManager::getInstance()->init();
    WeatherManager::getInstance()->init();
    ButtonManager::getInstance()->begin();
    
    // 设置天气管理器的天气表格引用
    WeatherManager::getInstance()->setWeatherTable(weather_table);
    
    // 标记系统已初始化
    systemInitialized = true;
    
    Serial.println("系统初始化完成");
    
    // 初始化完成后显示第一个屏幕（天气屏幕）
    ScreenManager::getInstance()->switchToScreen(WEATHER_SCREEN);
}

/**
 * 设置函数
 */
void setup() {
    // 初始化系统
    initSystem();
}

/**
 * 循环函数
 */
void loop() {
    // 确保系统已初始化
    if (!systemInitialized) {
        initSystem();
    }
    
    // 更新时间显示
    TimeManager::getInstance()->updateTimeDisplay();
    
    // 处理按钮事件
    handleButtonEvents();
    
    // 处理自动换屏
    handleAutoScreenChange();
    
    // 更新亮度
    updateBrightness();
    
    // LVGL处理
    lv_task_handler();
    
    // 短暂延迟
    delay(10);
}