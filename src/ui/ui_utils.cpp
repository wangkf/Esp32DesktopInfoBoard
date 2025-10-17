#include "ui_utils.h"
/**
 * 创建标签的通用函数实现
 */
lv_obj_t* createLabel(const lv_font_t* font, lv_color_t textColor, int xOfs, int yOfs, 
                     int height, lv_color_t bgColor, lv_opa_t bgOpacity, 
                     bool wrap, bool hidden) {
  lv_obj_t* label = lv_label_create(lv_scr_act());
  // 设置字体
  lv_obj_set_style_text_font(label, font, 0);
  // 设置文字颜色
  lv_obj_set_style_text_color(label, textColor, 0);
  // 设置宽度
  lv_obj_set_width(label, 320);
  // 如果指定了高度，则设置高度
  if (height > 0) {
    lv_obj_set_height(label, height);
  }
  // 设置背景颜色和不透明度
  lv_obj_set_style_bg_color(label, bgColor, 0);
  lv_obj_set_style_bg_opa(label, bgOpacity, 0);
  // 设置对齐方式
  lv_obj_align(label, LV_ALIGN_TOP_MID, xOfs, yOfs);
  // 设置长文本模式
  if (wrap) {
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  }
  // 设置隐藏标志
  if (hidden) {
    lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
  }
  return label;
}
/**
 * 创建图像的通用函数实现
 */
lv_obj_t* createImage(const void* src, int width, int height, int xOfs, int yOfs) {
  lv_obj_t* img = lv_img_create(lv_scr_act()); 
  // 设置图像源
  lv_img_set_src(img, src);
  // 设置尺寸
  lv_obj_set_size(img, width, height);
  // 设置对齐方式和偏移
  lv_obj_set_pos(img, xOfs, yOfs);
  // 设置隐藏标志
  lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
  return img;
}