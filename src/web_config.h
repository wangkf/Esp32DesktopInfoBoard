#ifndef WEB_CONFIG_H
#define WEB_CONFIG_H

#include <Preferences.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "lvgl.h"

// 外部变量声明
extern Preferences preferences;
extern String PrefSSID;
extern String PrefPassword;
extern String cityCode;
extern String APIKEY;
extern int GIFNUM;
extern bool setWiFi_Flag;
extern bool TianXingAPIKEY_Flag;
extern lv_obj_t *label_message_data;

extern const char *versionNumber;
extern const unsigned char connect_wifi_map[];

extern void setWiFi();
extern void connectNewWifi();
extern void getNews();
extern void handleRoot();
extern void handleRootPost();
extern void initBasic();
extern void initSoftAP();
extern void initWebServer();
extern void initDNS();

extern unsigned long getChipId();

extern DNSServer dnsServer;
extern WebServer server;

// 引脚定义
extern const int button;
extern const int localPort;
extern const int updateTime;

extern int Filter_Value;
extern int setModeValue;

#endif // WEB_CONFIG_H