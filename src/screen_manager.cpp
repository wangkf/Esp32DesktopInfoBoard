#include "screen_manager.h"
#include "weather_display.h"
#include "net_http.h"
#include "maoselect.h"
#include "toxicsoul.h"
#include "lvgl.h"
#include "display_manager.h"
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
    lv_label_set_text(screen_symbol_label, ""); // 不显示符号
    lv_obj_set_style_text_font(screen_symbol_label, &lv_font_montserrat_24, 0); // 24px字体
    lv_obj_set_style_text_color(screen_symbol_label, lv_color_hex(0xFFFFFF), 0); // 白色文字
    lv_obj_align(screen_symbol_label, LV_ALIGN_LEFT_MID, 0, 0); // 符号在左侧，左移10px(10→0)

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
    
    // 循环切换屏幕状态：天气 -> 新闻 -> 毛选 -> 乌鸡汤 -> 金山词霸 -> 宇航员信息 -> APRS信息 -> 天气...
    currentScreen = static_cast<ScreenState>((currentScreen + 1) % 7);
    
    // 根据当前屏幕状态显示相应的屏幕
    switch (currentScreen) {
        case WEATHER_SCREEN:
            showWeatherScreen();
            break;
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
        case APRS_SCREEN:
            showAPRSScreen();
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
    currentScreen = screenState;
    
    // 根据当前屏幕状态显示相应的屏幕
    switch (currentScreen) {
        case WEATHER_SCREEN:
            showWeatherScreen();
            break;
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
        case APRS_SCREEN:
            showAPRSScreen();
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
            // 检查WiFi连接状态
            if (WiFi.status() == WL_CONNECTED) {
                // 从网络获取数据并写入文件
                getCityWeater();
            }
            // 从文件加载并显示数据
            displayWeatherDataFromFile();
            break;
        case NEWS_SCREEN:
            // 检查WiFi连接状态
            if (WiFi.status() == WL_CONNECTED) {
                // 从网络获取数据并写入文件
                getNews();
            }
            // 从文件加载并显示数据
            displayNewsDataFromFile();
            break;
        case MAO_SELECT_SCREEN:
            showRandomMaoSelect();
            break;
        case TOXIC_SOUL_SCREEN:
            showRandomToxicSoul();
            break;
        case ICBA_SCREEN:
            // 检查WiFi连接状态
            if (WiFi.status() == WL_CONNECTED) {
                // 从网络获取数据并写入文件
                getIcibaDailyInfo();
            }
            // 从文件加载并显示数据
            displayIcibaDataFromFile();
            break;
        case ASTRONAUTS_SCREEN:
            // 检查WiFi连接状态
            if (WiFi.status() == WL_CONNECTED) {
                // 从网络获取数据并写入文件
                getAstronautsInfo();
            }
            // 从文件加载并显示数据
            displayAstronautsDataFromFile();
            break;
        case APRS_SCREEN:
            // 检查WiFi连接状态
            if (WiFi.status() == WL_CONNECTED) {
                // 从网络获取数据并写入文件
                showAPRSData();
            }
            // 从文件加载并显示数据
            displayAPRSDataFromFile();
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
    displayWeatherDataFromFile();
    
    // 如果没有缓存数据或需要强制刷新，则从网络获取数据
    if (forceRefreshWeather || !SPIFFS.exists("/weather_data.json")) {
        // 获取天气数据
        getCityWeater();
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
        lv_label_set_text(title_label, "\uF0C2 天气信息");
        
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
    displayNewsDataFromFile();
    
    // 如果没有缓存数据或需要强制刷新，则从网络获取数据
    if (forceRefreshNews || !SPIFFS.exists("/news_data.json")) {
        // 获取新闻数据
        getNews();
        forceRefreshNews = false;
    }
    
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新符号
        lv_label_set_text(screen_symbol_label, "");
        // 更新标题文本
        lv_label_set_text(title_label, "\uF0A1 新闻简报");
        
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
        lv_label_set_text(title_label, "\uF2DE 毛主席语录");
        
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
        lv_label_set_text(title_label, "\uF281 心灵鸡汤");
        
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
    displayIcibaDataFromFile();
    
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新符号
        lv_label_set_text(screen_symbol_label, "");
        // 更新标题文本
        lv_label_set_text(title_label, "\uF0AC 每日一句");
        
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
    
    // 首先尝试从文件显示宇航员数据
    displayAstronautsDataFromFile();
    
    // 外部变量声明
    extern unsigned long lastAstronautsUpdateTime;
    
    // 如果没有缓存数据或超过更新间隔，则从网络获取数据
    const unsigned long ASTRONAUTS_UPDATE_INTERVAL = 24 * 60 * 60 * 1000; // 24小时
    bool shouldUpdate = (millis() - lastAstronautsUpdateTime >= ASTRONAUTS_UPDATE_INTERVAL) || lastAstronautsUpdateTime == 0;
    
    // 检查WiFi连接状态
    if (WiFi.status() == WL_CONNECTED && (shouldUpdate || !SPIFFS.exists("/astronauts.json"))) {
        // 获取并显示宇航员信息
        getAstronautsInfo();
    } else if (!SPIFFS.exists("/astronauts.json")) {
        // 如果没有缓存文件，显示提示信息
        lv_label_set_text(astronauts_label, "暂无宇航员数据，请确保WiFi已连接以获取最新数据");
    }
    
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新符号
        lv_label_set_text(screen_symbol_label, "");
        // 更新标题文本
        lv_label_set_text(title_label, "\uF2D6 太空宇航员");
        
        // 更新色块颜色
        lv_obj_set_style_bg_color(screen_title_btn, lv_color_hex(0x4B0082), 0); // 靛蓝色
    }
}



/**
 * 显示APRS信息屏幕
 */
void ScreenManager::showAPRSScreen() {
    Serial.println("切换到APRS信息屏幕");
    
    // 创建并显示APRS标签（如果尚未创建）
    extern lv_obj_t* aprs_label;
    if (!aprs_label) {
        aprs_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(aprs_label, &lvgl_font_song_16, 0); // 16px字体
        lv_obj_set_style_text_color(aprs_label, lv_color_hex(0x000000), 0); // 黑色文字
        lv_obj_set_width(aprs_label, screenWidth - 20); // 宽度为屏幕宽度减20px
        lv_obj_align(aprs_label, LV_ALIGN_TOP_LEFT, 10, 70); // 左上角对齐，边距10,70
        lv_label_set_text(aprs_label, "正在连接APRS服务器...");
        lv_label_set_long_mode(aprs_label, LV_LABEL_LONG_WRAP); // 自动换行
    }
    
    // 确保APRS标签可见
    lv_obj_clear_flag(aprs_label, LV_OBJ_FLAG_HIDDEN);
    
    // 创建并显示APRS表格（如果尚未创建）
    extern lv_obj_t* aprs_table;
    if (!aprs_table) {
        aprs_table = lv_table_create(lv_scr_act());
        lv_table_set_col_cnt(aprs_table, 2); // 2列：呼号、信息
        lv_table_set_row_cnt(aprs_table, APRS_MAX_PACKETS + 1); // +1 用于表头
        
        // 设置表格样式
        lv_obj_set_style_text_font(aprs_table, &lvgl_font_song_16, 0);
        lv_obj_set_style_text_color(aprs_table, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_color(aprs_table, lv_color_hex(0xFFFFFF), 0);
        
        // 设置表格宽度为屏幕宽度
        lv_obj_set_width(aprs_table, screenWidth - 20);
        lv_obj_set_height(aprs_table, 300); // 设置表格高度
        
        // 设置列宽，适应屏幕宽度
        lv_table_set_col_width(aprs_table, 0, (screenWidth - 30) * 0.30); // 呼号列
        lv_table_set_col_width(aprs_table, 1, (screenWidth - 30) * 0.70); // 信息列
        
        // 设置表格位置（在标签下方）
        lv_obj_align(aprs_table, LV_ALIGN_TOP_MID, 0, 100);
    }
    
    // 确保APRS表格可见
    lv_obj_clear_flag(aprs_table, LV_OBJ_FLAG_HIDDEN);
    
    // 首先尝试从文件显示APRS数据
    displayAPRSDataFromFile();
    
    // 如果没有缓存数据或者WiFi已连接，则从网络获取数据
    if (!SPIFFS.exists("/aprs.json") && WiFi.status() == WL_CONNECTED) {
        // 显示APRS数据
        showAPRSData();
    } else if (!SPIFFS.exists("/aprs.json")) {
        // 如果没有缓存文件，显示提示信息
        if (aprs_label) {
            lv_label_set_text(aprs_label, "暂无APRS数据，请确保WiFi已连接以获取最新数据");
        }
    }
    
    // 更新屏幕标题和符号
    if (screen_symbol_label && screen_title_btn && title_label) {
        // 更新符号
        lv_label_set_text(screen_symbol_label, "");
        // 更新标题文本
        lv_label_set_text(title_label, "\uF279 APRS信息");
        
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
    extern lv_obj_t* astronauts_label;
    
    // 检查文件是否存在
    if (SPIFFS.exists("/astronauts.json")) {
      File file = SPIFFS.open("/astronauts.json", "r");
        if (file) {
            // 读取文件内容
            String jsonString = file.readString();
            file.close();
            
            // 解析JSON数据
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, jsonString);
            
            if (!error) {
                Serial.println("成功解析宇航员数据文件");
                
                // 构建显示内容
                String displayText = "太空宇航员信息：\n\n";
                
                if (doc["number"].is<int>() && doc["people"].is<JsonArray>()) {
                    int number = doc["number"];
                    displayText += "当前太空总人数：" + String(number) + " 人\n\n";
                    
                    // 遍历所有宇航员
                    for (JsonObject person : doc["people"].as<JsonArray>()) {
                        if (person["name"].is<const char*>() && person["craft"].is<const char*>()) {
                            const char* name = person["name"];
                            const char* craft = person["craft"];
                            displayText += "姓名：" + String(name) + "\n";
                            displayText += "所在飞船：" + String(craft) + "\n\n";
                        }
                    }
                }
                
                // 显示内容
                if (astronauts_label && displayText.length() > 0) {
                    lv_label_set_text(astronauts_label, displayText.c_str());
                    Serial.println("成功显示文件中的宇航员数据");
                }
            } else {
                Serial.println("解析宇航员数据文件失败");
            }
        } else {
            Serial.println("无法打开宇航员数据文件");
        }
    } else {
        Serial.println("宇航员数据文件不存在");
    }
}

/**
 * 从文件显示APRS数据
 */
void ScreenManager::displayAPRSDataFromFile() {
    Serial.println("尝试从文件加载APRS数据");
    extern lv_obj_t* aprs_label;
    extern lv_obj_t* aprs_table;
    
    // 检查文件是否存在
    if (SPIFFS.exists("/aprs.json")) {
      File file = SPIFFS.open("/aprs.json", "r");
        if (file) {
            // 读取文件内容
            String jsonString = file.readString();
            file.close();
            
            // 解析JSON数据
            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, jsonString);
            
            if (!error) {
                Serial.println("成功解析APRS数据文件");
                
                // 更新标签显示
                if (aprs_label) {
                    lv_label_set_text(aprs_label, "APRS数据（来自本地缓存）");
                }
                
                // 处理表格显示
                if (aprs_table) {
                    // 设置表头
                    lv_table_set_cell_value(aprs_table, 0, 0, "呼号");
                    lv_table_set_cell_value(aprs_table, 0, 1, "信息");
                    
                    // 设置表头样式
                    lv_table_add_cell_ctrl(aprs_table, 0, 0, LV_TABLE_CELL_CTRL_CUSTOM_1);
                    lv_table_add_cell_ctrl(aprs_table, 0, 1, LV_TABLE_CELL_CTRL_CUSTOM_1);
                    
                    // 遍历APRS数据
                    int row = 1;
                    if (doc["packets"].is<JsonArray>()) {
                        for (JsonObject packet : doc["packets"].as<JsonArray>()) {
                            if (row >= APRS_MAX_PACKETS + 1) break; // 不超过表格最大行数
                            
                            // 获取呼号和信息
                            const char* callsign = packet["callsign"].is<const char*>() ? packet["callsign"].as<const char*>() : "未知";
                            const char* message = packet["message"].is<const char*>() ? packet["message"].as<const char*>() : "无信息";
                            
                            // 设置表格内容
                            lv_table_set_cell_value(aprs_table, row, 0, callsign);
                            lv_table_set_cell_value(aprs_table, row, 1, message);
                            
                            row++;
                        }
                    }
                    
                    // 如果没有数据，显示提示
                    if (row == 1) {
                        lv_table_set_cell_value(aprs_table, 1, 0, "暂无数据");
                        lv_table_set_cell_value(aprs_table, 1, 1, "");
                    }
                }
                
                Serial.println("成功显示文件中的APRS数据");
            } else {
                Serial.println("解析APRS数据文件失败");
                if (aprs_label) {
                    lv_label_set_text(aprs_label, "本地APRS数据解析失败");
                }
            }
        } else {
            Serial.println("无法打开APRS数据文件");
            if (aprs_label) {
                lv_label_set_text(aprs_label, "无法打开本地APRS数据文件");
            }
        }
    } else {
        Serial.println("APRS数据文件不存在");
        if (aprs_label) {
            lv_label_set_text(aprs_label, "本地APRS数据文件不存在");
        }
    }
}

/**
 * 显示APRS数据
 */
void ScreenManager::showAPRSData() {
    // 这个函数在net_http.cpp中有实现
    Serial.println("尝试从网络获取APRS数据");
}