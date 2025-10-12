#include <WiFi.h>
#include "net_http.h"
#include "config.h"

// 添加缺少的函数声明
bool initFS();
void forceRefreshData();

// 初始化WiFi连接
void initWiFi() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 30) {
    delay(500);
    Serial.print(".");
    attempt++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connection failed");
  }
}

void setup() {
  // 初始化文件系统
  if (!initFS()) {
    Serial.println("文件系统初始化失败");
    return;
  }
  
  // 初始化WiFi
  initWiFi();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("无法连接到WiFi，无法获取数据");
    return;
  }
  
  // 强制刷新所有数据
  forceRefreshData();
  
  // 等待一段时间让文件系统准备好
  delay(1000);
  
  // 获取并保存数据
  Serial.println("\n开始获取天气数据...");
  getCityWeater();
  
  Serial.println("\n开始获取新闻数据...");
  getNews();
  
  Serial.println("\n开始获取金山词霸每日信息...");
  getIcibaDailyInfo();
  
  Serial.println("\n开始获取宇航员信息...");
  getAstronautsInfo();
  
  Serial.println("\n数据获取完成！数据已保存到SPIFFS文件系统中。");
  Serial.println("文件列表：");
  Serial.println("- /weather_data.json");
  Serial.println("- /news_data.json");
  Serial.println("- /iciba_info.json");
  Serial.println("- /astronauts_data.json");
  
  // 如果需要，可以选择性地获取APRS数据
  // Serial.println("\n开始获取APRS数据...");
  // showAPRSData();
  
  // 结束程序
  Serial.println("\n初始数据生成程序已完成任务");
}

void loop() {
  // 什么都不做，程序只运行一次
  delay(1000);
}