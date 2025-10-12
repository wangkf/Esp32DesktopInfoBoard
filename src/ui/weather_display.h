#ifndef WEATHER_DISPLAY_H
#define WEATHER_DISPLAY_H

#include <lvgl.h>
#include <ArduinoJson.h>
#include "config/config.h"
#include "ui/weather_manager.h"

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
    
    // 当前天气信息显示组件
    lv_obj_t* current_weather_container; // 当前天气信息容器
    lv_obj_t* current_temp_label;        // 当前温度标签
    lv_obj_t* current_weather_desc_label; // 当前天气描述标签
    lv_obj_t* current_weather_icon;      // 当前天气图标
    
    // 天气详情标签
    lv_obj_t* detail_humidity_label;     // 湿度标签
    lv_obj_t* detail_wind_label;         // 风力标签
    lv_obj_t* detail_limit_label;        // 限行标签
    
    // 天气建议标签
    lv_obj_t* weather_suggestion_label;

public:
    WeatherIconItem items[4]; // 4天的天气图标项目
    WeatherChart chart;       // 气温趋势图表
    
    WeatherDisplay();
    ~WeatherDisplay();
    
    void initialize();
    void updateWeatherData(const char* weatherJson);
    void updateWeatherData(bool shouldUpdate);
    void loadWeatherDataFromFile();
    void show();
    void hide();
    
    /**
     * 检查天气显示组件是否已初始化
     * @return 是否已初始化
     */
    bool isInitialized() const;
    
    /**
     * 设置限行信息
     * @param text 限行文本内容
     * @param color 文本颜色，默认为红色
     */
    void setDetailLimit(const char* text, lv_color_t color = lv_color_hex(0xFF0000));
    
    /**
     * 设置天气建议信息
     * @param text 天气建议文本内容
     */
    void setWeatherSuggestion(const char* text);
    
    /**
     * 设置湿度信息
     * @param text 湿度文本内容
     */
    void setDetailHumidity(const char* text);
    
    /**
     * 设置风力信息
     * @param text 风力文本内容
     */
    void setDetailWind(const char* text);
    
    /**
     * 设置当前天气图标
     * @param iconPath 图标路径
     */
    void setCurrentWeatherIcon(const char* iconPath);
    
    /**
     * 设置当前天气描述
     * @param text 天气描述文本
     * @param color 文本颜色
     */
    void setCurrentWeatherDesc(const char* text, lv_color_t color);
    
    /**
     * 设置当前天气描述文本
     * @param text 天气描述文本
     */
    void setCurrentWeatherDescText(const char* text);
    
    /**
     * 设置当前天气描述文本颜色
     * @param color 文本颜色
     */
    void setCurrentWeatherDescColor(lv_color_t color);
    
    /**
     * 设置当前温度标签文本
     * @param text 温度文本内容
     */
    void setCurrentTempLabel(const char* text);

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