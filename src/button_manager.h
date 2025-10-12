#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include <OneButton.h> // 引入OneButton库
#include "config.h" // 包含配置文件

/**
 * 按钮管理器类
 * 负责管理按钮事件的检测和处理
 */
class ButtonManager {
private:
    static ButtonManager* instance; // 单例实例
    
    // OneButton对象
    OneButton oneButton;
    
    // 引脚号
    int buttonPin;
    
    // 当前按钮事件
    volatile ButtonEvent currentEvent;
    
    // 点击计数
    volatile int clickCount;
    
    // 上次事件时间
    unsigned long lastEventTime;
    
    // 私有构造函数（单例模式）
    ButtonManager(int pin = BUTTON_PIN, 
                 unsigned long debounceTime = DEBOUNCE_DELAY, 
                 unsigned long shortPressTime = SHORT_PRESS_THRESHOLD, 
                 unsigned long longPressTime = LONG_PRESS_THRESHOLD, 
                 unsigned long multiClickTime = MULTI_CLICK_THRESHOLD);
    
    // 重置点击计数和事件
    void resetState();
    
public:
    // 获取单例实例
    static ButtonManager* getInstance();
    
    // 初始化按钮
    void begin();
    
    // 检查按钮事件（应该在loop中调用）
    ButtonEvent check();
    
    // 获取当前按钮状态
    bool isPressed();
    
    // 获取点击计数
    int getClickCount();
    
    // 更新按钮状态（在loop中调用）
    void update();
    
    // 事件回调函数
    static void clickCallback();
    static void doubleClickCallback();
    static void multiClickCallback();
    static void longPressCallback();
    
    // 设置按钮事件的时间阈值
    void setTimeThresholds(unsigned long shortPressTime, 
                          unsigned long longPressTime, 
                          unsigned long multiClickTime);
    
    // 获取上次事件时间
    unsigned long getLastEventTime();
};

#endif // BUTTON_MANAGER_H