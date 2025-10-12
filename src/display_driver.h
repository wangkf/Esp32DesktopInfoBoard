/**
 * 显示屏驱动头文件
 * 适用于ESP32桌面信息牌项目
 */

#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include "lvgl.h"

// 初始化显示屏
void initDisplay();

// 显示屏刷新回调函数
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);

// 触摸屏读取回调函数（如果有触摸功能）
void my_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

// 更新LVGL显示驱动配置
void updateLVGLDisplayDriver(lv_disp_drv_t *disp_drv);

// 创建触摸输入设备（如果有触摸功能）
void createTouchInputDevice();

#endif /* DISPLAY_DRIVER_H */