#ifndef BUTTON_UTILS_H_
#define BUTTON_UTILS_H_

#include <Arduino.h>
#include <OneButton.h> // 引入OneButton库
#include "config/config.h" // 包含配置文件

class ButtonUtils {
private:
  // OneButton对象
  OneButton oneButton;
  
  // 引脚号
  int buttonPin;
  
  // 当前按钮事件
  volatile ButtonEvent currentEvent;
  
  // 点击计数
  volatile int clickCount;

public:
  // 构造函数
  ButtonUtils(int pin, unsigned long debounceTime = 50, 
              unsigned long shortPressTime = 200, 
              unsigned long longPressTime = 1000, 
              unsigned long multiClickTime = 300);
  
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
};

// 全局实例指针，用于回调函数访问
extern ButtonUtils* g_buttonInstance;

#endif /* BUTTON_UTILS_H_ */