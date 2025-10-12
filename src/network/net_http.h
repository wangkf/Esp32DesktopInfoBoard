#ifndef NET_HTTP_H
#define NET_HTTP_H

#include <Arduino.h>
#include <lvgl.h>
#include <ArduinoJson.h>
#include "config/config.h"

// 外部变量声明
extern const uint32_t screenWidth;
extern const uint32_t screenHeight;
extern lv_obj_t* mao_select_label;
extern lv_obj_t* toxic_soul_label;
extern lv_obj_t* iciba_label;
extern lv_obj_t* weekday_label;
extern lv_obj_t* astronauts_label;


// 函数声明
void connectToWiFi();
void setupWiFi();
void updateTimeDisplay();
void getIcibaDailyInfo();
void getAstronautsInfo();
void getAstronautsData();
bool writeJsonToFile(const String& filename, const JsonDocument& doc);



// 从文件加载并显示数据的函数声明
  void net_displayIcibaDataFromFile();

#endif // NET_HTTP_H