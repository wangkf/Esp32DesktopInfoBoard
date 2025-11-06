#include "screen_manager.h"
#include "ui/ui_manager.h"
#include "network/net_http.h"
#include "content/maoselect.h"
#include "content/toxicsoul.h"
#include "content/soul.h"
#include "lvgl.h"
#include "ui/display_manager.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <ArduinoJson.h>
// 定义单例实例
ScreenManager* ScreenManager::instance = nullptr;
//*** 私有构造函数
ScreenManager::ScreenManager() : currentScreen(MAO_SELECT_SCREEN) {
    // 初始化屏幕元素指针
    screen_symbol_label = nullptr;
    screen_title_btn = nullptr;
    title_label = nullptr;
}
//*** 获取单例实例
ScreenManager* ScreenManager::getInstance() {
    if (instance == nullptr) {
        instance = new ScreenManager();
    }
    return instance;
}
//*** 初始化屏幕管理器
void ScreenManager::init() {
    // 创建3D按钮色块用于显示屏幕标题
    screen_title_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_width(screen_title_btn, 120); // 设置宽度为120px
    lv_obj_set_height(screen_title_btn, 40);
    lv_obj_align(screen_title_btn, LV_ALIGN_TOP_RIGHT, -5, 30); // 调整y坐标为30，与时间底对齐
    lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0x0000FF), 0); // 默认蓝色
    // 创建屏幕主题符号标签
    screen_symbol_label = lv_label_create(screen_title_btn);
    lv_label_set_text(screen_symbol_label, ""); // 默认显示设置图标 - 修正Unicode转义格式
    // 确保使用支持图标的字体
    lv_obj_set_style_text_font(screen_symbol_label, GBFont, 0);
    lv_obj_set_style_text_color(screen_symbol_label, lv_color_hex(0x808080), 0); // 符号标签对齐 - 调整左边距确保在可视区域内
    lv_obj_align(screen_symbol_label, LV_ALIGN_LEFT_MID, 10, 0); // 符号在左侧，左边距10px
    // 创建屏幕标题标签
    if (title_label == NULL) {
        title_label = lv_label_create(screen_title_btn);
        lv_obj_set_style_text_font(title_label, GBFont, 0);
        lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0); // 白色文字
        lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0); // 标题居中对齐
    }
}
//*** 隐藏所有屏幕元素
void ScreenManager::hideAllScreens() {
    Serial.println("使用UIManager批量隐藏所有屏幕元素");
    // 使用UIManager进行批量隐藏操作，减少重复代码和提高性能
    UIManager::getInstance()->hideAllElements();
}

//*** 显示留言板屏幕
void ScreenManager::showNoteScreen() {
    Serial.println("切换到留言板屏幕");   
    // 从文件加载并显示note内容
    ::displayNoteDataFromFile();
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新标题文本
        lv_label_set_text(title_label, "\uF075 留言板");
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0xFFA500), 0); // 橙色
    }
}
//*** 显示当前屏幕
void ScreenManager::showCurrentScreen() {
    // 根据当前屏幕状态显示相应的屏幕
    switch (currentScreen) {
        case MAO_SELECT_SCREEN:
            showMaoSelectScreen();
            break;
        case TOXIC_SOUL_SCREEN:
            showToxicSoulScreen();
            break;
        case SOUL_SCREEN:
            showSoulScreen();
            break;
        case ICIBA_SCREEN:
            showIcibaScreen();
            break;
        case ASTRONAUTS_SCREEN:
            showAstronautsScreen();
            break;
        case NEWS_SCREEN:
            showNewsScreen();
            break;
        case CALENDAR_SCREEN:
            showCalendarScreen();
            break;
        case NOTE_SCREEN:
            showNoteScreen();
            break;
        default:
            // 如果当前屏幕无效，默认显示毛选屏幕
            showMaoSelectScreen();
            break;
    }
}
//*** 显示日历屏幕
void ScreenManager::showCalendarScreen() {
    Serial.println("切换到日历屏幕");
    // 显示日历信息
    ::displayCalendar();
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新标题文本
        lv_label_set_text(title_label, "\uF073 今日日历");
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0x20B2AA), 0); // 海绿色
    }
}
//*** 切换到下一个屏幕
void ScreenManager::toggleScreen() {
    // 隐藏所有屏幕元素
    hideAllScreens();
    // 首先检查note.json文件是否存在且有内容
    bool hasNoteContent = false;
    String noteContent = "";
    // 检查note.json文件是否存在
    if (SPIFFS.exists("/note.json")) {
        File noteFile = SPIFFS.open("/note.json", "r");
        if (noteFile) {
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, noteFile);
            noteFile.close();       
            if (!error && doc.containsKey("note")) {
                noteContent = doc["note"].as<String>();
                // 检查note内容是否不为空
                if (!noteContent.isEmpty()) {
                    hasNoteContent = true;
                }
            }
        }
    }    
    // 如果有note内容，并且当前不是已经在留言板屏幕，则切换到留言板屏幕
    if (hasNoteContent && currentScreen != NOTE_SCREEN) {
        Serial.println("检测到note.json有内容，切换到留言板屏幕");
        currentScreen = NOTE_SCREEN;
    } else {
        // 定义屏幕切换顺序：新闻 -> 日历 -> 金山词霸 -> 太空宇航员 -> 毛选 -> 乌鸡汤 -> 禅语哲言 -> 新闻...
        static const ScreenState screenOrder[] = {NEWS_SCREEN, CALENDAR_SCREEN, ICIBA_SCREEN, ASTRONAUTS_SCREEN, MAO_SELECT_SCREEN, TOXIC_SOUL_SCREEN, SOUL_SCREEN};
        // 查找当前屏幕在顺序数组中的索引
        int currentIndex = 0;
        for (int i = 0; i < 7; i++) {
            if (screenOrder[i] == currentScreen) {
                currentIndex = i;
                break;
            }
        }
        // 计算下一个屏幕的索引（循环）
        int nextIndex = (currentIndex + 1) % 7;
        // 设置下一个屏幕
        currentScreen = screenOrder[nextIndex];
    }   
    // 先清空标题文本，实现"每次清空后再显示下一个"的效果
    if (title_label) {
        lv_label_set_text(title_label, "");
    }
    // 显示当前屏幕
    showCurrentScreen();
}
//*** 直接切换到指定屏幕
void ScreenManager::switchToScreen(ScreenState screenState) {
    Serial.printf("显示屏幕: %d\n", screenState);
    
    // 性能优化：使用UIManager批量隐藏所有元素
    hideAllScreens();
    
    // 更新当前屏幕状态
    currentScreen = screenState;
    
    // 获取UIManager实例
    UIManager* uiManager = UIManager::getInstance();
    
    // 根据屏幕状态显示相应的元素 - 使用批量操作优化性能
    switch(screenState) {
        case NEWS_SCREEN:
            // 显示新闻屏幕元素
            uiManager->showElement("news_label");
            break;
        case CALENDAR_SCREEN:
            // 显示日历屏幕元素 - 批量显示减少事件触发
            uiManager->showElements({"calendar_label", "calendar_img", "today_date_label"});
            break;
        case MAO_SELECT_SCREEN:
            // 显示毛选屏幕元素
            uiManager->showElements({"mao_select_label", "maoselect_img"});
            break;
        case TOXIC_SOUL_SCREEN:
            // 显示乌鸡汤屏幕元素
            uiManager->showElements({"toxic_soul_label", "toxic_soul_img"});
            break;
        case ICIBA_SCREEN:
            // 显示词霸屏幕元素
            uiManager->showElements({"iciba_label", "iciba_img"});
            break;
        case ASTRONAUTS_SCREEN:
            // 显示宇航员屏幕元素
            uiManager->showElements({"astronauts_label", "astronauts_img"});
            break;
        case SOUL_SCREEN:
            // 显示禅语哲言屏幕元素
            uiManager->showElements({"soul_label", "soul_img"});
            break;
        case NOTE_SCREEN:
            // 显示留言板屏幕元素
            uiManager->showElement("note_label");
            break;
        default:
            Serial.println("未知屏幕状态，显示默认屏幕");
            // 显示默认屏幕（例如新闻屏幕）
            uiManager->showElement("news_label");
            break;
    }
    
    // 显示当前屏幕以执行数据加载等操作
    showCurrentScreen();
}
//*** 刷新当前屏幕数据
void ScreenManager::refreshCurrentScreenData() {
    // 根据当前屏幕状态刷新数据
    switch (currentScreen) {
        case MAO_SELECT_SCREEN:
            showRandomMaoSelect();
            break;
        case TOXIC_SOUL_SCREEN:
            showRandomToxicSoul();
            break;
        case SOUL_SCREEN:
            showRandomSoul();
            break;
        case ICIBA_SCREEN:
            // 从文件加载并显示数据
            ::displayIcibaDataFromFile();
            break;
        case ASTRONAUTS_SCREEN:
            // 从文件加载并显示数据（使用成员函数确保只显示一次）
            ::displayAstronautsDataFromFile();
            // 确保宇航员标签可见
            break;
        case NEWS_SCREEN:
            // 从文件加载并显示新闻数据
            ::displayNewsDataFromFile();
            break;
        default:
            break;
    }
}
//*** 显示新闻屏幕
void ScreenManager::showNewsScreen() {
    Serial.println("切换到新闻屏幕：");
    // 从文件加载并显示新闻数据
    ::displayNewsDataFromFile();
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新标题文本
        lv_label_set_text(title_label, "\uF0AE 此刻头条");
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0x0000FF), 0); // 蓝色
    }
}
//*** 显示主席语录屏幕
void ScreenManager::showMaoSelectScreen() {
    Serial.println("切换到主席语录屏幕");
    // 显示随机一条主席语录
    showRandomMaoSelect();
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新标题文本
        lv_label_set_text(title_label, "\uF013 主席语录");
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0xFF0000), 0); // 红色
    }
}
//*** 显示乌鸡汤屏幕
void ScreenManager::showToxicSoulScreen() {
    Serial.println("切换到乌鸡汤屏幕");
    // 显示随机一条乌鸡汤
    showRandomToxicSoul();
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新标题文本
        lv_label_set_text(title_label, "\uF0C4 乌鸡汤");
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0xFFD700), 0); // 金色
    }
}
//*** 显示金山词霸每日信息屏幕
void ScreenManager::showIcibaScreen() {
    Serial.println("切换到金山词霸每日信息屏幕");
    // 从文件加载并显示金山词霸数据
    ::displayIcibaDataFromFile();
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新标题文本
        lv_label_set_text(title_label, "\uF0AC 每日一句");   
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0xFFA500), 0); // 橙色
    }
}
//*** 显示宇航员信息屏幕
void ScreenManager::showAstronautsScreen() {
    Serial.println("切换到宇航员信息屏幕：");
    // 从文件加载并显示宇航员数据
    ::displayAstronautsDataFromFile();
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新标题文本
        lv_label_set_text(title_label, "\uF0C2 太空宇航员");
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0x4B0082), 0); // 靛蓝色
    }
}
//*** 显示随机的毛主席语录
void ScreenManager::showRandomMaoSelect() {
    // 确保maoselect_img被创建并显示在上方
    extern lv_obj_t* maoselect_img;
    if (maoselect_img && lv_obj_is_valid(maoselect_img)) {
        // 显示上方图像
        lv_obj_clear_flag(maoselect_img, LV_OBJ_FLAG_HIDDEN);
    }  
    // 确保mao_select_label被创建并显示
extern lv_obj_t* mao_select_label;
if (mao_select_label && lv_obj_is_valid(mao_select_label)) {
    // 从数组中随机选择一条毛主席语录
    int count = sizeof(MaoSelect) / sizeof(MaoSelect[0]);
    int randomIndex = random(count);
    // 设置语录文本
    lv_label_set_text(mao_select_label, MaoSelect[randomIndex]);
    // 确保标签可见
    lv_obj_clear_flag(mao_select_label, LV_OBJ_FLAG_HIDDEN);
    // 确保标签显示在最上层
    lv_obj_move_foreground(mao_select_label);
}
}
//*** 显示禅语哲言屏幕
void ScreenManager::showSoulScreen() {
    Serial.println("切换到禅语哲言屏幕：");   
    // 显示随机的禅语哲言
    showRandomSoul();
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
      // 更新标题文本
        lv_label_set_text(title_label, "\uF084 禅语哲言");
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0x800080), 0); // 紫色
    }
}
//*** 显示随机的乌鸡汤
void ScreenManager::showRandomToxicSoul() {
    // 确保toxic_soul_img被创建并显示在底部
    extern lv_obj_t* toxic_soul_img;
    if (toxic_soul_img && lv_obj_is_valid(toxic_soul_img)) {
        // 显示底部图像
        lv_obj_clear_flag(toxic_soul_img, LV_OBJ_FLAG_HIDDEN);
    }  
    // 确保toxic_soul_label被创建并显示
extern lv_obj_t* toxic_soul_label;
if (toxic_soul_label && lv_obj_is_valid(toxic_soul_label)) {
    // 从数组中随机选择一条乌鸡汤
    int count = sizeof(ToxicSoul) / sizeof(ToxicSoul[0]);
    int randomIndex = random(count);
    // 设置乌鸡汤文本
    lv_label_set_text(toxic_soul_label, ToxicSoul[randomIndex]);
    // 确保标签可见
    lv_obj_clear_flag(toxic_soul_label, LV_OBJ_FLAG_HIDDEN);
    // 确保标签显示在最上层
    lv_obj_move_foreground(toxic_soul_label);
  }
}
//*** 显示随机的禅语哲言
void ScreenManager::showRandomSoul() {
    // 确保soul_img被创建并显示在底部
    extern lv_obj_t* soul_img;
    if (soul_img && lv_obj_is_valid(soul_img)) {
        // 显示底部图像
        lv_obj_clear_flag(soul_img, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 确保soul_label被创建并显示
extern lv_obj_t* soul_label;
if (soul_label && lv_obj_is_valid(soul_label)) {
    // 从数组中随机选择一条禅语哲言
    int count = sizeof(Soul) / sizeof(Soul[0]);
    int randomIndex = random(count);
    
    // 设置禅语哲言文本
    lv_label_set_text(soul_label, Soul[randomIndex]);
    
    // 确保标签可见
    lv_obj_clear_flag(soul_label, LV_OBJ_FLAG_HIDDEN);
    
    // 确保标签显示在最上层
    lv_obj_move_foreground(soul_label);
  }
}

//*** 设置配置模式图标状态
void ScreenManager::setConfigIconStatus(bool isConfigMode) {
    if (screen_symbol_label) {
        if (isConfigMode) {
            lv_label_set_text(screen_symbol_label, "⚙️"); // 配置模式图标
        } else {
            lv_label_set_text(screen_symbol_label, ""); // 普通模式下不显示图标
        }
    }
}