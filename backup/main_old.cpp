/*
屏幕配置文件路径lib\TFT_eSPI-2.5.0\User_Setups\Setup18_ST7789.h
*/

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

/*设置屏幕分辨率*/
static const uint32_t screenWidth = 240;  
static const uint32_t screenHeight = 240; 

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];
static lv_color_t buf2[screenHeight * 10];

/*   导入Gif素材   */
LV_IMG_DECLARE(fengche);
LV_IMG_DECLARE(hao_re_a);
LV_IMG_DECLARE(bad_apple_cut_raw);
LV_IMG_DECLARE(dai_ma_yu);
LV_IMG_DECLARE(ai);

lv_obj_t* img;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

void setup() {
  

  tft.begin();
  /*   TFT屏幕的旋转方向，参数：0、1、2、3*/ 
  tft.setRotation(0);

  /*   初始化LVGL   */
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, buf2, screenWidth * 10);

  /*   注册显示设备   */
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  lv_disp_drv_register(&disp_drv);

  /*   需注意：分辨率大于100*100的gif图片，ESP32支持PSRAM，否则
  将白屏，Log日志打印lv_gif_set_src: Could't load the source 	(in lv_gif.c line #78)的提示*/

   /*   创建Gif组件，并在屏幕正中央显示   */
  img = lv_gif_create(lv_scr_act());
  lv_gif_set_src(img, &fengche);
  lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

  pinMode(4,INPUT_PULLDOWN);
  Serial.begin(115200);
}

int keyTimes = 0;

void loop() {

  /*   运行LVGL   */
  lv_timer_handler();
  vTaskDelay(1);

  /*   通过按键切换动画   */
  if(digitalRead(4) == 1) {
    while(digitalRead(4) == 1);
    keyTimes++;
    Serial.println(keyTimes);
    switch(keyTimes) {
      case 1:lv_gif_set_src(img, &fengche);break;
      case 2:lv_gif_set_src(img, &hao_re_a);break;
      case 3:lv_gif_set_src(img, &ai);keyTimes=0;break;
      // case 4:lv_gif_set_src(img, &bad_apple_cut_raw);keyTimes=0;break;
      default:break;
    }
  }

}

