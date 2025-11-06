#include "ui_manager.h"
#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "config/config.h"
#include "config/config_manager.h"
#include "manager/time_manager.h"
#include "images/images.h"
#include "ui_utils.h"

// 静态成员初始化
UIManager* UIManager::instance = nullptr;
lv_disp_draw_buf_t UIManager::draw_buf;
lv_color_t UIManager::buf[screenWidth * 10]; // 统一使用screenWidth*10
lv_color_t UIManager::buf2[screenWidth * 10]; // 统一使用screenWidth*10

// TFT对象
TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight);

// LVGL显示回调函数
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

// 构造函数
UIManager::UIManager() {
  // 初始化主题颜色
  currentTheme.themeId = -1;
  previousTheme.themeId = -1;
}

// 获取单例实例（线程安全）
UIManager* UIManager::getInstance() {
  // ESP32上的简单线程安全实现
  if (instance == nullptr) {
    // 禁用中断来确保线程安全
    portDISABLE_INTERRUPTS();
    if (instance == nullptr) {
      instance = new UIManager();
    }
    portENABLE_INTERRUPTS();
  }
  return instance;
}

// 初始化UI管理器
bool UIManager::init() {
  Serial.println("初始化UI管理器...");
  
  // 初始化显示屏
  tft.init();
  tft.setRotation(0); // 设置为正常方向（0度）
  lv_init();  // 初始化LVGL
  
  // 配置显示缓冲区 - 使用双缓冲区，统一大小
  Serial.println("配置LVGL显示缓冲区...");
  lv_disp_draw_buf_init(&draw_buf, buf, buf2, screenWidth * 10);
  
  static lv_disp_drv_t disp_drv; // 配置显示驱动
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.flush_cb = my_disp_flush;
  lv_disp_drv_register(&disp_drv);
  
  // 初始化主题颜色
  initThemeColors();
  
  Serial.println("UI管理器初始化完成");
  return true;
}

// 初始化主题颜色
void UIManager::initThemeColors() {
  // 获取主题ID（与config.h中的ThemeID枚举保持一致）
  previousTheme = currentTheme; // 保存之前的主题
  
  ConfigManager* configManager = ConfigManager::getInstance();
  currentTheme.themeId = configManager->getDisplayTheme();
  
  // 确保主题ID在有效范围内
    if (currentTheme.themeId < THEME_DARK || currentTheme.themeId > THEME_SPECIAL) {
        currentTheme.themeId = THEME_DARK; // 默认使用深色主题
    }
    
    // 性能监控：记录开始时间
    uint32_t startTime = millis();
  
  // 使用映射表替代switch语句
  std::map<int, ThemeColors> themeMap;
  
  // 白天主题
  ThemeColors lightTheme;
  lightTheme.TextColor = 0x000000;
  lightTheme.bgColor = 0xFFFFFF;
  lightTheme.timeColor = 0x999999;
  lightTheme.dayColor = 0x0000FF;
  lightTheme.chromaKey = 0x000000;
  lightTheme.themeId = THEME_LIGHT;
  themeMap[THEME_LIGHT] = lightTheme;
  
  // 黑夜主题
  ThemeColors darkTheme;
  darkTheme.TextColor = 0xFFFFFF;
  darkTheme.bgColor = 0x000000;
  darkTheme.timeColor = 0x00FF00;
  darkTheme.dayColor = 0xFF0000;
  darkTheme.chromaKey = 0xFFFFFF;
  darkTheme.themeId = THEME_DARK;
  themeMap[THEME_DARK] = darkTheme;
  
  // 特殊主题
  ThemeColors specialTheme;
  specialTheme.TextColor = 0x000FFF;
  specialTheme.bgColor = 0xCCCCE0;
  specialTheme.timeColor = 0xBBBBBB;
  specialTheme.dayColor = 0x00FF00;
  specialTheme.chromaKey = 0x00ee00;
  specialTheme.themeId = THEME_SPECIAL;
  themeMap[THEME_SPECIAL] = specialTheme;
  
  // 获取对应的主题配置
  if (themeMap.find(currentTheme.themeId) != themeMap.end()) {
    currentTheme = themeMap[currentTheme.themeId];
    Serial.printf("已加载主题ID: %d\n", currentTheme.themeId);
  } else {
    // 默认使用深色主题
    currentTheme = themeMap[THEME_DARK];
    Serial.println("警告: 找不到指定主题，使用默认深色主题");
  }
  
  Serial.printf("主题初始化完成，主题ID: %d, 耗时: %u ms\n", currentTheme.themeId, millis() - startTime);
}

// 重新应用主题设置（只在颜色变化时更新）
void UIManager::reapplyTheme() {
    Serial.println("检查主题变更...");
    
    // 记录开始时间用于性能监控
    uint32_t startTime = millis();
    
    // 重新加载主题颜色
    initThemeColors();
    
    // 只有当主题实际发生变化时才更新UI
    if (currentTheme != previousTheme) {
        Serial.println("主题颜色已变化，开始应用新主题...");
        
        // 应用背景颜色，先检查是否需要更新
        lv_color_t bgColor = lv_color_hex(currentTheme.bgColor);
        lv_color_t currentBgColor = lv_obj_get_style_bg_color(lv_scr_act(), 0);
        if (currentBgColor.full != bgColor.full) {
            lv_obj_set_style_bg_color(lv_scr_act(), bgColor, 0);
            Serial.println("更新屏幕背景颜色");
        }
        
        // 更新时间标签颜色
        TimeManager::getInstance()->updateTimeLabelsColor();
        
        // 更新所有注册的UI元素，利用updateElementColor方法只在颜色实际变化时更新
        for (auto& element : uiElements) {
            if (element.object && lv_obj_is_valid(element.object)) {
                if (element.type == LABEL) {
                    // 使用updateElementColor方法，该方法内部会检查颜色是否已相同
                    updateElementColor(element.name, lv_color_hex(currentTheme.TextColor));
                } else if (element.type == IMAGE) {
                    // 使用updateElementColor方法，该方法内部会检查颜色是否已相同
                    updateElementColor(element.name, bgColor);
                }
            }
        }
        
        // 特殊处理日期标签
        lv_obj_t* todayDateLabel = getElement("today_date_label");
        if (todayDateLabel && lv_obj_is_valid(todayDateLabel)) {
            lv_obj_set_style_text_font(todayDateLabel, BIGFont, 0);
            lv_obj_set_style_text_color(todayDateLabel, 
              lv_color_hex(currentTheme.dayColor), 0);
        }
        
        // 刷新屏幕以应用更改
        lv_obj_invalidate(lv_scr_act());
    } else {
        Serial.println("主题颜色未变化，跳过更新");
    }
    
    // 性能监控：记录结束时间并输出执行时间
    uint32_t endTime = millis();
    Serial.printf("主题应用执行时间: %lu 毫秒\n", endTime - startTime);
}

// 创建标签并管理
lv_obj_t* UIManager::createManagedLabel(const lv_font_t* font, lv_color_t color, uint32_t x, uint32_t y, uint32_t height) {
  lv_obj_t* label = createLabel(font, color, x, y, height);
  String name = "label_" + String(uiElements.size());
  return createAndManageElement(label, LABEL, name);
}

// 创建图像并管理
lv_obj_t* UIManager::createManagedImage(const lv_img_dsc_t *src, uint32_t w, uint32_t h, uint32_t x, uint32_t y, lv_color_t bgcolor) {
  lv_obj_t* img = createImage(src, w, h, x, y, bgcolor);
  String name = "image_" + String(uiElements.size());
  return createAndManageElement(img, IMAGE, name);
}

// 创建UI元素并添加到管理列表
lv_obj_t* UIManager::createAndManageElement(lv_obj_t* element, UIElementType type, const String& name) {
  if (!element) return nullptr;
  
  UIElement uiElement;
  uiElement.object = element;
  uiElement.type = type;
  uiElement.name = name;
  
  uiElements.push_back(uiElement);
  elementMap[name] = element;
  
  Serial.printf("创建并管理UI元素: %s, 类型: %d\n", name.c_str(), type);
  return element;
}

// 隐藏所有UI元素
void UIManager::hideAllElements() {
  Serial.println("批量隐藏所有UI元素");
  
  // 批量操作前获取当前状态，减少事件触发
  bool hasChanges = false;
  for (auto& element : uiElements) {
    if (element.object && lv_obj_is_valid(element.object) && 
        !(lv_obj_has_flag(element.object, LV_OBJ_FLAG_HIDDEN))) {
      hasChanges = true;
      break;
    }
  }
  
  if (hasChanges) {
    // 批量隐藏所有可见元素
    for (auto& element : uiElements) {
      if (element.object && lv_obj_is_valid(element.object)) {
        lv_obj_add_flag(element.object, LV_OBJ_FLAG_HIDDEN);
      }
    }
  }
}

// 显示指定名称的元素
void UIManager::showElement(const String& name) {
  lv_obj_t* element = getElement(name);
  if (element && lv_obj_is_valid(element)) {
    if (lv_obj_has_flag(element, LV_OBJ_FLAG_HIDDEN)) {
      lv_obj_clear_flag(element, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(element);
      Serial.printf("显示UI元素: %s\n", name.c_str());
    }
  }
}

// 批量显示多个UI元素
void UIManager::showElements(const std::vector<String>& elementNames) {
  Serial.println("批量显示UI元素");
  
  // 强制显示所有指定的元素，无论当前状态如何
  for (const auto& name : elementNames) {
    lv_obj_t* element = getElement(name);
    if (element == nullptr) {
      Serial.printf("警告: 找不到UI元素: %s\n", name.c_str());
    } else if (!lv_obj_is_valid(element)) {
      Serial.printf("警告: UI元素无效: %s\n", name.c_str());
    } else {
      // 强制清除隐藏标志并将元素移到前台
      lv_obj_clear_flag(element, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(element);
      Serial.printf("已显示UI元素: %s\n", name.c_str());
    }
  }
}

// 隐藏指定名称的元素
void UIManager::hideElement(const String& name) {
  lv_obj_t* element = getElement(name);
  if (element && lv_obj_is_valid(element)) {
    if (!lv_obj_has_flag(element, LV_OBJ_FLAG_HIDDEN)) {
      lv_obj_add_flag(element, LV_OBJ_FLAG_HIDDEN);
      Serial.printf("隐藏UI元素: %s\n", name.c_str());
    }
  }
}

// 获取元素
lv_obj_t* UIManager::getElement(const String& name) {
  auto it = elementMap.find(name);
  if (it != elementMap.end()) {
    return it->second;
  }
  return nullptr;
}

// 获取颜色值
lv_color_t UIManager::getColorValue(ColorType colorType) {
    switch (colorType) {
        case COLOR_TEXT:
            return lv_color_hex(currentTheme.TextColor);
        case COLOR_BG:
            return lv_color_hex(currentTheme.bgColor);
        case COLOR_TIME:
            return lv_color_hex(currentTheme.timeColor);
        case COLOR_DAY:
            return lv_color_hex(currentTheme.dayColor);
        case COLOR_CHROMAKEY:
            return lv_color_hex(currentTheme.chromaKey);
        default:
            return lv_color_hex(currentTheme.TextColor);
    }
}

// 更新单个元素的颜色
void UIManager::updateElementColor(const String& name, lv_color_t color) {
  // 检查元素是否存在
  if (elementMap.find(name) == elementMap.end()) {
    Serial.println("UIManager: Element not found: " + name);
    return;
  }

  lv_obj_t* element = elementMap[name];
  if (!element || !lv_obj_is_valid(element)) {
    Serial.println("UIManager: Element invalid or deleted: " + name);
    return;
  }

  // 根据元素类型设置颜色
  for (const auto& uiElement : uiElements) {
    if (uiElement.object == element) {
      if (uiElement.type == LABEL) {
        // 检查当前颜色是否已相同，避免不必要的更新
        lv_color_t currentColor = lv_obj_get_style_text_color(element, 0);
        if (currentColor.full != color.full) {
          lv_obj_set_style_text_color(element, color, 0);
          Serial.printf("更新标签元素 %s 颜色\n", name.c_str());
        }
      } else if (uiElement.type == IMAGE) {
        // 检查当前颜色是否已相同，避免不必要的更新
        lv_color_t currentColor = lv_obj_get_style_bg_color(element, 0);
        if (currentColor.full != color.full) {
          lv_obj_set_style_bg_color(element, color, 0);
          Serial.printf("更新图像元素 %s 背景颜色\n", name.c_str());
        }
      }
      break;
    }
  }
}

void UIManager::registerElement(const String& name, lv_obj_t* element, UIElementType type) {
  // 创建UI元素结构体
  UIElement uiElement;
  uiElement.object = element;
  uiElement.type = type;
  uiElement.name = name;
  
  // 添加到管理列表
  uiElements.push_back(uiElement);
  elementMap[name] = element;
  
  Serial.println("UIManager: Registered element: " + name);
}

uint32_t UIManager::getLastOperationTime() const {
  // 实际实现中可以存储操作时间
  return 0; // 示例返回值
}