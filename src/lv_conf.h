/**
 * LVGL配置文件
 * 用于ESP32桌面信息牌项目
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/* 版本检查 - 由于在platformio.ini中已定义LV_CONF_INCLUDE_SIMPLE，此检查已完全注释
#if !defined(LV_CONF_INCLUDE_SIMPLE) && defined(LV_VERSION_CHECK) && (LV_VERSION_CHECK(8, 0, 0) == 0)
#error "LVGL版本不兼容，请使用LVGL 8.0或更高版本"
#endif
*/

/* 颜色深度：16位色 */
#define LV_COLOR_DEPTH 16

/* 屏幕刷新周期(毫秒) */
#define LV_DISP_DEF_REFR_PERIOD 30

/* 最大刷新行数 */
#define LV_DISP_DEF_HOR_RES 320
#define LV_DISP_DEF_VER_RES 480

/* 内存分配 */
#define LV_MEM_SIZE (32 * 1024U) /* 32KB缓冲区 */
#define LV_MEM_ATTR
#define LV_MEM_ADR 0 /* 使用默认内存分配 */

/* 最大对象数 */
#define LV_OBJ_FREE_NUM_TYPE uint16_t

/* 字体设置 */
#define LV_FONT_DEFAULT &lv_font_montserrat_16
#define LV_FONT_MONTSERRAT_8 1
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_30 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_34 1
#define LV_FONT_MONTSERRAT_36 1

/* 使用系统字体渲染 */
#define LV_USE_FONT_SYSTEM 0

/* 反锯齿 */
#define LV_USE_FONT_ANTIALIAS 1

/* 图像解码 */
#define LV_USE_BMP 1
#define LV_USE_JPG 0
#define LV_USE_PNG 1
#define LV_USE_GIF 1

/* 文件系统 */
#define LV_USE_FS_STDIO 0
#define LV_USE_FS_POSIX 0
#define LV_USE_FS_FATFS 0

/* 如果需要使用FATFS，取消下面的注释并确保已正确安装FATFS库
#define LV_USE_FS_FATFS 1
#define LV_FS_FATFS_LETTER 'A'  // 必须是大写ASCII字母
*/

/* 主题 */
#define LV_USE_THEME_DEFAULT 1
#define LV_USE_THEME_MONO 0
#define LV_USE_THEME_MATERIAL 0
#define LV_USE_THEME_ZEN 0
#define LV_USE_THEME_NIGHT 0

/* 部件 */
#define LV_USE_BTN 1
#define LV_USE_LABEL 1
#define LV_USE_LIST 1
#define LV_USE_CONT 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_PROGRESSBAR 1
#define LV_USE_IMG 1
#define LV_USE_LINE 1
#define LV_USE_CHART 1
#define LV_USE_TEXTAREA 1
#define LV_USE_TABVIEW 1
#define LV_USE_DROPDOWN 1
#define LV_USE_ROLLER 1
#define LV_USE_CALENDAR 1
#define LV_USE_KEYBOARD 1
#define LV_USE_SPINBOX 1
#define LV_USE_SPINNER 1
#define LV_USE_ARC 1
#define LV_USE_GAUGE 1
#define LV_USE_METER 1

/* 功能 */
#define LV_USE_ANIMATION 1
#define LV_USE_GROUP 1
#define LV_USE_GESTURE 1
#define LV_USE_FILESYSTEM 1
#define LV_USE_USER_DATA 1
#define LV_USE_REFR_DEBUG 0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

// 注释掉默认的LV_LOG_LEVEL定义，使用LVGL库的默认值
// #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

/* 断言 */
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_STYLE 0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0

/* 编译器设置 */
#define LV_ATTRIBUTE_TASK_HIGH_MEM_UNUSED
#define LV_ATTRIBUTE_IRAM_ATTR __attribute__((section(".iram0.text")))
#define LV_ATTRIBUTE_FLASH_RODATA __attribute__((section(".flash.rodata")))
#define LV_ATTRIBUTE_NOINLINE __attribute__((noinline))
#define LV_ATTRIBUTE_ALIGN(n) __attribute__((aligned(n)))
#define LV_ATTRIBUTE_PACKED __attribute__((packed))

/* 优化 */
#define LV_OPTIMIZE_LARGE_MEM 0

/* 触摸屏设置 */
#define LV_INDEV_DEF_READ_PERIOD 50
#define LV_INDEV_DEF_DRAG_LIMIT 10
#define LV_INDEV_DEF_DRAG_THROW 20
#define LV_INDEV_DEF_LONG_PRESS_TIME 400
#define LV_INDEV_DEF_LONG_PRESS_REP_TIME 100

/* 自定义颜色 - 已移除，使用LVGL库默认定义 */

#endif /* LV_CONF_H */