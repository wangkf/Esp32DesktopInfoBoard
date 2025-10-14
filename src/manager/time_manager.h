#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <lvgl.h>
#include <time.h>

/**
 * 时间管理器类
 * 负责管理时间显示和更新
 */
class TimeManager {
private:
    static TimeManager* instance; // 单例实例
    
    // 时间显示元素 - 直接管理所有时间相关标签
    lv_obj_t* date_label;         // 日期标签
    lv_obj_t* weekday_label;      // 星期几标签
    lv_obj_t* hour_minute_label;  // 时分钟标签
    lv_obj_t* second_label;       // 秒钟标签
    lv_obj_t* status_label;       // 状态信息标签
    lv_obj_t* ip_label;           // IP地址标签
    
    // 时间变量
    int currentHour;              // 当前小时
    int currentMinute;            // 当前分钟
    int currentSecond;            // 当前秒钟
    unsigned long lastSecondUpdate; // 上次秒更新时间
    unsigned long lastMinuteUpdate; // 上次分钟更新时间
    char timeStrBuffer[32];       // 时间字符串缓冲区
    bool forceUpdateMinute;       // 强制更新分钟标志
    
    // 私有构造函数（单例模式）
    TimeManager();
    
    // 更新分钟显示（每分钟更新一次）
    void updateMinuteDisplay();
    
    // 更新秒钟显示（每秒更新一次）
    void updateSecondDisplay();
    
public:
    // 获取单例实例
    static TimeManager* getInstance();
    
    // 初始化时间管理器
    void init();
    
    // 更新时间显示
    void updateTimeDisplay();
    
    // 获取当前时间
    bool getCurrentTime(struct tm &timeinfo);
    
    // 设置状态信息
    void setStatusInfo(const char* info, lv_color_t color, bool scroll);
    
    // 清除状态信息
    void clearStatusInfo();
    
    // 设置IP地址信息
    void setIpInfo(const char* info, lv_color_t color);
    
    // 清除IP地址信息
    void clearIpInfo();
    
    // 获取日期标签
    lv_obj_t* getDateLabel();
    
    // 获取星期几标签
    lv_obj_t* getWeekdayLabel();
    
    // 获取时分钟标签
    lv_obj_t* getHourMinuteLabel();
    
    // 获取秒钟标签
    lv_obj_t* getSecondLabel();
    
    // 设置强制更新分钟显示
    void setForceUpdateMinute(bool forceUpdate);
    
    // 强制更新所有时间显示
    void forceUpdateAll();
};

#endif // TIME_MANAGER_H