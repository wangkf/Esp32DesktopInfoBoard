#include "time_manager.h"
#include <WiFi.h>
#include "config/config.h"
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

// 定义中文星期数组
const char* weekDays[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};

// 声明配置中的屏幕尺寸
extern const uint32_t screenWidth;
extern const uint32_t screenHeight;

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
    forceUpdateMinute = false;
    
    // 初始化标签指针为nullptr
    date_label = nullptr;
    weekday_label = nullptr;
    hour_minute_label = nullptr;
    second_label = nullptr;
    status_label = nullptr;
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
    
    // === 时分标签（hour_minute_label）=== 位置：左上角对齐，x偏移5，y偏移30
    if (!hour_minute_label) {
        hour_minute_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(hour_minute_label, &lvgl_font_digital_48, 0); // 48像素数字字体
        lv_obj_set_style_text_color(hour_minute_label, lv_color_hex(0x006400), 0); // 深绿色
        lv_obj_align(hour_minute_label, LV_ALIGN_TOP_LEFT, 2, 20); // 位置：左上角对齐，x偏移5，y偏移30
        lv_label_set_text(hour_minute_label, "--:--");
    }
    
    // === 秒标签（second_label）=== 位置：时分标签右侧中间对齐，x偏移5，y偏移0
    if (!second_label) {
        second_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(second_label, &lvgl_font_digital_24, 0); // 24像素数字字体
        lv_obj_set_style_text_color(second_label, lv_color_hex(0xFF0000), 0); // 红色
        lv_obj_align_to(second_label, hour_minute_label, LV_ALIGN_OUT_RIGHT_MID, 2, 0); // 位置：时分标签右侧中间对齐，x偏移5，y偏移0
        lv_label_set_text(second_label, "--");
    }
    
    // === 日期标签（date_label）=== 位置：顶部中间对齐，x偏移0，y偏移2
    if (!date_label) {
        date_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(date_label, &lvgl_font_song_16, 0); // 16像素宋体
        lv_obj_set_style_text_color(date_label, lv_color_hex(0x0000FF), 0); // 蓝色
        lv_obj_align(date_label, LV_ALIGN_TOP_LEFT, 0, 2); // 位置：顶部中间对齐，x偏移0，y偏移2
        lv_obj_set_width(date_label, 120); // 设置足够的宽度确保显示完整
        lv_label_set_text(date_label, "2023年01月01日");
    }
    
    // === 星期标签（weekday_label）=== 位置：顶部中间对齐，x偏移0，y偏移22
    if (!weekday_label) {
        weekday_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(weekday_label, &lvgl_font_song_16, 0); // 16像素宋体
        lv_obj_set_style_text_color(weekday_label, lv_color_hex(0x0000FF), 0); // 蓝色
        lv_obj_align(weekday_label, LV_ALIGN_TOP_LEFT, 120, 2); // 位置：顶部中间对齐，x偏移0，y偏移22
        lv_label_set_text(weekday_label, "星期日");
    }
    
    // === 状态标签（status_label）=== 位置：星期标签右侧，延伸到最右边
    if (!status_label) {
        status_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(status_label, &lvgl_font_song_16, 0); // 16像素宋体
        lv_obj_set_style_text_color(status_label, lv_color_hex(0x008000), 0); // 默认绿色
        lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 200, 2); // 星期标签右侧
        lv_obj_set_width(status_label, screenWidth - 200); // 长度到最右边
        lv_label_set_text(status_label, ""); // 初始为空
        // 启用自动换行
        lv_label_set_long_mode(status_label, LV_LABEL_LONG_WRAP);
    }
    
    // 初始化时强制更新分钟显示
    updateMinuteDisplay();
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
    
    // 更新日期显示
    if (date_label) {
        char dateStringBuff[18]; // 格式为 YYYY年MM月DD日
        snprintf(dateStringBuff, sizeof(dateStringBuff), "%d年%02d月%02d日", 
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
        lv_label_set_text(date_label, dateStringBuff);
        lv_obj_clear_flag(date_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    }
    
    // 更新星期显示
    if (weekday_label && timeinfo.tm_wday >= 0 && timeinfo.tm_wday < 7) {
        lv_label_set_text(weekday_label, weekDays[timeinfo.tm_wday]);
        lv_obj_clear_flag(weekday_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    }
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
 * 设置状态信息
 */
void TimeManager::setStatusInfo(const char* info, lv_color_t color, bool scroll) {
    if (status_label) {
        lv_label_set_text(status_label, info);
        lv_obj_set_style_text_color(status_label, color, 0);
        
        if (scroll) {
            // 设置为滚动模式
            lv_label_set_long_mode(status_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
            // 确保标签宽度足够，设置为屏幕宽度减去200像素
            lv_obj_set_width(status_label, screenWidth - 200);
            // 设置滚动速度（可选）
            lv_obj_set_style_anim_speed(status_label, 100, 0); // 滚动动画速度
        } else {
            // 设置为换行模式
            lv_label_set_long_mode(status_label, LV_LABEL_LONG_WRAP);
        }
        
        lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    }
}

/**
 * 清除状态信息
 */
void TimeManager::clearStatusInfo() {
    if (status_label) {
        lv_label_set_text(status_label, "");
    }
}

/**
 * 强制更新所有时间显示
 */
void TimeManager::forceUpdateAll() {
    updateMinuteDisplay();
    updateSecondDisplay();
}