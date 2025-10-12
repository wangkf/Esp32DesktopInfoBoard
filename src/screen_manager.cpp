#include "screen_manager.h"
#include "weather_display.h"
#include "net_http.h"
#include "maoselect.h"
#include "toxicsoul.h"
#include "lvgl.h"

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
    // 隐藏天气显示组件
    hideWeatherDisplay();
    
    // 隐藏天气表格
    extern lv_obj_t* weather_table;
    if (weather_table) {
        lv_obj_add_flag(weather_table, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 隐藏天气标签
    extern lv_obj_t* weather_label;
    if (weather_label) {
        lv_obj_add_flag(weather_label, LV_OBJ_FLAG_HIDDEN);
    }
    
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
            getCityWeater();
            break;
        case NEWS_SCREEN:
            getNews();
            break;
        case MAO_SELECT_SCREEN:
            showRandomMaoSelect();
            break;
        case TOXIC_SOUL_SCREEN:
            showRandomToxicSoul();
            break;
        case ICBA_SCREEN:
            getIcibaDailyInfo();
            break;
        case ASTRONAUTS_SCREEN:
            getAstronautsInfo();
            break;
        case APRS_SCREEN:
            showAPRSData();
            break;
    }
}

/**
 * 显示天气屏幕
 */
void ScreenManager::showWeatherScreen() {
    Serial.println("切换到天气屏幕");
    
    // 显示天气标签
    extern lv_obj_t* weather_label;
    if (weather_label) {
        lv_obj_clear_flag(weather_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 获取天气数据
    getCityWeater();
    
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
    
    // 获取新闻数据
    getNews();
    
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
    
    // 获取金山词霸每日信息
    getIcibaDailyInfo();
    
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
    
    // 获取并显示宇航员信息
    getAstronautsInfo();
    
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
    
    // 显示APRS数据
    showAPRSData();
    
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
 * 显示APRS数据
 */
void ScreenManager::showAPRSData() {
    // 实现代码将在net_http.cpp中
}