#include "ui/weather_manager.h"
#include <extra/libs/png/lv_png.h>
#include <extra/widgets/chart/lv_chart.h>

// 定义单例实例
WeatherManager* WeatherManager::instance = nullptr;

/**
 * 私有构造函数
 */
WeatherManager::WeatherManager() : initialized(false), weather_table(nullptr) {
    // 初始化天气类型与描述的映射
    weatherTypeStrings[WEATHER_SUNNY] = "晴";
    weatherTypeStrings[WEATHER_CLOUDY] = "多云";
    weatherTypeStrings[WEATHER_RAINY] = "雨";
    weatherTypeStrings[WEATHER_SNOWY] = "雪";
    weatherTypeStrings[WEATHER_FOGGY] = "雾";
    weatherTypeStrings[WEATHER_THUNDER] = "雷";
    weatherTypeStrings[WEATHER_WINDY] = "风";
    weatherTypeStrings[WEATHER_UNKNOWN] = "未知";
    
    // 初始化天气图标文件路径
    weatherIconPaths[WEATHER_SUNNY] = "S:/images/12-sunny.png";
    weatherIconPaths[WEATHER_CLOUDY] = "S:/images/7-partlycloudy.png";
    weatherIconPaths[WEATHER_RAINY] = "S:/images/9-rainy.png";
    weatherIconPaths[WEATHER_SNOWY] = "S:/images/10-snowy.png";
    weatherIconPaths[WEATHER_FOGGY] = "S:/images/3-fog.png";
    weatherIconPaths[WEATHER_THUNDER] = "S:/images/5-lightning.png";
    weatherIconPaths[WEATHER_WINDY] = "S:/images/13-windy.png";
    weatherIconPaths[WEATHER_UNKNOWN] = "S:/images/0-default.png";
    
    // 初始化温度数据数组
    for (int i = 0; i < 4; i++) {
        chart.temp_high_values[i] = 0;
        chart.temp_low_values[i] = 0;
    }
}

/**
 * 获取单例实例
 */
WeatherManager* WeatherManager::getInstance() {
    if (instance == nullptr) {
        instance = new WeatherManager();
    }
    return instance;
}

/**
 * 初始化天气显示组件
 */
void WeatherManager::init() {
    // 防止重复初始化
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
    
    // 默认隐藏图表
    lv_obj_add_flag(chart.chart, LV_OBJ_FLAG_HIDDEN);
    
    // 标记为已初始化
    initialized = true;
}

/**
 * 清理天气显示组件
 */
void WeatherManager::cleanup() {
    if (!initialized) {
        return;
    }
    
    // 清理所有天气图标项目
    for (int i = 0; i < 4; i++) {
        if (items[i].date_label) {
            lv_obj_del(items[i].date_label);
            items[i].date_label = NULL;
        }
        if (items[i].icon) {
            lv_obj_del(items[i].icon);
            items[i].icon = NULL;
        }
        if (items[i].high_temp_label) {
            lv_obj_del(items[i].high_temp_label);
            items[i].high_temp_label = NULL;
        }
        if (items[i].low_temp_label) {
            lv_obj_del(items[i].low_temp_label);
            items[i].low_temp_label = NULL;
        }
    }
    
    // 清理图表
    if (chart.chart) {
        lv_obj_del(chart.chart);
        chart.chart = NULL;
        chart.high_temp_series = NULL;
        chart.low_temp_series = NULL;
    }
    
    // 标记为未初始化
    initialized = false;
}

/**
 * 根据天气描述文本获取天气类型
 */
WeatherType WeatherManager::getWeatherType(const char* weatherDesc) {
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
 */
const char* WeatherManager::getWeatherIconPath(WeatherType weatherType) {
    if (weatherType >= 0 && weatherType < 8) {
        return weatherIconPaths[weatherType];
    }
    return weatherIconPaths[WEATHER_UNKNOWN];
}

/**
 * 更新天气图标显示
 */
void WeatherManager::updateWeatherIcon(JsonObject dailyWeather, int index) {
    if (!initialized || index < 0 || index >= 4) {
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
    lv_label_set_text(items[index].date_label, dateText.c_str());
    
    // 根据天气描述设置图标
    WeatherType weatherType = getWeatherType(dailyWeatherDesc);
    const char* iconPath = getWeatherIconPath(weatherType);
    lv_img_set_src(items[index].icon, iconPath);
    
    // 更新温度标签
    lv_label_set_text(items[index].high_temp_label, highTempText.c_str());
    lv_label_set_text(items[index].low_temp_label, lowTempText.c_str());
    
    // 保存温度值用于图表显示
    chart.temp_high_values[index] = atoi(dailyTempHigh);
    chart.temp_low_values[index] = atoi(dailyTempLow);
}

/**
 * 更新气温趋势图表数据点
 */
void WeatherManager::updateWeatherChartData() {
    if (!initialized || !chart.chart) {
        return;
    }
    
    // 重新设置图表数据点数量，这会自动清除之前的数据
    lv_chart_set_point_count(chart.chart, 4);
    
    // 先删除之前的温度标签（如果存在）
    lv_obj_clean(chart.chart);
    
    // 重新添加数据系列（因为我们清除了图表）
    chart.high_temp_series = lv_chart_add_series(chart.chart, lv_color_hex(0xFF6600), LV_CHART_AXIS_PRIMARY_Y);
    chart.low_temp_series = lv_chart_add_series(chart.chart, lv_color_hex(0x0066CC), LV_CHART_AXIS_PRIMARY_Y);
    
    // 隐藏图例（左上方的色块）- 使用LVGL 8.3.7兼容的方法
    lv_obj_t * legend = lv_obj_get_child(chart.chart, -1);
    if (legend) {
      lv_obj_add_flag(legend, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 重新设置数据
    for (int i = 0; i < 4; i++) {
        lv_chart_set_next_value(chart.chart, chart.high_temp_series, chart.temp_high_values[i]);
        lv_chart_set_next_value(chart.chart, chart.low_temp_series, chart.temp_low_values[i]);
    }
    
    // 获取图表尺寸和位置信息来计算节点位置
    lv_coord_t x_ofs = lv_obj_get_x(chart.chart);
    lv_coord_t y_ofs = lv_obj_get_y(chart.chart);
    lv_coord_t w = lv_obj_get_width(chart.chart);
    
    // 手动计算每个点的位置并创建标签
    for (int i = 0; i < 4; i++) {
        // 计算X坐标（均匀分布在图表宽度上）
        lv_coord_t x = (w / 3) * i;
        
        // 计算Y坐标（根据温度值映射到图表高度）
        lv_coord_t chart_h = lv_obj_get_height(chart.chart);
        int temp_range = 40 - (-10); // 温度范围：-10℃到40℃
        
        // 最高温度Y坐标（温度越高，Y值越小，因为图表Y轴是从上到下递增）
        lv_coord_t high_y = chart_h - ((chart.temp_high_values[i] - (-10)) * chart_h / temp_range);
        
        // 最低温度Y坐标
        lv_coord_t low_y = chart_h - ((chart.temp_low_values[i] - (-10)) * chart_h / temp_range);
        
        // 创建最高温度标签，确保完整显示（使用中文字体）
        char high_temp_str[6];
        sprintf(high_temp_str, "%d℃", chart.temp_high_values[i]);
        lv_obj_t* high_temp_label = lv_label_create(chart.chart);
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
        sprintf(low_temp_str, "%d℃", chart.temp_low_values[i]);
        lv_obj_t* low_temp_label = lv_label_create(chart.chart);
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
    
    // 刷新图表显示
    lv_chart_refresh(chart.chart);
}

/**
 * 刷新完整的天气显示
 */
void WeatherManager::refreshDisplay() {
    if (!initialized) {
        return;
    }
    
    // 更新图表数据
    updateWeatherChartData();
    
    // 显示所有组件
    show();
}

/**
 * 显示天气显示组件
 */
void WeatherManager::show() {
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

/**
 * 隐藏天气显示组件
 */
void WeatherManager::hide() {
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

/**
 * 设置天气表格对象
 */
void WeatherManager::setWeatherTable(lv_obj_t* table) {
    weather_table = table;
}

/**
 * 从JSON数据更新天气信息
 */
bool WeatherManager::updateWeatherData(const String& jsonData) {
    if (!initialized || jsonData.isEmpty()) {
        return false;
    }
    
    // 解析JSON数据
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
        return false;
    }
    
    // 获取数据
    JsonArray daily = doc["result"]["daily"].as<JsonArray>();
    
    // 更新天气图标和温度数据
    for (int i = 0; i < daily.size() && i < 4; i++) {
        updateWeatherIcon(daily[i].as<JsonObject>(), i);
    }
    
    // 刷新显示
    refreshDisplay();
    
    return true;
}