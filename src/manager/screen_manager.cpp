#include "screen_manager.h"
#include "ui/weather_display.h"
#include "network/net_http.h"
#include "maoselect.h"
#include "toxicsoul.h"
#include "lvgl.h"
#include "ui/display_manager.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// 外部变量声明
extern bool forceRefreshNews;
extern bool forceRefreshWeather;

// 定义单例实例
ScreenManager* ScreenManager::instance = nullptr;

/**
 * 私有构造函数
 */
ScreenManager::ScreenManager() : currentScreen(WEATHER_SCREEN) {
    // 初始化屏幕元素指针
    screen_symbol_label = nullptr;
    screen_title_btn = nullptr;
    title_label = nullptr;
}

/**
 * 获取单例实例
 */
ScreenManager* ScreenManager::getInstance() {
    if (instance == nullptr) {
        instance = new ScreenManager();
    }
    return instance;
}

/**
 * 初始化屏幕管理器
 */
void ScreenManager::init() {
    // 创建3D按钮色块用于显示屏幕标题
    screen_title_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_width(screen_title_btn, 120); // 设置宽度为120px
    lv_obj_set_height(screen_title_btn, 40);
    lv_obj_align(screen_title_btn, LV_ALIGN_TOP_RIGHT, -5, 30); // 调整y坐标为30，与时间底对齐
    lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0x0000FF), 0); // 默认蓝色

    // 创建屏幕主题符号标签
    screen_symbol_label = lv_label_create(screen_title_btn);
    lv_label_set_text(screen_symbol_label, "/uF013"); // 默认显示设置图标
    // 确保使用支持图标的字体
    lv_obj_set_style_text_font(screen_symbol_label, &lvgl_font_song_16, 0);
    lv_obj_set_style_text_color(screen_symbol_label, lv_color_hex(0x808080), 0); // 默认灰色文字
    lv_obj_align(screen_symbol_label, LV_ALIGN_LEFT_MID, 220, 0); // 符号在左侧，增加左边距

    // 创建屏幕标题标签
    if (title_label == NULL) {
        title_label = lv_label_create(screen_title_btn);
        lv_obj_set_style_text_font(title_label, &lvgl_font_song_16, 0);
        lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0); // 白色文字
        lv_obj_align(title_label, LV_ALIGN_RIGHT_MID, -5, 0); // 标题靠右对齐，右侧边距5px
    }
}

/**
 * 隐藏所有屏幕元素
 */
void ScreenManager::hideAllScreens() {
    Serial.println("隐藏所有屏幕元素");
    
    // 隐藏天气标签和天气显示组件
    extern lv_obj_t* weather_label;
    if (weather_label) {
        lv_obj_add_flag(weather_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 隐藏天气表格
extern lv_obj_t* weather_table;
if (weather_table) {
    lv_obj_add_flag(weather_table, LV_OBJ_FLAG_HIDDEN);
}
    
    // 调用WeatherDisplay的hide方法隐藏所有天气组件
    weatherDisplay.hide();
    
    // 隐藏新闻标签
extern lv_obj_t* news_label;
if (news_label) {
    lv_obj_add_flag(news_label, LV_OBJ_FLAG_HIDDEN);
}
    
    // 隐藏毛选标签
extern lv_obj_t* mao_select_label;
if (mao_select_label) {
    lv_obj_add_flag(mao_select_label, LV_OBJ_FLAG_HIDDEN);
}
    
    // 隐藏乌鸡汤标签
extern lv_obj_t* toxic_soul_label;
if (toxic_soul_label) {
    lv_obj_add_flag(toxic_soul_label, LV_OBJ_FLAG_HIDDEN);
}
    
    // 隐藏金山词霸标签
extern lv_obj_t* iciba_label;
if (iciba_label) {
    lv_obj_add_flag(iciba_label, LV_OBJ_FLAG_HIDDEN);
}
    
    // 隐藏宇航员信息标签
extern lv_obj_t* astronauts_label;
if (astronauts_label) {
    lv_obj_add_flag(astronauts_label, LV_OBJ_FLAG_HIDDEN);
}
    
    // 隐藏APRS相关标签
extern lv_obj_t* aprs_label;
extern lv_obj_t* aprs_table;
if (aprs_label) {
    lv_obj_add_flag(aprs_label, LV_OBJ_FLAG_HIDDEN);
}
if (aprs_table) {
    lv_obj_add_flag(aprs_table, LV_OBJ_FLAG_HIDDEN);
}
}

/**
 * 切换到下一个屏幕
 */
void ScreenManager::toggleScreen() {
    // 隐藏所有屏幕元素
    hideAllScreens();
    
    // 循环切换屏幕状态：新闻 -> 毛选 -> 乌鸡汤 -> 金山词霸 -> 宇航员信息 -> APRS信息 -> 新闻...
    // 注：已屏蔽天气屏幕
    if (currentScreen == WEATHER_SCREEN) {
        currentScreen = NEWS_SCREEN; // 如果当前是天气屏幕，直接跳转到新闻屏幕
    } else {
        currentScreen = static_cast<ScreenState>((currentScreen % 7) + 1);
        if (currentScreen == WEATHER_SCREEN) {
            currentScreen = NEWS_SCREEN; // 跳过天气屏幕
        }
    }
    
    // 根据当前屏幕状态显示相应的屏幕
    switch (currentScreen) {
        case NEWS_SCREEN:
            showNewsScreen();
            break;
        case MAO_SELECT_SCREEN:
            showMaoSelectScreen();
            break;
        case TOXIC_SOUL_SCREEN:
            showToxicSoulScreen();
            break;
        case ICBA_SCREEN:
            showIcibaScreen();
            break;
        case ASTRONAUTS_SCREEN:
            showAstronautsScreen();
            break;
        case WEATHER_SCREEN:
        default:
            showNewsScreen(); // 默认显示新闻屏幕（如果意外进入天气屏幕状态）
            break;
    }
}

/**
 * 直接切换到指定屏幕
 */
void ScreenManager::switchToScreen(ScreenState screenState) {
    // 隐藏所有屏幕元素
    hideAllScreens();
    
    // 更新当前屏幕状态
    if (screenState == WEATHER_SCREEN) {
        currentScreen = NEWS_SCREEN; // 屏蔽天气屏幕，默认设置为新闻屏幕
    } else {
        currentScreen = screenState;
    }
    
    // 根据当前屏幕状态显示相应的屏幕
    switch (currentScreen) {
        case NEWS_SCREEN:
            showNewsScreen();
            break;
        case MAO_SELECT_SCREEN:
            showMaoSelectScreen();
            break;
        case TOXIC_SOUL_SCREEN:
            showToxicSoulScreen();
            break;
        case ICBA_SCREEN:
            showIcibaScreen();
            break;
        case ASTRONAUTS_SCREEN:
            showAstronautsScreen();
            break;
    }
}

/**
 * 获取当前屏幕状态
 */
ScreenState ScreenManager::getCurrentScreen() {
    return currentScreen;
}

/**
 * 刷新当前屏幕数据
 */
void ScreenManager::refreshCurrentScreenData() {
    // 根据当前屏幕状态刷新数据
    switch (currentScreen) {
        case WEATHER_SCREEN:
            // 天气屏幕已被屏蔽
            Serial.println("天气屏幕已被屏蔽，不刷新天气数据");
            break;
        case NEWS_SCREEN:
            // 从文件加载并显示数据
            ::displayNewsDataFromFile();
            break;
        case MAO_SELECT_SCREEN:
            showRandomMaoSelect();
            break;
        case TOXIC_SOUL_SCREEN:
            showRandomToxicSoul();
            break;
        case ICBA_SCREEN:
            // 从文件加载并显示数据
            ::displayIcibaDataFromFile();
            break;
        case ASTRONAUTS_SCREEN:
            // 从文件加载并显示数据
            displayAstronautsDataFromFile();
            break;
    }
}

/**
 * 显示天气屏幕
 */
void ScreenManager::showWeatherScreen() {
    Serial.println("切换到天气屏幕");
    
    // 确保隐藏新闻标签，防止内容重叠
extern lv_obj_t* news_label;
if (news_label) {
    lv_obj_add_flag(news_label, LV_OBJ_FLAG_HIDDEN);
}
    
    // 显示天气标签
extern lv_obj_t* weather_label;
if (weather_label) {
    lv_obj_clear_flag(weather_label, LV_OBJ_FLAG_HIDDEN);
}
    
    // 首先尝试从文件显示天气数据
    ::displayWeatherDataFromFile();
    
    // 如果没有缓存数据或需要强制刷新，则从网络获取数据
    if (forceRefreshWeather || !SPIFFS.exists("/weather.json")) {
        // 获取天气数据
        getCityWeather();
        forceRefreshWeather = false;
    }
    
    // 显示天气图标和图表
    initWeatherDisplay();
    showWeatherDisplay();
    
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新符号
        lv_label_set_text(screen_symbol_label, "");
        // 更新标题文本
        lv_label_set_text(title_label, " 天气信息");
        
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0x0000FF), 0); // 蓝色
    }
}

/**
 * 显示新闻屏幕
 */
void ScreenManager::showNewsScreen() {
    Serial.println("切换到新闻屏幕");
    
    // 显示新闻标签
extern lv_obj_t* news_label;
if (news_label) {
    lv_obj_clear_flag(news_label, LV_OBJ_FLAG_HIDDEN);
}
    
    // 首先尝试从文件显示新闻数据
    ::displayNewsDataFromFile();
    
    // 如果没有缓存数据或需要强制刷新，则从网络获取数据
    if (forceRefreshNews || !SPIFFS.exists("/news.json")) {
        // 获取新闻数据
        getNews();
        forceRefreshNews = false;
    }
    
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新符号
        lv_label_set_text(screen_symbol_label, "");
        // 更新标题文本
        lv_label_set_text(title_label, " 新闻简报");
        
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0x800080), 0); // 紫色
    }
}

/**
 * 显示主席语录屏幕
 */
void ScreenManager::showMaoSelectScreen() {
    Serial.println("切换到主席语录屏幕");
    
    // 显示随机的主席语录
    showRandomMaoSelect();
    
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新符号
        lv_label_set_text(screen_symbol_label, "");
        // 更新标题文本
        lv_label_set_text(title_label, " 毛主席语录");
        
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0xFF0000), 0); // 红色
    }
}

/**
 * 显示乌鸡汤屏幕
 */
void ScreenManager::showToxicSoulScreen() {
    Serial.println("切换到乌鸡汤屏幕");
    
    // 显示随机的乌鸡汤
    showRandomToxicSoul();
    
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新符号
        lv_label_set_text(screen_symbol_label, "");
        // 更新标题文本
        lv_label_set_text(title_label, " 心灵鸡汤");
        
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0x008000), 0); // 绿色
    }
}

/**
 * 显示金山词霸每日信息屏幕
 */
void ScreenManager::showIcibaScreen() {
    Serial.println("切换到金山词霸每日信息屏幕");
    
    // 显示金山词霸标签
extern lv_obj_t* iciba_label;
if (iciba_label) {
    lv_obj_clear_flag(iciba_label, LV_OBJ_FLAG_HIDDEN);
}
    
    // 首先尝试从文件显示金山词霸数据
    ::displayIcibaDataFromFile();
    
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新符号
        lv_label_set_text(screen_symbol_label, "");
        // 更新标题文本
        lv_label_set_text(title_label, " 每日一句");
        
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0xFFA500), 0); // 橙色
    }
}

/**
 * 显示宇航员信息屏幕
 */
void ScreenManager::showAstronautsScreen() {
    Serial.println("切换到宇航员信息屏幕");
    
    // 创建并显示宇航员标签
extern lv_obj_t* astronauts_label;
if (!astronauts_label) {
    astronauts_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(astronauts_label, &lvgl_font_song_16, 0); // 16px字体
    lv_obj_set_style_text_color(astronauts_label, lv_color_hex(0x000000), 0); // 黑色文字
    lv_obj_set_width(astronauts_label, screenWidth - 20); // 宽度为屏幕宽度减20px
    lv_obj_set_height(astronauts_label, screenHeight - 120); // 固定高度，顶部100px，留出20px边距
    lv_obj_align(astronauts_label, LV_ALIGN_TOP_LEFT, 10, 100); // 顶部离屏幕顶部100px
    lv_label_set_text(astronauts_label, "获取宇航员信息中...");
    lv_label_set_long_mode(astronauts_label, LV_LABEL_LONG_WRAP); // 自动换行
    lv_obj_set_style_radius(astronauts_label, 10, 0); // 设置圆角
    lv_obj_set_style_bg_color(astronauts_label, lv_color_hex(0xFFFFFF), 0); // 白色背景
    lv_obj_set_style_bg_opa(astronauts_label, 100, 0); // 设置背景透明度
}
    
    // 确保宇航员标签可见
    lv_obj_clear_flag(astronauts_label, LV_OBJ_FLAG_HIDDEN);
    
    // 只从文件显示宇航员数据
    displayAstronautsDataFromFile();
    
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新符号
        lv_label_set_text(screen_symbol_label, "");
        // 更新标题文本
        lv_label_set_text(title_label, " 太空宇航员");
        
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0x4B0082), 0); // 靛蓝色
    }
}

    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新符号
        lv_label_set_text(screen_symbol_label, "");
        // 更新标题文本
        lv_label_set_text(title_label, " APRS信息");
        
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0x800000), 0); // 栗色
    }
}

/**
 * 显示随机的毛主席语录
 */
void ScreenManager::showRandomMaoSelect() {
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

/**
 * 显示随机的乌鸡汤
 */
void ScreenManager::showRandomToxicSoul() {
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

/**
 * 从文件显示宇航员数据
 */
void ScreenManager::displayAstronautsDataFromFile() {
    Serial.println("尝试从文件加载宇航员数据");
    
    // 直接调用UI模块的displayAstronautsDataFromFile函数来显示数据
    // 此函数已经实现了按飞船类别分类显示宇航员的功能
    ::displayAstronautsDataFromFile();
}

/**
 * 从文件显示APRS数据
 */
void ScreenManager::displayAPRSDataFromFile() {
    Serial.println("尝试从文件加载APRS数据");
    
    // 直接调用UI模块的displayAPRSDataFromFile函数来显示数据
    ::displayAPRSDataFromFile();
}

/**
 * 显示APRS数据
 */
void ScreenManager::showAPRSData() {
    // 这个函数在net_http.cpp中有实现
    Serial.println("尝试从网络获取APRS数据");
}

/**
 * 设置配置模式图标状态
 */
void ScreenManager::setConfigIconStatus(bool isConfigMode) {
    if (screen_symbol_label) {
        lv_label_set_text(screen_symbol_label, "\uF013"); // 设置图标为设置图标
        if (isConfigMode) {
            lv_obj_set_style_text_color(screen_symbol_label, lv_color_hex(0x000000), 0); // 黑色文字
        } else {
            lv_obj_set_style_text_color(screen_symbol_label, lv_color_hex(0x808080), 0); // 灰色文字
        }
    }
}