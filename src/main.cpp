/**
 * ESP32 桌面信息牌程序
 * 根据 https://www.xutoubee.cn/225.html 资料开发
 * 功能包括：GIF动画显示、按键切换动画
 */

#include <Arduino.h>
#include <lvgl.h>
#include "display_driver.h"

/*   导入图像素材   */
LV_IMG_DECLARE(fengche);
LV_IMG_DECLARE(hao_re_a);
LV_IMG_DECLARE(dai_ma_yu);
LV_IMG_DECLARE(ai);

lv_obj_t* img;

void setup() {
  Serial.begin(115200);
  
  /*   初始化LVGL   */
  lv_init();
  
  /*   初始化显示屏系统   */
  initDisplay();
  
  /*   注册显示设备   */
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  updateLVGLDisplayDriver(&disp_drv);
  lv_disp_drv_register(&disp_drv);

  /*   需注意：分辨率大于100*100的gif图片，ESP32支持PSRAM，否则
  将白屏，Log日志打印lv_gif_set_src: Could't load the source 	(in lv_gif.c line #78)的提示*/

  /*   创建图像组件，并在屏幕正中央显示   */
  img = lv_img_create(lv_scr_act());
  lv_img_set_src(img, &fengche);
  lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

  // 设置按键引脚
  pinMode(4, INPUT_PULLDOWN);
}

int keyTimes = 0;

void loop() {

  /*   运行LVGL   */
  lv_timer_handler();
  vTaskDelay(1);

  /*   通过按键切换图像   */
  if(digitalRead(4) == 1) {
    while(digitalRead(4) == 1);
    keyTimes++;
    Serial.println(keyTimes);
    switch(keyTimes) {
      case 1:lv_img_set_src(img, &fengche);break;
      case 2:lv_img_set_src(img, &hao_re_a);break;
      case 3:lv_img_set_src(img, &dai_ma_yu);break;
      case 4:lv_img_set_src(img, &ai);keyTimes=0;break;
      default:break;
    }
  }
}