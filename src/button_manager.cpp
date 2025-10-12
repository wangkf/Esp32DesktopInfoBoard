#include "button_manager.h"

// 定义单例实例
ButtonManager* ButtonManager::instance = nullptr;

/**
 * 私有构造函数
 */
ButtonManager::ButtonManager(int pin, 
                            unsigned long debounceTime, 
                            unsigned long shortPressTime, 
                            unsigned long longPressTime, 
                            unsigned long multiClickTime) : 
    oneButton(pin, true), // true表示按钮按下时为LOW电平
    buttonPin(pin),
    currentEvent(NONE),
    clickCount(0),
    lastEventTime(0) {
    // 设置OneButton库的参数
    oneButton.setDebounceTicks(debounceTime);
    oneButton.setClickTicks(shortPressTime);
    oneButton.setPressTicks(longPressTime);
}

/**
 * 获取单例实例
 */
ButtonManager* ButtonManager::getInstance() {
    if (instance == nullptr) {
        // 使用默认参数创建单例实例
        instance = new ButtonManager();
    }
    return instance;
}

/**
 * 初始化按钮
 */
void ButtonManager::begin() {
    // 设置回调函数
    oneButton.attachClick(ButtonManager::clickCallback);
    oneButton.attachDoubleClick(ButtonManager::doubleClickCallback);
    oneButton.attachMultiClick(ButtonManager::multiClickCallback);
    oneButton.attachLongPressStart(ButtonManager::longPressCallback);
    
    // 初始化按钮引脚
    pinMode(buttonPin, INPUT_PULLUP);
}

/**
 * 检查按钮事件（应该在loop中调用）
 */
ButtonEvent ButtonManager::check() {
    // 首先更新按钮状态
    update();
    
    // 返回当前事件并重置事件状态
    ButtonEvent current = currentEvent;
    if (currentEvent != NONE) {
        // 只有在事件发生后才更新最后事件时间
        lastEventTime = millis();
    }
    
    return current;
}

/**
 * 获取当前按钮状态
 */
bool ButtonManager::isPressed() {
    return digitalRead(buttonPin) == LOW;
}

/**
 * 获取点击计数
 */
int ButtonManager::getClickCount() {
    return clickCount;
}

/**
 * 更新按钮状态（在loop中调用）
 */
void ButtonManager::update() {
    // 调用OneButton库的tick函数来更新按钮状态
    oneButton.tick();
    
    // 检查是否需要重置状态（如果长时间没有事件发生）
    if (currentEvent != NONE && millis() - lastEventTime > 1000) {
        resetState();
    }
}

/**
 * 重置点击计数和事件
 */
void ButtonManager::resetState() {
    currentEvent = NONE;
    clickCount = 0;
}

/**
 * 单击回调函数
 */
void ButtonManager::clickCallback() {
    if (instance) {
        instance->currentEvent = SHORT_PRESS;
        instance->clickCount = 1;
        instance->lastEventTime = millis();
    }
}

/**
 * 双击回调函数
 */
void ButtonManager::doubleClickCallback() {
    if (instance) {
        instance->currentEvent = DOUBLE_CLICK;
        instance->clickCount = 2;
        instance->lastEventTime = millis();
    }
}

/**
 * 多击回调函数
 */
void ButtonManager::multiClickCallback() {
    if (instance) {
        // 获取OneButton库的点击计数
        int count = instance->oneButton.getNumberClicks();
        if (count >= 3) {
            instance->currentEvent = TRIPLE_CLICK;
            instance->clickCount = count;
            instance->lastEventTime = millis();
        }
    }
}

/**
 * 长按回调函数
 */
void ButtonManager::longPressCallback() {
    if (instance) {
        instance->currentEvent = LONG_PRESS;
        instance->clickCount = 0;
        instance->lastEventTime = millis();
    }
}

/**
 * 设置按钮事件的时间阈值
 */
void ButtonManager::setTimeThresholds(unsigned long shortPressTime, 
                                     unsigned long longPressTime, 
                                     unsigned long multiClickTime) {
    oneButton.setClickTicks(shortPressTime);
    oneButton.setPressTicks(longPressTime);
}

/**
 * 获取上次事件时间
 */
unsigned long ButtonManager::getLastEventTime() {
    return lastEventTime;
}