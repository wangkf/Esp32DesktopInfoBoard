#include "button_utils.h"

// 全局实例指针定义
ButtonUtils* g_buttonInstance = nullptr;

// 构造函数
ButtonUtils::ButtonUtils(int pin, unsigned long debounceTime, 
                         unsigned long shortPressTime, 
                         unsigned long longPressTime, 
                         unsigned long multiClickTime) 
  : oneButton(pin, true), buttonPin(pin), currentEvent(NONE), clickCount(0) {
  
  // 设置消抖时间
  oneButton.setDebounceTicks(debounceTime);
  
  // 设置点击时间阈值
  oneButton.setClickTicks(shortPressTime);
  
  // 设置长按时间阈值
  oneButton.setPressTicks(longPressTime);
  
  // 保存全局实例指针
  g_buttonInstance = this;
}

// 初始化按钮
void ButtonUtils::begin() {
  // 设置事件回调
  oneButton.attachClick(clickCallback);
  oneButton.attachDoubleClick(doubleClickCallback);
  oneButton.attachMultiClick(multiClickCallback);
  oneButton.attachLongPressStart(longPressCallback);
}

// 点击回调
void ButtonUtils::clickCallback() {
  if (g_buttonInstance) {
    g_buttonInstance->currentEvent = SHORT_PRESS;
    g_buttonInstance->clickCount = 1;
  }
}

// 双击回调
void ButtonUtils::doubleClickCallback() {
  if (g_buttonInstance) {
    g_buttonInstance->currentEvent = DOUBLE_CLICK;
    g_buttonInstance->clickCount = 2;
  }
}

// 多击回调
void ButtonUtils::multiClickCallback() {
  if (g_buttonInstance) {
    g_buttonInstance->clickCount = g_buttonInstance->oneButton.getNumberClicks();
    if (g_buttonInstance->clickCount >= 3) {
      g_buttonInstance->currentEvent = TRIPLE_CLICK;
    }
  }
}

// 长按回调
void ButtonUtils::longPressCallback() {
  if (g_buttonInstance) {
    g_buttonInstance->currentEvent = LONG_PRESS;
  }
}

// 检查按钮事件
ButtonEvent ButtonUtils::check() {
  ButtonEvent event = currentEvent;
  if (event != NONE) {
    currentEvent = NONE; // 重置事件
  }
  return event;
}

// 获取当前按钮状态
bool ButtonUtils::isPressed() {
  return digitalRead(buttonPin) == LOW; // 按钮按下时返回true
}

// 获取点击计数
int ButtonUtils::getClickCount() {
  return clickCount;
}

// 更新按钮状态
void ButtonUtils::update() {
  oneButton.tick();
}