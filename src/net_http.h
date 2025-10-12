#ifndef NET_HTTP_H
#define NET_HTTP_H

#include <Arduino.h>
#include <lvgl.h>
#include <ArduinoJson.h>
#include "config.h"

// 外部变量声明
extern const uint32_t screenWidth;
extern const uint32_t screenHeight;
extern lv_obj_t* news_label;
extern lv_obj_t* weather_label;
extern lv_obj_t* weather_table;
extern lv_obj_t* mao_select_label;
extern lv_obj_t* toxic_soul_label;
extern lv_obj_t* iciba_label;
extern lv_obj_t* weekday_label;
extern lv_obj_t* astronauts_label;
extern String cityCode;
extern bool TianXingAPIKEY_Flag;
extern unsigned long lastWeatherUpdateTime;
extern unsigned long lastNewsUpdateTime;
extern unsigned long lastIcibaUpdateTime;
extern unsigned long lastAstronautsUpdateTime;

// 函数声明
void connectToWiFi();
void setupWiFi();
void updateTimeDisplay();
void getIcibaDailyInfo();
void getCityWeater();
void getNews();
void getAstronautsInfo();

// 获取APRS数据
extern lv_obj_t* aprs_label;
extern lv_obj_t* aprs_table;
void showAPRSData();

#endif // NET_HTTP_H