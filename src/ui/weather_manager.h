#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include <lvgl.h>
#include <ArduinoJson.h>
#include "config/config.h"

// 天气图标类型定义
enum WeatherType {
    WEATHER_SUNNY = 0,
    WEATHER_CLOUDY,
    WEATHER_RAINY,
    WEATHER_SNOWY,
    WEATHER_FOGGY,
    WEATHER_THUNDER,
    WEATHER_WINDY,
    WEATHER_UNKNOWN
};

// 天气图标组件结构体
typedef struct {
    lv_obj_t* icon;         // 天气图标
    lv_obj_t* date_label;   // 日期标签
    lv_obj_t* high_temp_label; // 最高温度标签
    lv_obj_t* low_temp_label;  // 最低温度标签
} WeatherIconItem;

// 天气图表组件结构体
typedef struct {
    lv_obj_t* chart;        // 图表控件
    lv_chart_series_t* high_temp_series;  // 最高温度系列
    lv_chart_series_t* low_temp_series;   // 最低温度系列
    int temp_high_values[4]; // 最高温度值
    int temp_low_values[4];  // 最低温度值
} WeatherChart;

/**
 * 天气管理器类
 * 负责管理天气数据的显示和更新
 */
class WeatherManager {
private:
    static WeatherManager* instance; // 单例实例
    
    WeatherIconItem items[4]; // 4天的天气图标项目
    WeatherChart chart;       // 气温趋势图表
    bool initialized;         // 是否已初始化
    
    // 天气类型与描述的映射
    const char* weatherTypeStrings[8];
    
    // 天气图标文件路径
    const char* weatherIconPaths[8];
    
    // 外部天气表格对象（暂时保留，逐步迁移到类内部）
    lv_obj_t* weather_table;
    
    // 私有构造函数（单例模式）
    WeatherManager();
    
    // 根据天气描述文本获取天气类型
    WeatherType getWeatherType(const char* weatherDesc);
    
    // 获取天气图标路径
    const char* getWeatherIconPath(WeatherType weatherType);
    
    // 更新天气图标显示
    void updateWeatherIcon(JsonObject dailyWeather, int index);
    
    // 更新气温趋势图表数据点
    void updateWeatherChartData();
    
public:
    // 获取单例实例
    static WeatherManager* getInstance();
    
    // 初始化天气显示组件
    void init();
    
    // 清理天气显示组件
    void cleanup();
    
    // 刷新完整的天气显示
    void refreshDisplay();
    
    // 显示天气显示组件
    void show();
    
    // 隐藏天气显示组件
    void hide();
    
    // 设置天气表格对象
    void setWeatherTable(lv_obj_t* table);
    
    // 从JSON数据更新天气信息
    bool updateWeatherData(const String& jsonData);
};

#endif // WEATHER_MANAGER_H