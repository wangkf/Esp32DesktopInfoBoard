#ifndef UI_MANAGER_H
#define UI_MANAGER_H
#include <Arduino.h>
#include <lvgl.h>
#include "config/config.h"
#include "config/config_manager.h"
#include <memory>
#include <vector>
#include <map>

// 颜色类型枚举
enum ColorType {
    COLOR_TEXT,
    COLOR_BG,
    COLOR_TIME,
    COLOR_DAY,
    COLOR_CHROMAKEY,
    COLOR_RED = 0xFF0000,
    COLOR_BLACK = 0x000000
};

// UI元素类型枚举
enum UIElementType {
    LABEL,
    IMAGE
};

// UI元素结构体
struct UIElement {
    lv_obj_t* object;
    UIElementType type;
    String name;
};

// 主题颜色结构体
struct ThemeColors {
    uint32_t TextColor;
    uint32_t bgColor;
    uint32_t timeColor;
    uint32_t dayColor;
    uint32_t chromaKey;
    int themeId;
    
    // 比较运算符，用于检测颜色是否变化
    bool operator!=(const ThemeColors& other) const {
        return TextColor != other.TextColor ||
               bgColor != other.bgColor ||
               timeColor != other.timeColor ||
               dayColor != other.dayColor ||
               chromaKey != other.chromaKey ||
               themeId != other.themeId;
    }
};

// UI管理器类 - 采用单例模式
class UIManager {
private:
    static UIManager* instance;
    ThemeColors currentTheme;
    ThemeColors previousTheme;
    std::vector<UIElement> uiElements;
    std::map<String, lv_obj_t*> elementMap;
    
    UIManager(); // 私有构造函数
    
    // 初始化LVGL缓冲区
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[];
    static lv_color_t buf2[];
    
    // 创建UI元素并添加到管理列表
    lv_obj_t* createAndManageElement(lv_obj_t* element, UIElementType type, const String& name);
    
public:
    // 获取单例实例（线程安全）
    static UIManager* getInstance();
    
    // 初始化UI管理器
    bool init();
    
    // 初始化主题颜色
    void initThemeColors();
    
    // 重新应用主题设置（只在颜色变化时更新）
    void reapplyTheme();
    
    // 创建标签并管理
    lv_obj_t* createManagedLabel(const lv_font_t* font, lv_color_t color, uint32_t x, uint32_t y, uint32_t height = 0);
    
    // 创建图像并管理
    lv_obj_t* createManagedImage(const lv_img_dsc_t *src, uint32_t w, uint32_t h, uint32_t x, uint32_t y, lv_color_t bgcolor);
    
    // 隐藏所有UI元素
    void hideAllElements();
    
    // 显示指定名称的元素
    void showElement(const String& name);
    
    // 隐藏指定名称的元素
    void hideElement(const String& name);
    
    // 获取元素
    lv_obj_t* getElement(const String& name);
    
    // 获取当前主题颜色
    ThemeColors getCurrentTheme() const { return currentTheme; }
    
    // 获取颜色值
    lv_color_t getColorValue(ColorType colorType);
    
    // 更新单个元素的颜色
    void updateElementColor(const String& name, lv_color_t color);
    
    // 注册UI元素到管理器
    void registerElement(const String& name, lv_obj_t* element, UIElementType type);
    
    // 批量显示多个UI元素
    void showElements(const std::vector<String>& elementNames);
    
    // 性能监控相关方法
    uint32_t getLastOperationTime() const; // 获取上次操作耗时
};

#endif // UI_MANAGER_H