#include "network/net_http.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config/config.h"
#include "manager/time_manager.h"

// 声明全局字体
extern lv_font_t lvgl_font_digital_48;
#include "ui/weather_display.h"
#include "manager/data_manager.h"
#include <time.h>
#include <map>
#include <vector>
#include <SPIFFS.h>

// 定义强制刷新标志
bool forceRefreshWeather = false;
bool forceRefreshNews = false;
bool forceRefreshAstronauts = false;
bool forceRefreshICIBA = false;

// 外部变量声明
extern const char* ntpServer;

extern unsigned long lastUpdateTime;
extern const long updateInterval;

extern epd_rtc_data epd_rtc;

// DataManager单例通过getInstance()方法获取，不需要外部变量

// 缓存数据变量
String cachedNewsData = "";
String cachedWeatherData = "";
String cachedIcibaData = "";
String cachedAstronautsData = "";
String cachedISSData = "";
unsigned long lastNewsUpdateTime = 0;
unsigned long lastWeatherUpdateTime = 0;
unsigned long lastIcibaUpdateTime = 0;
unsigned long lastAstronautsUpdateTime = 0;
unsigned long lastISSUpdateTime = 0;

// 天气代码映射数组
int wtr_code[33] = {
  100, 101, 102, 103, 104, 150, 151, 152, 153, 154, 
  301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 
  311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 
  321, 322, 323
};

// 函数前向声明
void getICIBADailyInfo();
void getCityWeather();
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

// 写入JSON数据到文件
bool writeJsonToFile(const String& filename, const JsonDocument& doc) {
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

// 连接WiFi网络
void setupWiFi() {
  Serial.print("连接WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
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
      getCityWeather();
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
  if (DataManager::getInstance()->fetchICIBAData(result)) {
    Serial.println("获取金山词霸每日信息成功");
  } else {
    Serial.println("获取金山词霸每日信息失败");
  }
  forceRefreshICIBA = false;
}

// 获取天气信息 - 已被屏蔽
void getCityWeather() {
  Serial.println("天气信息获取功能已被屏蔽");
  // 此函数已被屏蔽，不执行任何天气数据获取操作
  return;
}


// 获取新闻
void getNews() {
  // 检查是否需要从网络获取数据（首次启动或整点更新时）
  if (DataManager::getInstance()->getIsFirstStartup() || forceRefreshNews) {
    
    // 检查WiFi连接状态
    if (WiFi.status() != WL_CONNECTED) {
      // 如果有缓存数据，则使用缓存数据
      forceRefreshNews = false;
      return;
    }
    
    String newsStr;
    bool success = DataManager::getInstance()->fetchNewsData(newsStr);
    
    if (success) {
      Serial.println("获取今日简报成功");
      // 更新新闻数据的最后更新时间戳
      lastNewsUpdateTime = millis();
    } else {
      Serial.println("获取今日简报失败");
    }
    
    forceRefreshNews = false;
  }
}

// 标记为强制刷新数据（用于整点更新时调用）
void forceRefreshData() {
  forceRefreshNews = true;
  forceRefreshWeather = true;
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
  if (DataManager::getInstance()->fetchAstronautsData(result)) {
    Serial.println("获取宇航员信息成功");
    lastAstronautsUpdateTime = millis();
  } else {
    Serial.println("获取宇航员信息失败");
  }
  forceRefreshAstronauts = false;
}



// 定义APRS数据包结构体
struct APRSPacket {
  String callSign;
  String info;
  unsigned long timestamp;
};

// 定义APRS数据缓存
std::vector<APRSPacket> aprsPacketCache;

// 缓存新的APRS数据包，保持最多6条记录
void cacheAPRSPacket(String callSign, String info) {
  APRSPacket packet;
  packet.callSign = callSign;
  packet.info = info;
  packet.timestamp = millis();
  
  // 添加新数据包到缓存
  aprsPacketCache.push_back(packet);
  
  // 如果缓存超过6条记录，删除最旧的记录
  if (aprsPacketCache.size() > APRS_MAX_PACKETS) {
    aprsPacketCache.erase(aprsPacketCache.begin());
  }
}

// 从缓存显示APRS数据
void showCachedAPRSData() {
  // 表头设置
  lv_table_set_cell_value(aprs_table, 0, 0, "呼号");
  lv_table_set_cell_value(aprs_table, 0, 1, "信息");
  
  // 设置表头样式
  lv_obj_set_style_bg_color(aprs_table, lv_color_hex(0xCCCCCC), LV_PART_ITEMS | LV_STATE_DEFAULT);
  
  // 清空之前的数据
  for (int i = 1; i <= APRS_MAX_PACKETS; i++) {
    lv_table_set_cell_value(aprs_table, i, 0, "");
    lv_table_set_cell_value(aprs_table, i, 1, "");
  }
  
  // 显示缓存的数据包（倒序，最新的在上面）
  int packetCount = 0;
  for (auto it = aprsPacketCache.rbegin(); it != aprsPacketCache.rend() && packetCount < APRS_MAX_PACKETS; ++it, ++packetCount) {
    lv_table_set_cell_value(aprs_table, packetCount + 1, 0, it->callSign.c_str());
    lv_table_set_cell_value(aprs_table, packetCount + 1, 1, it->info.c_str());
  }
  
  if (packetCount > 0) {
    lv_label_set_text(aprs_label, String("已缓存" + String(packetCount) + "条APRS数据").c_str());
  } else {
    lv_label_set_text(aprs_label, "暂无缓存的APRS数据");
  }
}

// 获取APRS数据
extern lv_obj_t* aprs_label;
extern lv_obj_t* aprs_table;
void showAPRSData() {
    Serial.println("尝试从网络获取APRS数据");
  
  // 检查WiFi连接状态
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi未连接，无法获取APRS数据");
    return;
  }
  
  // 连接到APRS服务器
  WiFiClient aprsClient;
  Serial.print("尝试连接APRS服务器：");
  Serial.print(APRS_SERVER);
  Serial.print(":");
  Serial.println(APRS_PORT);
  
  if (!aprsClient.connect(APRS_SERVER, APRS_PORT)) {
    Serial.println("无法连接到APRS服务器");
    return;
  }
  
  Serial.println("已连接到APRS服务器");
  
  // 发送登录命令
  String loginCmd = "user " + String(APRS_CALLSIGN) + " pass " + String(APRS_PASSCODE) + " vers ESP32DesktopInfoBoard 1.0 filter r/34.23/108.95/" + String(APRS_RANGE_KM);
  aprsClient.println(loginCmd);
  
  Serial.print("发送登录命令：");
  Serial.println(loginCmd);
  
  // 等待服务器响应
  unsigned long startTime = millis();
  int newPacketCount = 0;
  
  // 接收数据的最大等待时间（毫秒）
  const unsigned long MAX_WAIT_TIME = 10000; // 10秒
  
  // 接收APRS数据
  while (aprsClient.connected() && (millis() - startTime < MAX_WAIT_TIME) && (newPacketCount < APRS_MAX_PACKETS)) {
    if (aprsClient.available()) {
      String line = aprsClient.readStringUntil('\n');
      line.trim();
      
      // 跳过服务器的登录响应和其他非数据包信息
      if (line.startsWith("#") || line.isEmpty()) continue;
      
      Serial.print("收到APRS数据包：");
      Serial.println(line);
      
      // 解析APRS数据包
      // 简单解析：呼号部分通常在数据包的开头，用>分隔
      int callSignEndPos = line.indexOf('>');
      if (callSignEndPos > 0) {
        String callSign = line.substring(0, callSignEndPos);
        String info = line.substring(callSignEndPos + 1);
        
        // 只保存信息部分的前50个字符
        if (info.length() > 50) {
          info = info.substring(0, 50) + "...";
        }
        
        // 缓存新的数据包
        cacheAPRSPacket(callSign, info);
        
        newPacketCount++;
      }
    }
  }
  
  // 断开连接
  aprsClient.stop();
  
  // 将APRS数据写入JSON文件，按照aprs.json的结构
  if (!aprsPacketCache.empty()) {
      JsonDocument aprsJson;
      
      // 创建result对象
      JsonObject resultObj = aprsJson["result"].to<JsonObject>();
      resultObj["total"] = aprsPacketCache.size();
      
      // 设置最后更新时间
      time_t now;
      time(&now);
      struct tm timeinfo;
      localtime_r(&now, &timeinfo);
      char updateTimeStr[32];
      sprintf(updateTimeStr, "%04d-%02d-%02d %02d:%02d:%02d", 
              timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
              timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      resultObj["last_update"] = updateTimeStr;
      
      JsonArray packetsArray = resultObj["packets"].to<JsonArray>();
      
      // 倒序添加，最新的在前面
      for (auto it = aprsPacketCache.rbegin(); it != aprsPacketCache.rend(); ++it) {
          JsonObject packetObj = packetsArray.add<JsonObject>();
          packetObj["callsign"] = it->callSign;
          packetObj["raw_data"] = it->info;
          
          // 格式化时间戳
          time_t packetTime = it->timestamp / 1000; // 转换为秒
          localtime_r(&packetTime, &timeinfo);
          sprintf(updateTimeStr, "%04d-%02d-%02d %02d:%02d:%02d", 
                  timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
          packetObj["timestamp"] = updateTimeStr;
      }
      
      writeJsonToFile("/aprs.json", aprsJson);
  }
  
  Serial.print("APRS数据接收完成，新增：");
  Serial.print(newPacketCount);
  Serial.println("条数据包");
  }

// 获取天气数据函数 - 基于weather.com.cn的API
int Http_Get_wthrcdn(void)
{
  int i;
  HTTPClient http;
  String  errjson = "err_code";
  
  // 设置请求头中的User-Agent和Referer
  http.begin("http://d1.weather.com.cn/weather_index/" + String(epd_rtc.citycode) + ".html"); 
  http.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  http.addHeader("Referer", "http://www.weather.com.cn/");
  
  int httpCode = http.GET();
  
  // 处理HTTP请求结果
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String str = http.getString();
      
      // 提取dataSK部分的数据
      int indexStart = str.indexOf("dataSK = ");
      int indexEnd = str.indexOf(";var dataZS");
      if (indexStart != -1 && indexEnd != -1) {
        String jsonDataSK = str.substring(indexStart + 8, indexEnd);
        docbuffer.clear();
        deserializeJson(docbuffer, jsonDataSK);   // 反序列化json 
        JsonObject sk = docbuffer.as<JsonObject>();
        
        // 提取城市名称
        String city = sk["cityname"].as<String>();
        sprintf(epd_rtc.city, "%s", city.c_str());
        
        // 提取实时温度
        epd_rtc.Weather_Tmp_Real = sk["temp"];
        
        // 提取湿度
        const char *weatherdatas0 = sk["SD"];
        sprintf(epd_rtc.humidity, "%s", weatherdatas0);
        
        // 提取更新时间
        const char *weatherdatas1 = sk["time"];
        sprintf(epd_rtc.time, "%s", weatherdatas1);
        
        // 提取限行信息
        const char *weatherdatas2 = sk["limitnumber"];
        sprintf(epd_rtc.limitnumber, "%s", weatherdatas2);
        
        // 提取天气代码并进行转换
        const char *real_weather_code = sk["weathercode"];
        int weather_code = atoi(&real_weather_code[1]);
        if (weather_code == 301) {weather_code = 8;}
        if (weather_code == 302) {weather_code = 15;}
        if (weather_code > 31) {weather_code = 32;}
        epd_rtc.Weather_TypeCode = wtr_code[weather_code];
        
        // 提取天气描述
        const char *weatherdatas3 = sk["weather"];
        sprintf(epd_rtc.Weather_Type, "%s", weatherdatas3);
        
        // 提取风力风向
        String Wind_data = sk["WD"].as<String>() + " " + sk["WS"].as<String>();
        sprintf(epd_rtc.Wind_Direction, "%s", Wind_data.c_str());
      }
      
      // 提取dataZS部分的数据
      indexStart = str.indexOf(";var dataZS ={\"zs\":");
      indexEnd = str.indexOf(",\"cn\":\"");
      if (indexStart != -1 && indexEnd != -1) {
        String jsonSuggest = str.substring(indexStart + 19, indexEnd);
        docbuffer.clear();
        deserializeJson(docbuffer, jsonSuggest);   // 反序列化json 
        JsonObject dataSuggestJson = docbuffer.as<JsonObject>();
        
        // 提取防晒信息
        String fs_data = dataSuggestJson["pp_name"].as<String>() + ":" + dataSuggestJson["pp_hint"].as<String>() + "," + dataSuggestJson["pp_des_s"].as<String>();
        sprintf(epd_rtc.fs, "%s", fs_data.c_str());
        
        // 提取带伞建议
        String ys_data = dataSuggestJson["ys_name"].as<String>() + ":" + dataSuggestJson["ys_hint"].as<String>() + "," + dataSuggestJson["ys_des_s"].as<String>();
        sprintf(epd_rtc.ys, "%s", ys_data.c_str());
      }
      
      // 提取5天天气预报数据
      indexStart = str.indexOf("};var fc = ");
      if (indexStart != -1) {
        String payload = str.substring(indexStart + 10);
        docbuffer.clear();
        deserializeJson(docbuffer, payload);   // 反序列化json 
        JsonObject root = docbuffer.as<JsonObject>();
        
        if (root != 0) {
          int error_code = root[errjson];
          if (error_code == 0)   // 无错误
          {
            // 提取当天最高和最低温度
            epd_rtc.Weather_Tmp_High = root["f"][0]["fc"];
            epd_rtc.Weather_Tmp_Low = root["f"][0]["fd"];
            
            // 提取5天天气数据
            for (i = 0; i < 5; i++) {
              // 提取白天天气代码
              const char *day = root["f"][i]["fa"];
              int dn = atoi(day);
              if (dn > 31) dn = 32; // 显示空
              epd_rtc.day_code[i][0] = wtr_code[dn];
              
              // 提取夜晚天气代码
              const char *night = root["f"][i]["fb"];
              int nn = atoi(night);
              if (nn > 31) nn = 32; // 显示空
              epd_rtc.night_code[i][0] = wtr_code[nn];
              
              // 转换晚上晴天和阴天为夜晚图标
              if (epd_rtc.night_code[i][0] == 100) { // 晚上晴天转换成夜晚图标
                epd_rtc.night_code[i][0] = 150;
              }
              if (epd_rtc.night_code[i][0] == 104) { // 晚上阴天转换成夜晚图标
                epd_rtc.night_code[i][0] = 154;
              }
              
              // 提取最高温度
              const char *tmp_max = root["f"][i]["fc"];
              epd_rtc.tmp_max_data[i][0] = atoi(tmp_max);
              
              // 提取最低温度
              const char *tmp_min = root["f"][i]["fd"];
              epd_rtc.tmp_min_data[i][0] = atoi(tmp_min);
              
              // 提取日期
              const char *date = root["f"][i]["fj"];
              sprintf(epd_rtc.Weather_data[i], "%s", date); 
            }
          }
        }
      }
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  
  // 结束HTTP连接
  http.end();
  
  return 1;
}

/**
 * 从文件加载并显示天气数据
 */
void net_displayWeatherDataFromFile() {
  Serial.println("尝试从文件加载天气数据");
  
  // 确保weather_label已创建
  if (weather_label == NULL || !lv_obj_is_valid(weather_label)) {
    if (weather_label != NULL) {
      // 确保旧标签被正确清理
      lv_obj_del(weather_label);
    }
    weather_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(weather_label, &lvgl_font_song_16, 0);
    lv_obj_set_style_text_color(weather_label, lv_color_hex(0x0000FF), 0); // 默认蓝色
    lv_obj_set_style_text_align(weather_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(weather_label, LV_LABEL_LONG_WRAP); // 设置自动换行
    lv_obj_set_width(weather_label, screenWidth - 40); // 设置标签宽度
    lv_obj_set_height(weather_label, screenHeight - 120); // 固定高度，与屏幕底部对齐
    lv_obj_align(weather_label, LV_ALIGN_TOP_MID, 0, 100); // 顶部居中对齐，顶部离屏幕顶部100px
    
    // 不添加背景和边框效果
    lv_obj_set_style_bg_opa(weather_label, 0, 0); // 完全透明背景
    lv_obj_set_style_border_width(weather_label, 0, 0); // 无边框
    lv_obj_set_style_radius(weather_label, 0, 0); // 无圆角
    lv_obj_set_style_pad_all(weather_label, 10, 0); // 内边距
    
    Serial.println("创建或重新创建了weather_label");
  }
  
  // 检查文件是否存在
  if (SPIFFS.exists("/weather.json")) {
    File file = SPIFFS.open("/weather.json", "r");
    if (file) {
      // 读取文件内容
      String jsonString = file.readString();
      file.close();
      
      // 解析JSON数据
      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, jsonString);
      
      if (!error) {
        Serial.println("成功解析天气数据文件");
        
        // 构建天气内容字符串
        String weatherContent = "";
        
        // 检查是否包含result对象
        if (doc.containsKey("result") && doc["result"].is<JsonObject>()) {
          JsonObject result = doc["result"].as<JsonObject>();
          
          // 获取城市信息
          if (result.containsKey("city") && result["city"].is<const char*>()) {
            weatherContent += "城市: " + String(result["city"].as<const char*>()) + "\n";
          }
          
          // 获取当前天气信息
          if (result.containsKey("realtime") && result["realtime"].is<JsonObject>()) {
            JsonObject realtime = result["realtime"].as<JsonObject>();
            // temperature字段是字符串，需要特殊处理
            if (realtime.containsKey("temperature") && realtime["temperature"].is<const char*>()) {
              weatherContent += "当前温度: " + String(realtime["temperature"].as<const char*>()) + "°C\n";
            }
            // humidity字段是带百分号的字符串
            if (realtime.containsKey("humidity") && realtime["humidity"].is<const char*>()) {
              weatherContent += "湿度: " + String(realtime["humidity"].as<const char*>()) + "\n";
            }
            // wind_speed在JSON中是power_detail
            if (realtime.containsKey("power_detail") && realtime["power_detail"].is<const char*>()) {
              weatherContent += "风速: " + String(realtime["power_detail"].as<const char*>()) + "\n";
            }
            // weather_type在JSON中是info
            if (realtime.containsKey("info") && realtime["info"].is<const char*>()) {
              weatherContent += "天气状况: " + String(realtime["info"].as<const char*>()) + "\n";
            }
            weatherContent += "\n";
          }
          
          // 获取每日天气预报
          if (result.containsKey("future") && result["future"].is<JsonArray>()) {
            JsonArray future = result["future"].as<JsonArray>();
            
            for (JsonVariant day : future) {
              if (day.containsKey("date") && day["date"].is<const char*>()) {
                weatherContent += "日期: " + String(day["date"].as<const char*>()) + "\n";
              }
              // 在JSON中只有一个temperature字段包含温度范围
              if (day.containsKey("temperature") && day["temperature"].is<const char*>()) {
                weatherContent += "温度范围: " + String(day["temperature"].as<const char*>()) + "°C\n";
              }
              // weather_type在JSON中是weather
              if (day.containsKey("weather") && day["weather"].is<const char*>()) {
                weatherContent += "天气状况: " + String(day["weather"].as<const char*>()) + "\n";
              }
              // 增加风向信息
              if (day.containsKey("direct") && day["direct"].is<const char*>()) {
                weatherContent += "风向: " + String(day["direct"].as<const char*>()) + "\n";
              }
              weatherContent += "\n";
            }
          }
          
          // 添加最后更新时间
          if (result.containsKey("last_updated") && result["last_updated"].is<const char*>()) {
            weatherContent += "最后更新: " + String(result["last_updated"].as<const char*>());
          }
        } else {
          weatherContent = "天气数据格式不正确";
        }
        
        // 更新天气标签
        if (weather_label && lv_obj_is_valid(weather_label)) {
          lv_label_set_text(weather_label, weatherContent.c_str());
        }
        
        Serial.println("成功显示文件中的天气数据");
      } else {
        Serial.print("解析天气数据文件失败: ");
        Serial.println(error.c_str());
        
        if (weather_label && lv_obj_is_valid(weather_label)) {
          lv_label_set_text(weather_label, "本地天气数据解析失败");
        }
      }
    } else {
      Serial.println("无法打开天气数据文件");
      
      if (weather_label && lv_obj_is_valid(weather_label)) {
        lv_label_set_text(weather_label, "无法打开本地天气数据文件");
      }
    }
  } else {
    Serial.println("天气数据文件不存在");
    
    if (weather_label && lv_obj_is_valid(weather_label)) {
      lv_label_set_text(weather_label, "本地天气数据文件不存在");
    }
  }
}

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

/**
 * 从文件加载并显示APRS数据
 */
void net_displayAPRSDataFromFile() {
  Serial.println("尝试从文件加载APRS数据");
  
  // 检查文件是否存在
  if (SPIFFS.exists("/aprs.json")) {
      File file = SPIFFS.open("/aprs.json", "r");
    if (file) {
      // 读取文件内容
      String jsonString = file.readString();
      file.close();
      
      // 解析JSON数据
      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, jsonString);
      
      if (!error) {
        Serial.println("成功解析APRS数据文件");
        
        // 确保APRS标签和表格已创建
        if (!aprs_label) {
          aprs_label = lv_label_create(lv_scr_act());
          lv_obj_set_style_text_font(aprs_label, &lvgl_font_song_16, 0);
          lv_obj_set_style_text_color(aprs_label, lv_color_hex(0x000000), 0);
          lv_obj_set_width(aprs_label, screenWidth - 20);
          lv_obj_align(aprs_label, LV_ALIGN_TOP_LEFT, 10, 70);
          lv_label_set_long_mode(aprs_label, LV_LABEL_LONG_WRAP);
        }
        
        if (!aprs_table) {
          aprs_table = lv_table_create(lv_scr_act());
          lv_table_set_col_cnt(aprs_table, 2);
          lv_table_set_row_cnt(aprs_table, APRS_MAX_PACKETS + 1);
          
          // 设置表格样式
          lv_obj_set_style_text_font(aprs_table, &lvgl_font_song_16, 0);
          lv_obj_set_style_text_color(aprs_table, lv_color_hex(0x000000), 0);
          lv_obj_set_style_bg_color(aprs_table, lv_color_hex(0xFFFFFF), 0);
          
          // 设置表格宽度和高度
          lv_obj_set_width(aprs_table, screenWidth - 20);
          lv_obj_set_height(aprs_table, 300);
          
          // 设置列宽
          lv_table_set_col_width(aprs_table, 0, (screenWidth - 30) * 0.30);
          lv_table_set_col_width(aprs_table, 1, (screenWidth - 30) * 0.70);
          
          // 设置表格位置
          lv_obj_align(aprs_table, LV_ALIGN_TOP_MID, 0, 100);
        }
        
        // 确保标签和表格可见
        lv_obj_clear_flag(aprs_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(aprs_table, LV_OBJ_FLAG_HIDDEN);
        
        // 更新标签显示
        lv_label_set_text(aprs_label, "显示本地APRS数据");
        
        // 表头设置
        lv_table_set_cell_value(aprs_table, 0, 0, "呼号");
        lv_table_set_cell_value(aprs_table, 0, 1, "信息");
        
        // 设置表头样式
        lv_obj_set_style_bg_color(aprs_table, lv_color_hex(0xCCCCCC), LV_PART_ITEMS | LV_STATE_DEFAULT);
        
        // 清空之前的数据
        for (int i = 1; i <= APRS_MAX_PACKETS; i++) {
          lv_table_set_cell_value(aprs_table, i, 0, "");
          lv_table_set_cell_value(aprs_table, i, 1, "");
        }
        
        // 检查是否包含result对象
        if (doc.containsKey("result") && doc["result"].is<JsonObject>()) {
          JsonObject result = doc["result"].as<JsonObject>();
          
          // 显示APRS数据
          if (result.containsKey("packets") && result["packets"].is<JsonArray>()) {
            JsonArray packets = result["packets"];
            int row = 1;
            
            // 显示最多APRS_MAX_PACKETS条数据
            int displayCount = min(packets.size(), (size_t)APRS_MAX_PACKETS);
            for (int i = 0; i < displayCount; i++) {
              JsonObject packet = packets[i];
              
              // 获取呼号
              const char* callsign = packet["callsign"].is<const char*>() ? packet["callsign"].as<const char*>() : "未知";
              
              // 获取信息
              const char* raw_data = packet["raw_data"].is<const char*>() ? packet["raw_data"].as<const char*>() : "";
              
              // 设置表格内容
              lv_table_set_cell_value(aprs_table, row, 0, callsign);
              lv_table_set_cell_value(aprs_table, row, 1, raw_data);
              
              row++;
            }
            
            // 如果没有数据，显示提示
            if (packets.size() == 0) {
              lv_table_set_cell_value(aprs_table, 1, 0, "暂无数据");
              lv_table_set_cell_value(aprs_table, 1, 1, "");
            }
          }
          
          // 添加最后更新时间到标签
          if (result.containsKey("last_update") && result["last_update"].is<const char*>()) {
            lv_label_set_text(aprs_label, ("显示本地APRS数据 (" + 
                                           String(result.containsKey("total") ? result["total"].as<int>() : 0) + 
                                           "条数据)\n最后更新: " + 
                                           String(result["last_update"].as<const char*>())).c_str());
          }
        }
        
        Serial.println("成功显示文件中的APRS数据");
      } else {
        Serial.print("解析APRS数据文件失败: ");
        Serial.println(error.c_str());
        
        if (aprs_label && lv_obj_is_valid(aprs_label)) {
          lv_label_set_text(aprs_label, "本地APRS数据解析失败");
        }
      }
    } else {
      Serial.println("无法打开APRS数据文件");
      
      if (aprs_label && lv_obj_is_valid(aprs_label)) {
        lv_label_set_text(aprs_label, "无法打开本地APRS数据文件");
      }
    }
  } else {
    Serial.println("APRS数据文件不存在");
    
    if (aprs_label && lv_obj_is_valid(aprs_label)) {
      lv_label_set_text(aprs_label, "本地APRS数据文件不存在");
    }
  }
}