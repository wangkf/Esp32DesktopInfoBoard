#include "weather_display.h"
#include "net_http.h"
#include <lvgl.h>
#include <extra/libs/png/lv_png.h>
#include <extra/widgets/chart/lv_chart.h>
#include "weather_display.h"

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
    
    // 创建4天的天气图标显示区域
    int iconWidth = screenWidth / 4;
    int iconHeight = 100;
    
    for (int i = 0; i < 4; i++) {
        // 创建日期标签
        items[i].date_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(items[i].date_label, &lvgl_font_song_16, 0);
        lv_obj_set_style_text_color(items[i].date_label, lv_color_hex(0x333333), 0);
        lv_obj_align(items[i].date_label, LV_ALIGN_TOP_MID, 
                    (i - 1.5f) * iconWidth, 150);
        
        // 创建天气图标
        items[i].icon = lv_img_create(lv_scr_act());
        lv_obj_set_size(items[i].icon, 60, 60);
        lv_obj_align(items[i].icon, LV_ALIGN_TOP_MID, 
                    (i - 1.5f) * iconWidth, 180);
        
        // 创建最高温度标签
        items[i].high_temp_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(items[i].high_temp_label, &lvgl_font_song_16, 0);
        lv_obj_set_style_text_color(items[i].high_temp_label, lv_color_hex(0xFF6600), 0); // 橙色文字
        lv_obj_align(items[i].high_temp_label, LV_ALIGN_TOP_MID, 
                    (i - 1.5f) * iconWidth, 250);
        
        // 创建最低温度标签
        items[i].low_temp_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(items[i].low_temp_label, &lvgl_font_song_16, 0);
        lv_obj_set_style_text_color(items[i].low_temp_label, lv_color_hex(0x0066CC), 0); // 蓝色文字
        lv_obj_align(items[i].low_temp_label, LV_ALIGN_TOP_MID, 
                    (i - 1.5f) * iconWidth, 305);
        
        // 默认隐藏所有组件
        lv_obj_add_flag(items[i].date_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(items[i].icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(items[i].high_temp_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(items[i].low_temp_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 创建气温趋势图表，使其顶部挨着日期标签底部，宽度与屏幕一致
    chart.chart = lv_chart_create(lv_scr_act());
    lv_obj_set_size(chart.chart, screenWidth-80, 160); // 折线图宽度与屏幕宽度保持一致
    lv_obj_align(chart.chart, LV_ALIGN_TOP_MID, 0, 310); // 调整Y坐标使其顶部挨着日期标签底部
    
    // 配置图表样式
    lv_chart_set_type(chart.chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart.chart, LV_CHART_AXIS_PRIMARY_Y, -10, 40);
    
    // 禁用背景参考线和网格
    lv_chart_set_div_line_count(chart.chart, 0, 0);
    
    // 移除轴标签和刻度
    lv_obj_set_style_text_opa(chart.chart, 0, LV_PART_ANY);
    
    // 设置图表背景为透明
    lv_obj_set_style_bg_opa(chart.chart, 0, 0);
    
    // 设置图表边框为透明
    lv_obj_set_style_border_opa(chart.chart, 0, 0);
    
    // 启用节点显示
    lv_obj_set_style_size(chart.chart, 4, LV_PART_INDICATOR); // 设置节点大小
    
    // 创建最高温度和最低温度数据系列
    chart.high_temp_series = lv_chart_add_series(chart.chart, lv_color_hex(0xFF6600), LV_CHART_AXIS_PRIMARY_Y);
    chart.low_temp_series = lv_chart_add_series(chart.chart, lv_color_hex(0x0066CC), LV_CHART_AXIS_PRIMARY_Y);
    
    // 隐藏图例（左上方的色块）- 使用LVGL 8.3.7兼容的方法
    lv_obj_t * legend = lv_obj_get_child(chart.chart, -1);
    if (legend) {
      lv_obj_add_flag(legend, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 初始化温度数据数组
    for (int i = 0; i < 4; i++) {
        chart.temp_high_values[i] = 0;
        chart.temp_low_values[i] = 0;
    }
    
    // 默认隐藏图表
    lv_obj_add_flag(chart.chart, LV_OBJ_FLAG_HIDDEN);
    
    // 标记为已初始化
    initialized = true;
}

void WeatherDisplay::updateWeatherData(const char* weatherJson) {
    // 这里可以实现从JSON解析天气数据的逻辑
    // 目前这个功能已经在net_http.cpp中实现
}

void WeatherDisplay::show() {
    if (!initialized) {
        return;
    }
    
    // 隐藏原来的天气表格
    if (weather_table) {
        lv_obj_add_flag(weather_table, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 显示所有天气图标项目
    for (int i = 0; i < 4; i++) {
        lv_obj_clear_flag(items[i].date_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(items[i].icon, LV_OBJ_FLAG_HIDDEN);
        // 显示日期下方的温度标签
        lv_obj_clear_flag(items[i].high_temp_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(items[i].low_temp_label, LV_OBJ_FLAG_HIDDEN);
    }
    // 显示图表
    lv_obj_clear_flag(chart.chart, LV_OBJ_FLAG_HIDDEN);
}

void WeatherDisplay::hide() {
    if (!initialized) {
        return;
    }
    
    // 隐藏所有天气图标项目
    for (int i = 0; i < 4; i++) {
        lv_obj_add_flag(items[i].date_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(items[i].icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(items[i].high_temp_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(items[i].low_temp_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 隐藏图表
    lv_obj_add_flag(chart.chart, LV_OBJ_FLAG_HIDDEN);
}

bool WeatherDisplay::isInitialized() const {
    return initialized;
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
    
    // 重新设置图表数据点数量，这会自动清除之前的数据
    lv_chart_set_point_count(weatherDisplay.chart.chart, 4);
    
    // 使用lv_chart_set_next_value来设置数据，确保日期和折线正确对应
    for (int i = 0; i < 4; i++) {
        lv_chart_set_next_value(weatherDisplay.chart.chart, weatherDisplay.chart.high_temp_series, weatherDisplay.chart.temp_high_values[i]);
        lv_chart_set_next_value(weatherDisplay.chart.chart, weatherDisplay.chart.low_temp_series, weatherDisplay.chart.temp_low_values[i]);
    }
    
    // 刷新图表显示
    lv_chart_refresh(weatherDisplay.chart.chart);

    // 在图表上显示温度标签
    // 先删除之前的温度标签（如果存在）
    lv_obj_clean(weatherDisplay.chart.chart);
    
    // 重新添加数据系列（因为我们清除了图表）
    weatherDisplay.chart.high_temp_series = lv_chart_add_series(weatherDisplay.chart.chart, lv_color_hex(0xFF6600), LV_CHART_AXIS_PRIMARY_Y);
    weatherDisplay.chart.low_temp_series = lv_chart_add_series(weatherDisplay.chart.chart, lv_color_hex(0x0066CC), LV_CHART_AXIS_PRIMARY_Y);
    
    // 隐藏图例（左上方的色块）- 使用LVGL 8.3.7兼容的方法
    lv_obj_t * legend = lv_obj_get_child(weatherDisplay.chart.chart, -1);
    if (legend) {
      lv_obj_add_flag(legend, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 重新设置数据
    for (int i = 0; i < 4; i++) {
        lv_chart_set_next_value(weatherDisplay.chart.chart, weatherDisplay.chart.high_temp_series, weatherDisplay.chart.temp_high_values[i]);
        lv_chart_set_next_value(weatherDisplay.chart.chart, weatherDisplay.chart.low_temp_series, weatherDisplay.chart.temp_low_values[i]);
    }
    
    // 获取图表尺寸和位置信息来计算节点位置
    lv_coord_t x_ofs = lv_obj_get_x(weatherDisplay.chart.chart);
    lv_coord_t y_ofs = lv_obj_get_y(weatherDisplay.chart.chart);
    lv_coord_t w = lv_obj_get_width(weatherDisplay.chart.chart);
    
    // 手动计算每个点的位置并创建标签
    for (int i = 0; i < 4; i++) {
        // 计算X坐标（均匀分布在图表宽度上）
        lv_coord_t x = (w / 3) * i;
        
        // 计算Y坐标（根据温度值映射到图表高度）
        lv_coord_t chart_h = lv_obj_get_height(weatherDisplay.chart.chart);
        int temp_range = 40 - (-10); // 温度范围：-10℃到40℃
        
        // 最高温度Y坐标（温度越高，Y值越小，因为图表Y轴是从上到下递增）
        lv_coord_t high_y = chart_h - ((weatherDisplay.chart.temp_high_values[i] - (-10)) * chart_h / temp_range);
        
        // 最低温度Y坐标
        lv_coord_t low_y = chart_h - ((weatherDisplay.chart.temp_low_values[i] - (-10)) * chart_h / temp_range);
        
        // 创建最高温度标签，确保完整显示（使用中文字体）
        char high_temp_str[6];
        sprintf(high_temp_str, "%d℃", weatherDisplay.chart.temp_high_values[i]);
        lv_obj_t* high_temp_label = lv_label_create(weatherDisplay.chart.chart);
        lv_label_set_text(high_temp_label, high_temp_str);
        // 使用项目中可用的中文字体显示温度标签
        lv_obj_set_style_text_font(high_temp_label, &lvgl_font_song_16, 0);
        lv_obj_set_style_text_color(high_temp_label, lv_color_hex(0xFF6600), 0);
        // 设置标签宽度，确保完整显示温度值
        lv_obj_set_width(high_temp_label, 40);
        // 使用文本样式设置对齐方式
        lv_obj_set_style_text_align(high_temp_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(high_temp_label, LV_ALIGN_TOP_LEFT, x - 20, high_y - 25);
        
        // 创建最低温度标签，确保完整显示（使用中文字体）
        char low_temp_str[6];
        sprintf(low_temp_str, "%d℃", weatherDisplay.chart.temp_low_values[i]);
        lv_obj_t* low_temp_label = lv_label_create(weatherDisplay.chart.chart);
        lv_label_set_text(low_temp_label, low_temp_str);
        // 使用项目中可用的中文字体显示温度标签
        lv_obj_set_style_text_font(low_temp_label, &lvgl_font_song_16, 0);
        lv_obj_set_style_text_color(low_temp_label, lv_color_hex(0x0066CC), 0);
        // 设置标签宽度，确保完整显示温度值
        lv_obj_set_width(low_temp_label, 40);
        // 使用文本样式设置对齐方式
        lv_obj_set_style_text_align(low_temp_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(low_temp_label, LV_ALIGN_TOP_LEFT, x - 20, low_y + 5);
    }

    // 显示所有组件
    showWeatherDisplay();
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
    // 显示图表
    lv_obj_clear_flag(weatherDisplay.chart.chart, LV_OBJ_FLAG_HIDDEN);
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
    
    // 隐藏图表
    lv_obj_add_flag(weatherDisplay.chart.chart, LV_OBJ_FLAG_HIDDEN);
}