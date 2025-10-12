#ifndef WEATHER_DISPLAY_H
#define WEATHER_DISPLAY_H

#include <lvgl.h>
#include <ArduinoJson.h>
#include "config.h"
#include "weather_manager.h"

// 外部变量声明
extern const uint32_t screenWidth;
extern const uint32_t screenHeight;

extern lv_obj_t* weather_table;

extern bool TianXingAPIKEY_Flag;

/**
 * 天气显示类
 * 负责天气数据的可视化显示
 */
class WeatherDisplay {
private:
    bool initialized;         // 是否已初始化

public:
    WeatherIconItem items[4]; // 4天的天气图标项目
    WeatherChart chart;       // 气温趋势图表
    
    WeatherDisplay();
    ~WeatherDisplay();
    
    void initialize();
    void updateWeatherData(const char* weatherJson);
    void show();
    void hide();
    
    /**
     * 检查天气显示组件是否已初始化
     * @return 是否已初始化
     */
    bool isInitialized() const;

};

extern WeatherDisplay weatherDisplay;

// 函数声明

/**
 * 初始化天气显示组件
 */
void initWeatherDisplay();

/**
 * 清理天气显示组件
 */
void cleanupWeatherDisplay();

/**
 * 根据天气描述文本获取天气类型
 * @param weatherDesc 天气描述文本
 * @return 天气类型枚举值
 */
WeatherType getWeatherType(const char* weatherDesc);

/**
 * 获取天气图标路径
 * @param weatherType 天气类型
 * @return 图标路径字符串
 */
const char* getWeatherIconPath(WeatherType weatherType);

/**
 * 更新天气图标显示
 * @param dailyWeather 每日天气数据
 * @param index 索引（0-3）
 */
void updateWeatherIcon(JsonObject dailyWeather, int index);

/**
 * 更新气温趋势图表
 * @param dailyWeather 每日天气数据
 * @param index 索引（0-3）
 */
void updateWeatherChart(JsonObject dailyWeather, int index);

/**
 * 刷新完整的天气显示
 */
void refreshWeatherDisplay();

/**
 * 显示天气显示组件
 */
void showWeatherDisplay();

/**
 * 隐藏天气显示组件
 */
void hideWeatherDisplay();

#endif // WEATHER_DISPLAY_H