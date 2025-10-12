#ifndef LOCAL_DISPLAY_H
#define LOCAL_DISPLAY_H

#include <lvgl.h>
#include <Arduino.h>
#include "config.h" // 包含配置文件
#include "toxicsoul.h"
#include "maoselect.h"
//#include "soul.h"
// 声明全局变量
extern lv_obj_t* mao_select_label;
extern lv_obj_t* toxic_soul_label;
extern lv_obj_t* weather_table;
extern lv_obj_t* news_label;

// 内容类型枚举
enum ContentType {
  MAO_SELECT,    // 主席语录
  TOXIC_SOUL     // 乌鸡汤
};

// 声明全局函数
extern void showRandomMaoSelect();
extern void showRandomToxicSoul();
extern void showRandomContent(ContentType type);

// 函数实现

/**
 * 根据类型显示随机内容
 * @param type 内容类型（MAO_SELECT或TOXIC_SOUL）
 */
void showRandomContent(ContentType type) {
  // 根据类型选择标签和内容
  lv_obj_t** current_label = (type == MAO_SELECT) ? &mao_select_label : &toxic_soul_label;
  const char* title = (type == MAO_SELECT) ? "主席语录：\n" : "今日乌鸡汤：\n";
  const char* const* content_array = (type == MAO_SELECT) ? MaoSelect : ToxicSoul;
  int content_count = (type == MAO_SELECT) ? MaoSelectCount : ToxicSoulCount;
  const char* array_name = (type == MAO_SELECT) ? "MaoSelect" : "ToxicSoul";
  
  // 创建标签（如果尚未创建或无效）
  if (!(*current_label) || !lv_obj_is_valid(*current_label)) {
    if (*current_label) {
      // 确保旧标签被正确清理
      lv_obj_del(*current_label);
    }
    *current_label = lv_label_create(lv_scr_act());
    lv_label_set_long_mode(*current_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_size(*current_label, screenWidth - 20, screenHeight - 120); // 固定高度，顶部100px，留出20px边距
    lv_obj_align(*current_label, LV_ALIGN_TOP_LEFT, 10, 100); // 顶部离屏幕顶部100px
    lv_obj_set_style_text_font(*current_label, &lvgl_font_song_16, 0);
    lv_obj_set_style_text_color(*current_label, lv_color_hex(0x333333), 0); // 深灰色文字
    
    // 添加背景和边框效果（与天气标签风格统一）
    lv_obj_set_style_bg_color(*current_label, lv_color_hex(0xFFFFFF), 0); // 白色背景
    lv_obj_set_style_bg_opa(*current_label, 100, 0); // 设置背景透明度
    lv_obj_set_style_border_width(*current_label, 2, 0); // 边框宽度
    lv_obj_set_style_border_color(*current_label, lv_color_hex(0xCCCCCC), 0); // 灰色边框
    lv_obj_set_style_radius(*current_label, 10, 0); // 圆角
    lv_obj_set_style_pad_all(*current_label, 10, 0); // 内边距
  }
  
  // 隐藏其他屏幕元素
  if (weather_table) {
    lv_obj_add_flag(weather_table, LV_OBJ_FLAG_HIDDEN);
  }
  if (news_label) {
    lv_obj_add_flag(news_label, LV_OBJ_FLAG_HIDDEN);
  }
  // 隐藏另一个标签
  if (type == MAO_SELECT && toxic_soul_label) {
    lv_obj_add_flag(toxic_soul_label, LV_OBJ_FLAG_HIDDEN);
  } else if (type == TOXIC_SOUL && mao_select_label) {
    lv_obj_add_flag(mao_select_label, LV_OBJ_FLAG_HIDDEN);
  }
  
  // 显示当前标签
  lv_obj_clear_flag(*current_label, LV_OBJ_FLAG_HIDDEN);
  
  // 从数组中随机选择一条内容显示
  if (content_count > 0) {
    int randomIndex = random(content_count);
    const char* randomContent = content_array[randomIndex];
    
    // 添加标题，与第二屏排版保持一致
    String displayText = title;
    displayText += randomContent;
    
    lv_label_set_text(*current_label, displayText.c_str());
    Serial.printf("显示随机%s内容，索引：%d\n", array_name, randomIndex);
  } else {
    String displayText = title;
    displayText += "暂无内容";
    lv_label_set_text(*current_label, displayText.c_str());
    Serial.printf("错误：%s数组为空\n", array_name);
  }
}

/**
 * 显示随机毛泽东选集内容
 */
void showRandomMaoSelect() {
  showRandomContent(MAO_SELECT);
}

/**
 * 显示随机毒鸡汤内容
 */
void showRandomToxicSoul() {
  showRandomContent(TOXIC_SOUL);
}

#endif // LOCAL_DISPLAY_H