#include "network/net_http.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config/config.h"
#include "config/config_manager.h"
#include "manager/time_manager.h"
// 声明全局字体
extern const lv_font_t lvgl_font_digital_48;
#include "manager/data_manager.h"
#include <time.h>
#include <map>
#include <vector>
#include <SPIFFS.h>
// 定义强制刷新标志
bool forceRefreshAstronauts = false;
bool forceRefreshICIBA = false;
// 外部变量声明
extern const char* ntpServer;
extern unsigned long lastUpdateTime;
// 缓存数据变量
unsigned long lastAstronautsUpdateTime = 0;
// 函数前向声明
void getICIBADailyInfo();
void getAstronautsData();
// 文件系统初始化
bool initFS() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS挂载失败");
        return false;
    }
    Serial.println("SPIFFS挂载成功");
    return true;
}
// 从文件读取JSON数据
bool readJsonFromFile(const String& filename, JsonDocument& doc) {
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.println("无法打开文件进行读取: " + filename);
        return false;
    }
    
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.println("JSON反序列化失败: " + String(error.c_str()));
        file.close();
        return false;
    }
    
    file.close();
    return true;
}
// 写入JSON数据到文件，只有在内容变化时才重写文件
bool writeJsonToFile(const String& filename, const JsonDocument& doc) {
    // 检查文件是否已经存在
    if (SPIFFS.exists(filename)) {
        // 读取现有文件内容进行比较
        DynamicJsonDocument existingDoc(4096);
        if (readJsonFromFile(filename, existingDoc)) {
            // 创建临时文档，用于比较内容（不包括last_updated字段）
            DynamicJsonDocument tempDoc1(4096);
            DynamicJsonDocument tempDoc2(4096);
            
            // 对于doc，复制除了last_updated、code和msg之外的所有字段
            if (doc.is<JsonObject>()) {
                // 在ArduinoJson v7中，直接使用as获取JsonObjectConst
                const JsonObjectConst obj = doc.as<JsonObjectConst>();
                for (JsonPairConst kv : obj) {
                    const char* key = kv.key().c_str();
                    if (strcmp(key, "last_updated") != 0 && 
                        strcmp(key, "code") != 0 && 
                        strcmp(key, "msg") != 0) {
                        tempDoc1[key] = kv.value();
                    }
                }
            }
            
            // 对于existingDoc，复制除了last_updated、code和msg之外的所有字段
            if (existingDoc.is<JsonObject>()) {
                // 在ArduinoJson v7中，直接使用as获取JsonObjectConst
                const JsonObjectConst obj = existingDoc.as<JsonObjectConst>();
                for (JsonPairConst kv : obj) {
                    const char* key = kv.key().c_str();
                    if (strcmp(key, "last_updated") != 0 && 
                        strcmp(key, "code") != 0 && 
                        strcmp(key, "msg") != 0) {
                        tempDoc2[key] = kv.value();
                    }
                }
            }
            
            // 将两个临时文档序列化为字符串进行比较
            String str1, str2;
            serializeJson(tempDoc1, str1);
            serializeJson(tempDoc2, str2);
            
            if (str1.equals(str2)) {
                Serial.println("JSON内容没有变化，不重写文件: " + filename);
                return true;
            }
        }
    }
    
    // 内容有变化或文件不存在，写入新内容
    File file = SPIFFS.open(filename, "w");
    if (!file) {
        Serial.println("无法打开文件进行写入: " + filename);
        return false;
    }
    
    if (serializeJson(doc, file) == 0) {
        Serial.println("JSON序列化失败");
        file.close();
        return false;
    }
    
    file.close();
    Serial.println("成功写入文件: " + filename);
    return true;
}
// 连接WiFi网络
void setupWiFi() {
  // 使用ConfigManager获取WiFi配置
  ConfigManager* configManager = ConfigManager::getInstance();
  String ssid = "";
  String password = "";
  
  if (configManager->isConfigLoaded()) {
    configManager->getWiFiConfig(ssid, password);
  }
  
  Serial.print("连接WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid.c_str(), password.c_str());
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi连接成功");
  Serial.print("IP地址: ");
  Serial.println(WiFi.localIP());
  
  // 初始化文件系统
  initFS();
}
// 更新分钟显示（每分钟更新一次）
void updateMinuteDisplay() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }
  // 系统启动后只更新NTP数据，其他数据从本地JSON文件读取，在设定的时间在后台获取并更新JSON数据
  if (WiFi.status() == WL_CONNECTED) {
    // 检查是否为整点且分钟为0，小时能被2整除（每2小时更新一次）
    if (timeinfo.tm_min == 0 && timeinfo.tm_hour % 2 == 0 && timeinfo.tm_sec < 10) {
      // 是整点，且小时能被2整除（即2小时整点）
      // 检查是否已经到了更新时间（避免重复更新）
      static int lastUpdatedHour = -1;
      if (lastUpdatedHour != timeinfo.tm_hour) {
        Serial.println("[后台] 开始更新数据");
        // 后台获取并更新数据
          getICIBADailyInfo();
          getAstronautsData(); // 获取宇航员数据
        
        lastUpdatedHour = timeinfo.tm_hour;
        Serial.println("[后台] 数据更新完成");
      }
    }
  }
}
// 更新时间显示（整合函数，供外部调用）
void updateTimeDisplay() {
  // 调用TimeManager更新时间显示
  TimeManager::getInstance()->updateTimeDisplay();  
  // 调用原有的更新逻辑，但不再负责标签显示
  updateMinuteDisplay();
}
// 获取金山词霸每日信息
void getICIBADailyInfo() {
  // 委托给DataManager处理
  String result;
  if (DataManager::getInstance()->fetchData(ICIBA_DATA, result)) {
    Serial.println("获取金山词霸每日信息成功");
  } else {
    Serial.println("获取金山词霸每日信息失败");
  }
  forceRefreshICIBA = false;
}
// 宇航员数据更新函数 - 与updateMinuteDisplay中调用的函数名保持一致
void getAstronautsData() {
  // 委托给DataManager处理
  String result;
  if (DataManager::getInstance()->fetchData(ASTRONAUTS_DATA, result)) {
    Serial.println("获取宇航员信息成功");
    lastAstronautsUpdateTime = millis();
  } else {
    Serial.println("获取宇航员信息失败");
  }
  forceRefreshAstronauts = false;
}
