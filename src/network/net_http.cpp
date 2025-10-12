#include "network/net_http.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config/config.h"
#include "config/config_manager.h"
#include "manager/time_manager.h"

// 声明全局字体
extern lv_font_t lvgl_font_digital_48;
#include "manager/data_manager.h"
#include <time.h>
#include <map>
#include <vector>
#include <SPIFFS.h>

// 定义强制刷新标志
bool forceRefreshNews = false;
bool forceRefreshAstronauts = false;
bool forceRefreshICIBA = false;

// 外部变量声明
extern const char* ntpServer;

extern unsigned long lastUpdateTime;
extern const long updateInterval;

// DataManager单例通过getInstance()方法获取，不需要外部变量

// 缓存数据变量
String cachedNewsData = "";
String cachedIcibaData = "";
String cachedAstronautsData = "";
String cachedISSData = "";
unsigned long lastNewsUpdateTime = 0;
unsigned long lastIcibaUpdateTime = 0;
unsigned long lastAstronautsUpdateTime = 0;
unsigned long lastISSUpdateTime = 0;

// 函数前向声明
void getICIBADailyInfo();
void getNews();
void getAstronautsData();

// JSON解析缓冲区
DynamicJsonDocument docbuffer(4096);

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
  
  // 系统启动后只更新NTP数据，其他数据从本地JSON文件读取
  // 在设定的时间在后台获取并更新JSON数据
  if (WiFi.status() == WL_CONNECTED) {
    // 检查是否为整点且分钟为0，小时能被2整除（每2小时更新一次）
    if (timeinfo.tm_min == 0 && timeinfo.tm_hour % 2 == 0 && timeinfo.tm_sec < 10) {
      // 是整点，且小时能被2整除（即2小时整点）
      // 检查是否已经到了更新时间（避免重复更新）
      static int lastUpdatedHour = -1;
      if (lastUpdatedHour != timeinfo.tm_hour) {
        Serial.println("[后台] 开始更新数据");
        // 后台获取并更新数据
        getNews();
        getICIBADailyInfo();
        getAstronautsData(); // 获取宇航员数据
        
        lastUpdatedHour = timeinfo.tm_hour;
        Serial.println("[后台] 数据更新完成");
      }
    }
  }
}

// 更新秒钟显示（每秒更新一次）
void updateSecondDisplay() {
  // 此函数已被重构为使用TimeManager类处理
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

// 获取新闻
void getNews() {
  // 完全委托给DataManager处理，与其他数据类型保持一致
  String result;
  if (DataManager::getInstance()->fetchData(NEWS_DATA, result)) {
    Serial.println("获取今日简报成功");
    lastNewsUpdateTime = millis();
  } else {
    Serial.println("获取今日简报失败");
  }
  forceRefreshNews = false;
}

// 标记为强制刷新数据（用于整点更新时调用）
void forceRefreshData() {
  forceRefreshNews = true;
  forceRefreshICIBA = true;
  // 强制刷新宇航员数据
  lastAstronautsUpdateTime = 0;
}

// 宇航员数据更新函数 - 与updateMinuteDisplay中调用的函数名保持一致
void getAstronautsData() {
  getAstronautsInfo();
}

// 占位符函数实现
void getCityCode() {
  // 函数实现待补充
  Serial.println("getCityCode函数尚未实现");
}

void getlunar() {
  // 函数实现待补充
  Serial.println("getlunar函数尚未实现");
}

// 获取国际空间站宇航员信息
void getAstronautsInfo() {
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

// 定义APRS_MAX_PACKETS常量，保留以兼容旧代码

/**
 * 从文件加载并显示新闻数据
 */
void net_displayNewsDataFromFile() {
  Serial.println("尝试从文件加载新闻数据");
  
  // 确保news_label已创建
  if (news_label == NULL || !lv_obj_is_valid(news_label)) {
    if (news_label != NULL) {
      // 确保旧标签被正确清理
      lv_obj_del(news_label);
    }
    news_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(news_label, &lvgl_font_song_16, 0);
    lv_obj_set_style_text_color(news_label, lv_color_hex(0x000000), 0); // 默认黑色
    lv_obj_set_style_text_align(news_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(news_label, LV_LABEL_LONG_WRAP); // 设置自动换行
    lv_obj_set_width(news_label, screenWidth - 40); // 设置标签宽度
    lv_obj_set_height(news_label, screenHeight - 120); // 固定高度
    lv_obj_align(news_label, LV_ALIGN_TOP_MID, 0, 100); // 顶部居中对齐，顶部离屏幕顶部100px
    
    // 不添加背景和边框效果
    lv_obj_set_style_bg_opa(news_label, 0, 0); // 完全透明背景
    lv_obj_set_style_border_width(news_label, 0, 0); // 无边框
    lv_obj_set_style_radius(news_label, 0, 0); // 无圆角
    lv_obj_set_style_pad_all(news_label, 10, 0); // 内边距
    
    Serial.println("创建或重新创建了news_label");
  }
  
  // 检查文件是否存在
  if (SPIFFS.exists("/news.json")) {
    File file = SPIFFS.open("/news.json", "r");
    if (file) {
      // 读取文件内容
      String jsonString = file.readString();
      file.close();
      
      // 解析JSON数据
      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, jsonString);
      
      if (!error) {
        Serial.println("成功解析新闻数据文件");
        
        // 构建新闻内容字符串
        String newsContent = "";
        
        // 检查是否包含result对象
        if (doc.containsKey("result") && doc["result"].is<JsonObject>()) {
          JsonObject result = doc["result"].as<JsonObject>();
          
          // 检查是否有新闻数据
          if (result.containsKey("list") && result["list"].is<JsonArray>()) {
            JsonArray newsList = result["list"].as<JsonArray>();
            
            // 显示最多10条新闻
            int displayCount = min((int)newsList.size(), 10);
            for (int i = 0; i < displayCount; i++) {
              JsonObject newsItem = newsList[i];
              
              // 检查是否有标题
              if (newsItem["title"].is<const char*>()) {
                newsContent += String(i + 1) + ". " + String(newsItem["title"].as<const char*>()) + "\n\n";
              }
            }
            
            // 如果没有新闻数据，显示提示信息
            if (newsList.size() == 0) {
              newsContent = "暂无新闻数据";
            }
          } else {
            newsContent = "暂无新闻数据";
          }
          
          // 添加最后更新时间
          if (result.containsKey("last_updated") && result["last_updated"].is<const char*>()) {
            newsContent += "最后更新: " + String(result["last_updated"].as<const char*>());
          }
        } else {
          newsContent = "新闻数据格式不正确";
        }
        
        // 更新新闻标签
        if (news_label && lv_obj_is_valid(news_label)) {
          lv_label_set_text(news_label, newsContent.c_str());
        }
        
        Serial.println("成功显示文件中的新闻数据");
      } else {
        Serial.print("解析新闻数据文件失败: ");
        Serial.println(error.c_str());
        
        if (news_label && lv_obj_is_valid(news_label)) {
          lv_label_set_text(news_label, "本地新闻数据解析失败");
        }
      }
    } else {
      Serial.println("无法打开新闻数据文件");
      
      if (news_label && lv_obj_is_valid(news_label)) {
        lv_label_set_text(news_label, "无法打开本地新闻数据文件");
      }
    }
  } else {
    Serial.println("新闻数据文件不存在");
    
    if (news_label && lv_obj_is_valid(news_label)) {
      lv_label_set_text(news_label, "本地新闻数据文件不存在");
    }
  }
}

/**
 * 从文件加载并显示金山词霸每日信息
 */
void net_displayIcibaDataFromFile() {
  Serial.println("尝试从文件加载金山词霸每日信息");
  
  // 确保iciba_label已创建
  if (iciba_label == NULL || !lv_obj_is_valid(iciba_label)) {
    if (iciba_label != NULL) {
      // 确保旧标签被正确清理
      lv_obj_del(iciba_label);
    }
    iciba_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(iciba_label, &lvgl_font_song_16, 0);
    lv_obj_set_style_text_color(iciba_label, lv_color_hex(0x000000), 0); // 默认黑色
    lv_obj_set_style_text_align(iciba_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(iciba_label, LV_LABEL_LONG_WRAP); // 设置自动换行
    lv_obj_set_width(iciba_label, screenWidth - 40); // 设置标签宽度
    lv_obj_set_height(iciba_label, screenHeight - 120); // 固定高度
    lv_obj_align(iciba_label, LV_ALIGN_TOP_MID, 0, 100); // 顶部居中对齐，顶部离屏幕顶部100px
    
    // 不添加背景和边框效果
    lv_obj_set_style_bg_opa(iciba_label, 0, 0); // 完全透明背景
    lv_obj_set_style_border_width(iciba_label, 0, 0); // 无边框
    lv_obj_set_style_radius(iciba_label, 0, 0); // 无圆角
    lv_obj_set_style_pad_all(iciba_label, 10, 0); // 内边距
    
    Serial.println("创建或重新创建了iciba_label");
  }
  
  // 检查文件是否存在
  if (SPIFFS.exists("/iciba.json")) {
    File file = SPIFFS.open("/iciba.json", "r");
    if (file) {
      // 读取文件内容
      String jsonString = file.readString();
      file.close();
      
      // 解析JSON数据
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, jsonString);
      
      if (!error) {
        Serial.println("成功解析金山词霸数据文件");
        
        // 构建显示内容
        String displayText = "\n";
        
        // 获取句子信息（保持访问顶级字段）
        if (doc.containsKey("content") && doc["content"].is<const char*>()) {
          displayText += String(doc["content"].as<const char*>()) + "\n\n";
        }
        
        // 获取翻译信息（保持访问顶级字段）
        if (doc.containsKey("note") && doc["note"].is<const char*>()) {
          displayText += String(doc["note"].as<const char*>());
        }
        
        // 如果有tts字段，可以显示提示
        if (doc.containsKey("tts")) {
          displayText += "\n\n[有发音]";
        }
        
        // 从result对象中获取最后更新时间
        if (doc.containsKey("result") && doc["result"].is<JsonObject>()) {
          JsonObject result = doc["result"].as<JsonObject>();
          if (result.containsKey("last_updated") && result["last_updated"].is<const char*>()) {
            displayText += "\n\n" + String(result["last_updated"].as<const char*>());
          }
        }
        
        // 更新金山词霸标签
        if (iciba_label && lv_obj_is_valid(iciba_label)) {
          lv_label_set_text(iciba_label, displayText.c_str());
        }
        
        Serial.println("成功显示文件中的金山词霸数据");
      } else {
        Serial.print("解析金山词霸数据文件失败: ");
        Serial.println(error.c_str());
        
        if (iciba_label && lv_obj_is_valid(iciba_label)) {
          lv_label_set_text(iciba_label, "本地金山词霸数据解析失败");
        }
      }
    } else {
      Serial.println("无法打开金山词霸数据文件");
      
      if (iciba_label && lv_obj_is_valid(iciba_label)) {
        lv_label_set_text(iciba_label, "无法打开本地金山词霸数据文件");
      }
    }
  } else {
    Serial.println("金山词霸数据文件不存在");
    
    if (iciba_label && lv_obj_is_valid(iciba_label)) {
      lv_label_set_text(iciba_label, "本地金山词霸数据文件不存在");
    }
  }
}