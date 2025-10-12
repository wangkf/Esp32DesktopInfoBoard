#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <lvgl.h>
#include "config/config.h"

// 屏幕状态枚举已在config.h中定义



/**
 * 屏幕管理器类
 * 负责管理所有屏幕的切换和显示
 */
class ScreenManager {
private:
    static ScreenManager* instance; // 单例实例
    ScreenState currentScreen;      // 当前屏幕状态
    
    // 屏幕元素指针
    lv_obj_t* screen_symbol_label;  // 屏幕主题符号标签
    lv_obj_t* screen_title_btn;     // 屏幕标题按钮
    lv_obj_t* title_label;          // 屏幕标题文本标签
    
    // 私有构造函数（单例模式）
    ScreenManager();
    
    // 隐藏所有屏幕元素
    void hideAllScreens();
    
    // 显示特定屏幕
    void showNewsScreen();
    void showMaoSelectScreen();
    void showToxicSoulScreen();
    void showIcibaScreen();
    void showAstronautsScreen();
    
    // 显示当前屏幕
    void showCurrentScreen();
    
public:
    // 获取单例实例
    static ScreenManager* getInstance();
    
    // 初始化屏幕管理器
    void init();
    
    // 切换到下一个屏幕
    void toggleScreen();
    
    // 直接切换到指定屏幕
    void switchToScreen(ScreenState screenState);
    
    // 获取当前屏幕状态
    inline ScreenState getCurrentScreen() { return currentScreen; }
    
    // 刷新当前屏幕数据
    void refreshCurrentScreenData();
    
    // 显示特定类型的信息
    void showRandomMaoSelect();
    void showRandomToxicSoul();
    void displayAstronautsDataFromFile();
    
    // 设置配置模式图标状态
    void setConfigIconStatus(bool isConfigMode);
};

#endif // SCREEN_MANAGER_H