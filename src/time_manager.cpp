#include "time_manager.h"
#include <WiFi.h>
#include "config.h"
#include "lvgl.h"
#include "esp_log.h"

// 字体定义
extern const lv_font_t lvgl_font_digital_24;
extern const lv_font_t lvgl_font_digital_48;
extern const lv_font_t lvgl_font_song_16;

// 定义单例实例
TimeManager* TimeManager::instance = nullptr;

// 定义日志标签
static const char* TAG = "TimeManager";

// 全局标签声明 - 只在这个文件中定义，避免与net_http.cpp冲突
lv_obj_t* hour_minute_label = nullptr;
lv_obj_t* second_label = nullptr;
lv_obj_t* date_label = nullptr;
lv_obj_t* week_label = nullptr;

// 定义中文星期数组
const char* weekDays[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};

/**
 * 私有构造函数
 */
TimeManager::TimeManager() {
    // 初始化时间变量
    currentHour = 0;
    currentMinute = 0;
    currentSecond = 0;
    lastSecondUpdate = 0;
    lastMinuteUpdate = 0;
    timeStrBuffer[0] = '\0';
    
    // 初始化标签指针
    hour_minute_label = nullptr;
    second_label = nullptr;
    date_label = nullptr;
    week_label = nullptr;
}

/**
 * 获取单例实例
 */
TimeManager* TimeManager::getInstance() {
    if (instance == nullptr) {
        instance = new TimeManager();
    }
    return instance;
}

/**
 * 初始化时间管理器
 */
void TimeManager::init() {
    Serial.println("初始化时间管理器");
    
    // 创建时分钟标签
    if (!hour_minute_label) {
        hour_minute_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(hour_minute_label, &lvgl_font_digital_48, 0); // 48px数字字体
        lv_obj_set_style_text_color(hour_minute_label, lv_color_hex(0x000000), 0); // 黑色，提高可见度
        lv_obj_align(hour_minute_label, LV_ALIGN_TOP_LEFT, 10, 30); // 放在第一排下方
        Serial.println("创建时分钟标签成功");
    }
    
    // 创建秒标签
    if (!second_label) {
        second_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(second_label, &lvgl_font_digital_24, 0); // 使用24px数字字体
        lv_obj_set_style_text_color(second_label, lv_color_hex(0x000000), 0); // 黑色
        // 放置在hour_minute_label右侧，与上边缘对齐
        lv_obj_align_to(second_label, hour_minute_label, LV_ALIGN_OUT_RIGHT_TOP, 18, 2);
        Serial.println("创建秒标签成功");
    }

    // 创建日期标签 - 修改位置和大小，确保完整显示
    if (!date_label) {
        date_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(date_label, &lvgl_font_song_16, 0); // 16px字体
        lv_obj_set_style_text_color(date_label, lv_color_hex(0x000000), 0); // 黑色文字
        lv_obj_align(date_label, LV_ALIGN_TOP_LEFT, 10, 4); // 放在最顶部
        lv_obj_set_width(date_label, 150); // 增加宽度，确保完整显示中文日期
        Serial.println("创建日期标签成功");
    }

    // 创建星期标签
    if (!week_label) {
        week_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(week_label, &lvgl_font_song_16, 0); // 16px字体
        lv_obj_set_style_text_color(week_label, lv_color_hex(0x000000), 0); // 黑色文字
        lv_obj_align(week_label, LV_ALIGN_TOP_LEFT, 120, 4); // 放在日期标签右侧
        lv_obj_set_width(week_label, 80); // 限制宽度
        Serial.println("创建星期标签成功");
    }
    
    // 初始化显示内容
    if (hour_minute_label) {
        lv_label_set_text(hour_minute_label, "--:--");
        lv_obj_clear_flag(hour_minute_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    }
    
    if (second_label) {
        lv_label_set_text(second_label, "--");
        lv_obj_clear_flag(second_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    }
    
    if (date_label) {
        lv_label_set_text(date_label, "2023-01-01");
        lv_obj_clear_flag(date_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    }
    
    if (week_label) {
        lv_label_set_text(week_label, "星期日");
        lv_obj_clear_flag(week_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    }
    
    // 初始化时强制更新分钟显示
    updateMinuteDisplay();
}

/**
 * 获取时分钟标签
 */
lv_obj_t* TimeManager::getHourMinuteLabel() {
    return hour_minute_label;
}

/**
 * 获取秒标签
 */
lv_obj_t* TimeManager::getSecondLabel() {
    return second_label;
}

/**
 * 更新时间显示
 */
void TimeManager::updateTimeDisplay() {
    unsigned long currentTime = millis();
    
    // 更新秒显示
    if (currentTime - lastSecondUpdate >= 1000) {
        updateSecondDisplay();
        lastSecondUpdate = currentTime;
    }
    
    // 更新分钟显示（每分钟或强制刷新时）
    if (currentTime - lastMinuteUpdate >= 60000 || (currentTime - lastMinuteUpdate >= 5000 && forceUpdateMinute)) {
        updateMinuteDisplay();
        lastMinuteUpdate = currentTime;
        forceUpdateMinute = false;
    }
}

/**
 * 更新分钟显示
 */
void TimeManager::updateMinuteDisplay() {
    // 获取当前时间
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // 更新当前小时和分钟
    currentHour = timeinfo.tm_hour;
    currentMinute = timeinfo.tm_min;
    
    // 格式化小时和分钟
    char hourMinuteStringBuff[6]; // 格式为 HH:MM
snprintf(hourMinuteStringBuff, sizeof(hourMinuteStringBuff), "%02d:%02d", currentHour, currentMinute);
    
    // 更新时分钟标签
    if (hour_minute_label) {
        lv_label_set_text(hour_minute_label, hourMinuteStringBuff);
        lv_obj_clear_flag(hour_minute_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    }
    
    // 更新日期显示 - 改进格式确保完整显示
    if (date_label) {
        char dateStringBuff[15]; // 格式为 YYYY-MM-DD
        snprintf(dateStringBuff, sizeof(dateStringBuff), "%04d-%02d-%02d", 
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
        lv_label_set_text(date_label, dateStringBuff);
    }
    
    // 更新星期显示
    if (week_label && timeinfo.tm_wday >= 0 && timeinfo.tm_wday < 7) {
        lv_label_set_text(week_label, weekDays[timeinfo.tm_wday]);
    }
    

    
    // 获取农历信息（如果有必要）
    // getlunar();
}

/**
 * 更新秒显示
 */
void TimeManager::updateSecondDisplay() {
    // 获取当前时间
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // 更新当前秒
    currentSecond = timeinfo.tm_sec;
    
    // 格式化秒
    char secondStringBuff[3]; // 格式为 SS
    snprintf(secondStringBuff, sizeof(secondStringBuff), "%02d", currentSecond);
    
    // 更新秒标签
    if (second_label) {
        lv_label_set_text(second_label, secondStringBuff);
        lv_obj_clear_flag(second_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    }
    

}

/**
 * 设置强制更新分钟显示
 */
void TimeManager::setForceUpdateMinute(bool forceUpdate) {
    forceUpdateMinute = forceUpdate;
}

/**
 * 强制更新所有时间显示
 */
void TimeManager::forceUpdateAll() {
    updateMinuteDisplay();
    updateSecondDisplay();
}