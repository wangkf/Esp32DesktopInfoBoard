#ifndef UI_UTILS_H
#define UI_UTILS_H

#include <lvgl.h>

/**
 * 创建标签的通用函数
 * 
 * @param font 字体
 * @param textColor 文字颜色
 * @param width 宽度
 * @param align 对齐方式
 * @param xOfs X轴偏移
 * @param yOfs Y轴偏移
 * @param height 高度（默认0，表示自适应）
 * @param bgColor 背景颜色（默认黑色）
 * @param bgOpacity 背景不透明度（默认透明）
 * @param wrap 是否自动换行（默认true）
 * @param hidden 是否默认隐藏（默认true）
 * @return 创建的标签对象指针
 */
lv_obj_t* createLabel(const lv_font_t* font, lv_color_t textColor, int xOfs, int yOfs, 
                     int height = 0, lv_color_t bgColor = lv_color_hex(0x000000), lv_opa_t bgOpacity = LV_OPA_TRANSP, 
                     bool wrap = true, bool hidden = true);

/**
 * 创建图像的通用函数
 * 
 * @param src 图像源
 * @param height 高度
 * @param xOfs X轴偏移
 * @param yOfs Y轴偏移
 * @param hidden 是否默认隐藏（默认true）
 * @return 创建的图像对象指针
 */
lv_obj_t* createImage(const void* src, int width, int height, int xOfs, int yOfs);

#endif // UI_UTILS_H