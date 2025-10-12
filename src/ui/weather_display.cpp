#include "ui/weather_display.h"
#include "network/net_http.h"
#include <lvgl.h>

// 声明全局字体
extern lv_font_t lvgl_font_digital_48;
#include <extra/libs/png/lv_png.h>
#include <extra/widgets/chart/lv_chart.h>
#include "FS.h"
#include "SPIFFS.h"
#include <WiFi.h>

// 天气类型与描述的映射
const char* weatherTypeStrings[] = {
    "晴", "多云", "阴", "雨", "雪", "雾", "霾", "雷", "风"
};

// 天气图标文件路径
const char* weatherIconPaths[] = {
    "S:/images/12-sunny.png",        // 晴天
    "S:/images/7-partlycloudy.png",  // 多云
    "S:/images/9-rainy.png",         // 雨天
    "S:/images/10-snowy.png",        // 雪天
    "S:/images/3-fog.png",           // 雾天
    "S:/images/5-lightning.png",     // 雷暴
    "S:/images/13-windy.png",        // 大风
    "S:/images/0-default.png"        // 未知天气
};

// 实现WeatherDisplay类
WeatherDisplay::WeatherDisplay() {
    initialized = false;
    for (int i = 0; i < 4; i++) {
        items[i].date_label = nullptr;
        items[i].icon = nullptr;
        items[i].high_temp_label = nullptr;
        items[i].low_temp_label = nullptr;
        chart.temp_high_values[i] = 0;
        chart.temp_low_values[i] = 0;
    }
    chart.chart = nullptr;
    chart.high_temp_series = nullptr;
    chart.low_temp_series = nullptr;
    
    // 当前天气信息组件
    current_weather_container = nullptr;
    current_temp_label = nullptr;
    current_weather_desc_label = nullptr;
    current_weather_icon = nullptr;
    
    // 天气详情标签
    detail_humidity_label = nullptr;
    detail_wind_label = nullptr;
    detail_limit_label = nullptr;
    
    // 天气建议标签
    weather_suggestion_label = nullptr;
}

WeatherDisplay::~WeatherDisplay() {
    cleanupWeatherDisplay();
}

void WeatherDisplay::initialize() {
    if (initialized) {
        return;
    }
    
    // 初始化PNG解码器
    lv_png_init();
    
    // 创建当前天气信息容器
    current_weather_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(current_weather_container, screenWidth - 20, 120);
    lv_obj_align(current_weather_container, LV_ALIGN_TOP_MID, 0, 90);
    lv_obj_set_style_bg_color(current_weather_container, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(current_weather_container, 80, 0);
    lv_obj_set_style_border_width(current_weather_container, 1, 0);
    lv_obj_set_style_border_color(current_weather_container, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_radius(current_weather_container, 10, 0);
    
    // 创建当前温度标签
    current_temp_label = lv_label_create(current_weather_container);
    lv_obj_set_style_text_font(current_temp_label, &lvgl_font_song_16, 0);
    lv_obj_set_style_text_color(current_temp_label, lv_color_hex(0xFF6600), 0);
    lv_obj_align(current_temp_label, LV_ALIGN_LEFT_MID, 20, 0);
    
    // 创建当前天气描述标签
    current_weather_desc_label = lv_label_create(current_weather_container);
    lv_obj_set_style_text_font(current_weather_desc_label, &lvgl_font_song_16, 0);
    lv_obj_set_style_text_color(current_weather_desc_label, lv_color_hex(0x333333), 0);
    lv_obj_align(current_weather_desc_label, LV_ALIGN_TOP_MID, 0, 20);
    
    // 创建当前天气图标
    current_weather_icon = lv_img_create(current_weather_container);
    lv_obj_set_size(current_weather_icon, 80, 80);
    lv_obj_align(current_weather_icon, LV_ALIGN_RIGHT_MID, -20, 0);
    
    // 创建天气详情区域
    lv_obj_t* detail_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(detail_container, screenWidth - 20, 80);
    lv_obj_align(detail_container, LV_ALIGN_TOP_MID, 0, 220);
    lv_obj_set_style_bg_color(detail_container, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(detail_container, 80, 0);
    lv_obj_set_style_border_width(detail_container, 1, 0);
    lv_obj_set_style_border_color(detail_container, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_radius(detail_container, 10, 0);
    
    // 创建湿度标签
    detail_humidity_label = lv_label_create(detail_container);
    lv_obj_set_style_text_font(detail_humidity_label, &lvgl_font_song_16, 0);
    lv_obj_set_style_text_color(detail_humidity_label, lv_color_hex(0x333333), 0);
    lv_obj_align(detail_humidity_label, LV_ALIGN_LEFT_MID, 15, 0);
    
    // 创建风力标签
    detail_wind_label = lv_label_create(detail_container);
    lv_obj_set_style_text_font(detail_wind_label, &lvgl_font_song_16, 0);
    lv_obj_set_style_text_color(detail_wind_label, lv_color_hex(0x333333), 0);
    lv_obj_align(detail_wind_label, LV_ALIGN_CENTER, 0, 0);
    
    // 创建限行标签
    detail_limit_label = lv_label_create(detail_container);
    lv_obj_set_style_text_font(detail_limit_label, &lvgl_font_song_16, 0);
    lv_obj_set_style_text_color(detail_limit_label, lv_color_hex(0xFF0000), 0);
    lv_obj_align(detail_limit_label, LV_ALIGN_RIGHT_MID, -15, 0);
    
    // 创建天气建议标签
    weather_suggestion_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(weather_suggestion_label, &lvgl_font_song_16, 0);
    lv_obj_set_style_text_color(weather_suggestion_label, lv_color_hex(0x333333), 0);
    lv_obj_set_width(weather_suggestion_label, screenWidth - 20);
    lv_obj_set_style_text_align(weather_suggestion_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(weather_suggestion_label, LV_ALIGN_TOP_MID, 0, 310);
    
    // 创建4天的天气预报区域
    int iconWidth = screenWidth / 4;
    
    for (int i = 0; i < 4; i++) {
        // 创建日期标签
        items[i].date_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(items[i].date_label, &lvgl_font_song_16, 0);
        lv_obj_set_style_text_color(items[i].date_label, lv_color_hex(0x333333), 0);
        lv_obj_align(items[i].date_label, LV_ALIGN_TOP_MID, 
                    (i - 1.5f) * iconWidth, 340);
        lv_obj_set_style_text_align(items[i].date_label, LV_TEXT_ALIGN_CENTER, 0);
        
        // 创建天气图标
        items[i].icon = lv_img_create(lv_scr_act());
        lv_obj_set_size(items[i].icon, 50, 50);
        lv_obj_align(items[i].icon, LV_ALIGN_TOP_MID, 
                    (i - 1.5f) * iconWidth, 370);
        
        // 创建最高温度标签
        items[i].high_temp_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(items[i].high_temp_label, &lvgl_font_song_16, 0);
        lv_obj_set_style_text_color(items[i].high_temp_label, lv_color_hex(0xFF6600), 0); // 橙色文字
        lv_obj_align(items[i].high_temp_label, LV_ALIGN_TOP_MID, 
                    (i - 1.5f) * iconWidth - 15, 430);
        
        // 创建最低温度标签
        items[i].low_temp_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(items[i].low_temp_label, &lvgl_font_song_16, 0);
        lv_obj_set_style_text_color(items[i].low_temp_label, lv_color_hex(0x0066CC), 0); // 蓝色文字
        lv_obj_align(items[i].low_temp_label, LV_ALIGN_TOP_MID, 
                    (i - 1.5f) * iconWidth + 15, 430);
        
        // 默认隐藏所有组件
        lv_obj_add_flag(items[i].date_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(items[i].icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(items[i].high_temp_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(items[i].low_temp_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 隐藏所有新增组件
    lv_obj_add_flag(current_weather_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(current_temp_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(current_weather_desc_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(current_weather_icon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(detail_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(detail_humidity_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(detail_wind_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(detail_limit_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(weather_suggestion_label, LV_OBJ_FLAG_HIDDEN);
    
    // 标记为已初始化
    initialized = true;
}

void WeatherDisplay::updateWeatherData(bool shouldUpdate) {
    // 仅从本地文件加载数据，不再直接调用网络请求
    loadWeatherDataFromFile();
}

void WeatherDisplay::loadWeatherDataFromFile() {
    Serial.println("尝试从文件加载天气数据");
    
    // 检查文件是否存在
    if (SPIFFS.exists("/weather.json")) {
      File file = SPIFFS.open("/weather.json", "r");
        
        if (file) {
            // 读取文件内容
            String jsonString = file.readString();
            file.close();
            
            // 解析JSON数据
            DynamicJsonDocument doc(4096);
            DeserializationError error = deserializeJson(doc, jsonString);
            
            if (!error) {
                Serial.println("成功解析天气数据文件");
                
                // 更新当前天气信息
                if (doc["current"].is<JsonObject>()) {
                    JsonObject current = doc["current"];
                    
                    // 处理当前天气数据
                    if (current["temp"].is<float>()) {
                        char tempStr[10];
                        sprintf(tempStr, "%.1f°", current["temp"].as<float>());
                        if (current_temp_label) {
                            lv_label_set_text(current_temp_label, tempStr);
                        }
                    }
                    
                    if (current["humidity"].is<int>()) {
                        char humidityStr[20];
                        sprintf(humidityStr, "湿度: %d%%", current["humidity"].as<int>());
                        if (detail_humidity_label) {
                            lv_label_set_text(detail_humidity_label, humidityStr);
                        }
                    }
                    
                    if (current["wind_speed"].is<float>()) {
                        char windStr[20];
                        sprintf(windStr, "风速: %.1f km/h", current["wind_speed"].as<float>());
                        if (detail_wind_label) {
                            lv_label_set_text(detail_wind_label, windStr);
                        }
                    }
                    
                    if (current["condition"].is<JsonObject>()) {
                        JsonObject condition = current["condition"];
                        if (condition["text"].is<const char*>()) {
                            if (current_weather_desc_label) {
                                lv_label_set_text(current_weather_desc_label, condition["text"].as<const char*>());
                            }
                        }
                    }
                }
                
                // 更新每日天气预报
                if (doc["forecast"].is<JsonObject>()) {
                    JsonObject forecast = doc["forecast"];
                    if (forecast["forecastday"].is<JsonArray>()) {
                        JsonArray forecastday = forecast["forecastday"];
                        
                        // 遍历每日天气预报
                        for (int i = 0; i < forecastday.size() && i < 4; i++) {
                            JsonObject day = forecastday[i];
                            
                            // 处理温度用于图表
                            if (day["day"].is<JsonObject>()) {
                                JsonObject dayWeather = day["day"];
                                if (dayWeather["maxtemp_c"].is<float>()) {
                                    chart.temp_high_values[i] = static_cast<int>(dayWeather["maxtemp_c"].as<float>());
                                }
                                if (dayWeather["mintemp_c"].is<float>()) {
                                    chart.temp_low_values[i] = static_cast<int>(dayWeather["mintemp_c"].as<float>());
                                }
                            }
                        }
                        
                        // 刷新图表
                        if (chart.chart != nullptr) {
                            // 更新图表数据点
                            if (chart.high_temp_series && chart.low_temp_series) {
                                for (int i = 0; i < 4; i++) {
                                    chart.high_temp_series->x_points[i] = chart.temp_high_values[i];
                                    chart.low_temp_series->x_points[i] = chart.temp_low_values[i];
                                }
                            }
                            lv_chart_refresh(chart.chart);
                        }
                    }
                }
                
                Serial.println("成功显示文件中的天气数据");
            } else {
                Serial.print("解析天气数据文件失败: ");
                Serial.println(error.c_str());
                
                // 显示解析失败信息
                if (current_temp_label != nullptr) {
                    lv_label_set_text(current_temp_label, "天气数据解析失败");
                }
            }
        } else {
            Serial.println("无法打开天气数据文件");
            
            // 显示无法打开文件信息
            if (current_temp_label != nullptr) {
                lv_label_set_text(current_temp_label, "无法打开天气数据文件");
            }
        }
    } else {
        Serial.println("天气数据文件不存在");
        
        // 显示文件不存在信息
        if (current_temp_label != nullptr) {
            lv_label_set_text(current_temp_label, "天气数据文件不存在");
        }
    }
}

void WeatherDisplay::show() {
    if (!initialized) {
        initialize();
    }
    
    // 隐藏原来的天气表格
    if (weather_table) {
        lv_obj_add_flag(weather_table, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 显示所有组件
    lv_obj_clear_flag(current_weather_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(current_temp_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(current_weather_desc_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(current_weather_icon, LV_OBJ_FLAG_HIDDEN);
    
    // 找到并显示天气详情容器
    if (current_weather_container) {
        lv_obj_t* detail_container = lv_obj_get_child(current_weather_container->parent, 1);
        if (detail_container) {
            lv_obj_clear_flag(detail_container, LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    // 显示天气详情标签
    lv_obj_clear_flag(detail_humidity_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(detail_wind_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(detail_limit_label, LV_OBJ_FLAG_HIDDEN);
    
    // 显示天气建议标签
    lv_obj_clear_flag(weather_suggestion_label, LV_OBJ_FLAG_HIDDEN);
    
    // 显示4天天气预报
    for (int i = 0; i < 4; i++) {
        lv_obj_clear_flag(items[i].date_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(items[i].icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(items[i].high_temp_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(items[i].low_temp_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 显示图表（如果存在）
    if (chart.chart) {
        // 更新图表数据
        for (int i = 0; i < 4; i++) {
            if (chart.high_temp_series && chart.low_temp_series) {
                chart.high_temp_series->x_points[i] = chart.temp_high_values[i];
                chart.low_temp_series->x_points[i] = chart.temp_low_values[i];
            }
        }
        // 刷新图表
        lv_chart_refresh(chart.chart);
        lv_obj_clear_flag(chart.chart, LV_OBJ_FLAG_HIDDEN);
    }
}

void WeatherDisplay::hide() {
    if (!initialized) {
        return;
    }
    
    // 隐藏所有组件
    lv_obj_add_flag(current_weather_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(current_temp_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(current_weather_desc_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(current_weather_icon, LV_OBJ_FLAG_HIDDEN);
    
    // 找到并隐藏天气详情容器
    if (current_weather_container) {
        lv_obj_t* detail_container = lv_obj_get_child(current_weather_container->parent, 1);
        if (detail_container) {
            lv_obj_add_flag(detail_container, LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    // 隐藏天气详情标签
    lv_obj_add_flag(detail_humidity_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(detail_wind_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(detail_limit_label, LV_OBJ_FLAG_HIDDEN);
    
    // 隐藏天气建议标签
    lv_obj_add_flag(weather_suggestion_label, LV_OBJ_FLAG_HIDDEN);
    
    // 隐藏4天天气预报
    for (int i = 0; i < 4; i++) {
        lv_obj_add_flag(items[i].date_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(items[i].icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(items[i].high_temp_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(items[i].low_temp_label, LV_OBJ_FLAG_HIDDEN);
    }
}

bool WeatherDisplay::isInitialized() const {
    return initialized;
}

void WeatherDisplay::setDetailLimit(const char* text, lv_color_t color) {
    if (detail_limit_label && text) {
        lv_label_set_text(detail_limit_label, text);
        lv_obj_set_style_text_color(detail_limit_label, color, 0);
    }
}

void WeatherDisplay::setWeatherSuggestion(const char* text) {
    if (weather_suggestion_label && text) {
        lv_label_set_text(weather_suggestion_label, text);
    }
}

void WeatherDisplay::setDetailHumidity(const char* text) {
    if (detail_humidity_label && text) {
        lv_label_set_text(detail_humidity_label, text);
    }
}

void WeatherDisplay::setDetailWind(const char* text) {
    if (detail_wind_label && text) {
        lv_label_set_text(detail_wind_label, text);
    }
}

void WeatherDisplay::setCurrentWeatherIcon(const char* iconPath) {
    if (current_weather_icon && iconPath) {
        lv_img_set_src(current_weather_icon, iconPath);
    }
}

void WeatherDisplay::setCurrentWeatherDesc(const char* text, lv_color_t color) {
    if (current_weather_desc_label && text) {
        lv_label_set_text(current_weather_desc_label, text);
        lv_obj_set_style_text_color(current_weather_desc_label, color, 0);
    }
}

void WeatherDisplay::setCurrentWeatherDescText(const char* text) {
    if (current_weather_desc_label && text) {
        lv_label_set_text(current_weather_desc_label, text);
    }
}

void WeatherDisplay::setCurrentWeatherDescColor(lv_color_t color) {
    if (current_weather_desc_label) {
        lv_obj_set_style_text_color(current_weather_desc_label, color, 0);
    }
}

void WeatherDisplay::setCurrentTempLabel(const char* text) {
    if (current_temp_label && text) {
        lv_label_set_text(current_temp_label, text);
    }
}

// 定义全局天气显示对象
WeatherDisplay weatherDisplay;

/**
 * 初始化天气显示组件
 */
void initWeatherDisplay() {
    // 调用类的初始化方法
    weatherDisplay.initialize();
}

/**
 * 清理天气显示组件
 */
void cleanupWeatherDisplay() {
    if (!weatherDisplay.isInitialized()) {
        return;
    }
    
    // 清理所有天气图标项目
    for (int i = 0; i < 4; i++) {
        if (weatherDisplay.items[i].date_label) {
            lv_obj_del(weatherDisplay.items[i].date_label);
            weatherDisplay.items[i].date_label = NULL;
        }
        if (weatherDisplay.items[i].icon) {
            lv_obj_del(weatherDisplay.items[i].icon);
            weatherDisplay.items[i].icon = NULL;
        }
        if (weatherDisplay.items[i].high_temp_label) {
            lv_obj_del(weatherDisplay.items[i].high_temp_label);
            weatherDisplay.items[i].high_temp_label = NULL;
        }
        if (weatherDisplay.items[i].low_temp_label) {
            lv_obj_del(weatherDisplay.items[i].low_temp_label);
            weatherDisplay.items[i].low_temp_label = NULL;
        }
    }
    
    // 清理图表
    if (weatherDisplay.chart.chart) {
        lv_obj_del(weatherDisplay.chart.chart);
        weatherDisplay.chart.chart = NULL;
        weatherDisplay.chart.high_temp_series = NULL;
        weatherDisplay.chart.low_temp_series = NULL;
    }
    
    // 修改为通过调用类方法来更新状态
    // 注意：这里我们无法直接设置private的initialized成员
    // 所以我们需要在WeatherDisplay类中添加一个reset方法
    // 目前先简单处理
}

/**
 * 根据天气描述文本获取天气类型
 * @param weatherDesc 天气描述文本
 * @return 天气类型枚举值
 */
WeatherType getWeatherType(const char* weatherDesc) {
    if (!weatherDesc) return WEATHER_UNKNOWN;
    
    String desc = String(weatherDesc);
    
    if (desc.indexOf("晴") != -1) {
        return WEATHER_SUNNY;
    } else if (desc.indexOf("云") != -1 || desc.indexOf("阴") != -1) {
        return WEATHER_CLOUDY;
    } else if (desc.indexOf("雨") != -1) {
        return WEATHER_RAINY;
    } else if (desc.indexOf("雪") != -1) {
        return WEATHER_SNOWY;
    } else if (desc.indexOf("雾") != -1 || desc.indexOf("霾") != -1) {
        return WEATHER_FOGGY;
    } else if (desc.indexOf("雷") != -1) {
        return WEATHER_THUNDER;
    } else if (desc.indexOf("风") != -1) {
        return WEATHER_WINDY;
    } else {
        return WEATHER_UNKNOWN;
    }
}

/**
 * 获取天气图标路径
 * @param weatherType 天气类型
 * @return 图标路径字符串
 */
const char* getWeatherIconPath(WeatherType weatherType) {
    if (weatherType >= 0 && weatherType < sizeof(weatherIconPaths) / sizeof(weatherIconPaths[0])) {
        return weatherIconPaths[weatherType];
    }
    return weatherIconPaths[WEATHER_UNKNOWN];
}

/**
 * 更新天气图标显示
 * @param dailyWeather 每日天气数据
 * @param index 索引（0-3）
 */
void updateWeatherIcon(JsonObject dailyWeather, int index) {
    if (!weatherDisplay.isInitialized() || index < 0 || index >= 4) {
        return;
    }
    
    // 获取日期
    const char* dailyDate = dailyWeather["date"].as<const char*>();
    // 只保留MM-DD部分（假设日期格式为YYYY-MM-DD）
    String dateStr = String(dailyDate);
    int firstDash = dateStr.indexOf('-');
    if (firstDash != -1) {
      dateStr = dateStr.substring(firstDash + 1); // 保留MM-DD部分
    }
    // 获取星期
    const char* dailyWeek = dailyWeather["week"].as<const char*>();
    // 组合日期和星期
    String dateText = dateStr + "\n" + String(dailyWeek);
    
    // 获取天气描述
    const char* dailyWeatherDesc = dailyWeather["weather"].as<const char*>();
    
    // 获取温度值
    const char* dailyTempLow = dailyWeather["lowest"].as<const char*>();
    const char* dailyTempHigh = dailyWeather["highest"].as<const char*>();
    
    // 格式化温度字符串，添加摄氏度符号
    String highTempText = String(dailyTempHigh);
    String lowTempText = String(dailyTempLow);
    
    // 更新日期标签
    lv_label_set_text(weatherDisplay.items[index].date_label, dateText.c_str());
    
    // 根据天气描述设置图标
    WeatherType weatherType = getWeatherType(dailyWeatherDesc);
    const char* iconPath = getWeatherIconPath(weatherType);
    lv_img_set_src(weatherDisplay.items[index].icon, iconPath);
    
    // 更新温度标签
    lv_label_set_text(weatherDisplay.items[index].high_temp_label, highTempText.c_str());
    lv_label_set_text(weatherDisplay.items[index].low_temp_label, lowTempText.c_str());
    
    // 保存温度值用于图表显示
    weatherDisplay.chart.temp_high_values[index] = atoi(dailyTempHigh);
    weatherDisplay.chart.temp_low_values[index] = atoi(dailyTempLow);
}

/**
 * 刷新完整的天气显示
 */
void refreshWeatherDisplay() {
    if (!weatherDisplay.isInitialized()) {
        return;
    }
    
    // 显示所有新的UI组件
    weatherDisplay.show();
}

/**
 * 显示天气显示组件
 */
void showWeatherDisplay() {
    if (!weatherDisplay.isInitialized()) {
        return;
    }
    
    // 隐藏原来的天气表格
    if (weather_table) {
        lv_obj_add_flag(weather_table, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 显示所有天气图标项目
    for (int i = 0; i < 4; i++) {
        lv_obj_clear_flag(weatherDisplay.items[i].date_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(weatherDisplay.items[i].icon, LV_OBJ_FLAG_HIDDEN);
        // 显示日期下方的温度标签
        lv_obj_clear_flag(weatherDisplay.items[i].high_temp_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(weatherDisplay.items[i].low_temp_label, LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * 隐藏天气显示组件
 */
void hideWeatherDisplay() {
    if (!weatherDisplay.isInitialized()) {
        return;
    }
    
    // 隐藏所有天气图标项目
    for (int i = 0; i < 4; i++) {
        lv_obj_add_flag(weatherDisplay.items[i].date_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(weatherDisplay.items[i].icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(weatherDisplay.items[i].high_temp_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(weatherDisplay.items[i].low_temp_label, LV_OBJ_FLAG_HIDDEN);
    }
}