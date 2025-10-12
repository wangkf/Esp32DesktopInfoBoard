/*
 * 显示屏驱动适配文件
 * 适用于ESP32桌面信息牌项目
 * 屏幕驱动芯片：ILI9488
 * 分辨率：320*480
 */

// 屏幕尺寸 - ILI9488分辨率为320*480
// 注意：避免与TFT_eSPI库中的宏定义冲突
#define ILI9488_WIDTH 320
#define ILI9488_HEIGHT 480

// 确保我们只定义TFT_eSPI库中未定义的引脚
// 以下引脚定义确保与实际硬件连接一致
#ifndef TFT_BL
#define TFT_BL   22 // 背光控制引脚
#endif

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "lvgl.h"

// 函数前置声明
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);
void my_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
void updateLVGLDisplayDriver(lv_disp_drv_t *disp_drv);
void createTouchInputDevice();

// 显示屏对象
TFT_eSPI tft = TFT_eSPI();

// 显示缓冲区配置
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[ILI9488_WIDTH * 10];  // 10行缓冲区
static lv_color_t buf2[ILI9488_WIDTH * 10];  // 第二个缓冲区

// 直接硬件测试函数 - 简化版，专注于基本显示功能
void hardwareDisplayTest() {
  Serial.println("===== 显示屏硬件测试开始 =====");
  
  // 设置背光引脚为输出模式并开启
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  Serial.println("背光已开启 (GPIO " + String(TFT_BL) + ")");
  
  // 初始化TFT显示屏
  Serial.print("初始化TFT显示屏...");
  tft.init();
  Serial.println("完成");
  
  // 重置显示屏
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, LOW);
  delay(100);
  digitalWrite(TFT_RST, HIGH);
  delay(100);
  Serial.println("显示屏已重置");
  
  // 设置屏幕旋转方向为1（横屏）
  tft.setRotation(1);
  Serial.println("屏幕旋转方向设置为: 1");
  
  // 填充黑色背景
  tft.fillScreen(TFT_BLACK);
  delay(500);
  
  // 显示彩色方块测试
  Serial.println("测试彩色显示...");
  tft.fillRect(0, 0, ILI9488_WIDTH/4, ILI9488_HEIGHT, TFT_RED);
  tft.fillRect(ILI9488_WIDTH/4, 0, ILI9488_WIDTH/4, ILI9488_HEIGHT, TFT_GREEN);
  tft.fillRect(ILI9488_WIDTH/2, 0, ILI9488_WIDTH/4, ILI9488_HEIGHT, TFT_BLUE);
  tft.fillRect(ILI9488_WIDTH*3/4, 0, ILI9488_WIDTH/4, ILI9488_HEIGHT, TFT_YELLOW);
  
  // 显示测试文字
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("ESP32 + ILI9488");
  tft.setCursor(10, 40);
  tft.println("显示测试成功!");
  tft.setCursor(10, 70);
  tft.print("分辨率: " + String(ILI9488_WIDTH) + "x" + String(ILI9488_HEIGHT));
  
  Serial.println("彩色方块和文字已显示");
  delay(2000); // 显示2秒
  
  // 闪烁背光3次，表示测试完成
  for (int i = 0; i < 3; i++) {
    digitalWrite(TFT_BL, LOW);
    delay(200);
    digitalWrite(TFT_BL, HIGH);
    delay(200);
  }
  
  Serial.println("===== 显示屏硬件测试完成 =====");
  
  // 清空屏幕为黑色，准备LVGL渲染
  tft.fillScreen(TFT_BLACK);
}

// 初始化显示屏
void initDisplay() {
  Serial.println("===== 初始化显示屏系统 =====");
  
  // 尝试使用低SPI速度提高稳定性（如果库支持）
  #if defined(USE_HSPI_PORT) || defined(USE_VSPI_PORT)
    tft.setSPISpeed(20000000); // 20MHz，比默认的40MHz更稳定
    Serial.println("SPI速度设置为20MHz");
  #else
    Serial.println("使用默认SPI速度");
  #endif
  
  // 执行直接硬件测试
  hardwareDisplayTest();
  
  // 初始化LVGL显示缓冲区 - 使用双缓冲提高性能
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, ILI9488_WIDTH * 10);
  Serial.println("LVGL显示缓冲区已初始化");
}

// 显示屏刷新回调函数 - 针对ILI9488优化
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  
  // 设置显示区域
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  
  // 推送颜色数据
  tft.pushColors((uint16_t*)color_p, w * h, true);
  
  tft.endWrite();
  
  // 通知LVGL刷新已完成
  lv_disp_flush_ready(disp_drv);
}

// 触摸屏读取回调函数（如果有触摸功能）
void my_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  // 读取触摸坐标
  static uint16_t touchX = 0, touchY = 0;
  
  // 注意：TFT_eSPI库的触摸API可能因版本而异
  // 以下是一个简化实现，假设没有实际触摸功能或使用基础方法
  bool touched = false;
  
  // 如果您有实际的触摸屏硬件，请根据硬件和库版本实现正确的触摸读取
  // 例如：对于XPT2046触摸屏控制器，可以使用以下代码：
  // touched = tft.getTouchRaw(&touchX, &touchY);
  // 或者对于某些版本可能需要使用自定义的触摸读取函数
  
  if (touched) {
    data->state = LV_INDEV_STATE_PR;    
    data->point.x = touchX;
    data->point.y = touchY;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

// 更新LVGL显示驱动配置
void updateLVGLDisplayDriver(lv_disp_drv_t *disp_drv) {
  // 设置绘制缓冲区和刷新回调
  disp_drv->flush_cb = my_disp_flush;
  disp_drv->draw_buf = &draw_buf;
  
  // 优化LVGL渲染性能
  disp_drv->full_refresh = 1; // 强制全屏刷新，有助于解决显示问题
  Serial.println("LVGL显示驱动已配置");
}

// 创建触摸输入设备（如果有触摸功能）
void createTouchInputDevice() {
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touch_read;
  lv_indev_drv_register(&indev_drv);
}