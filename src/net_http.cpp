#include "net_http.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include "weather_display.h"
#include "data_manager.h"
#include <time.h>
#include <map>
#include <vector>
#include <SPIFFS.h>

// 定义强制刷新标志
bool forceRefreshWeather = false;
bool forceRefreshNews = false;
bool forceRefreshAstronauts = false;
bool forceRefreshIciba = false;

// 外部变量声明
extern lv_obj_t* hour_minute_label;
extern lv_obj_t* second_label;
extern lv_obj_t* weekday_label;
extern lv_obj_t* date_label;
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
    // 获取时间失败时，所有标签显示默认值
    if (date_label) {
      lv_label_set_text(date_label, "----年--月--日");
    }
    if (hour_minute_label) {
      lv_label_set_text(hour_minute_label, "--:--");
    }
    if (weekday_label) {
      lv_label_set_text(weekday_label, "--");
    }
    return;
  }
  
  // 格式化日期字符串（YYYY年MM月DD日）
  static char dateStringBuff[15]; // 使用static减少堆栈分配
  sprintf(dateStringBuff, "%d年%02d月%02d日", 
          timeinfo.tm_year + 1900, // tm_year是从1900开始的年份
          timeinfo.tm_mon + 1,     // tm_mon是从0开始的月份
          timeinfo.tm_mday);       // tm_mday是日期
  
  // 格式化时分字符串（格式：HH:MM）
  static char hourMinuteStringBuff[6]; // 使用static减少堆栈分配
  strftime(hourMinuteStringBuff, sizeof(hourMinuteStringBuff), "%H:%M", &timeinfo);

  // 格式化星期几字符串
  static char weekdayStringBuff[7]; // 使用static减少堆栈分配
  // 使用中文星期几格式
  const char* weekdays[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
  int wday = timeinfo.tm_wday; // tm_wday: 0=星期日, 1=星期一, ..., 6=星期六
  strcpy(weekdayStringBuff, weekdays[wday]);
  
  // 更新日期标签
  if (date_label && strcmp(lv_label_get_text(date_label), dateStringBuff) != 0) {
    lv_label_set_text(date_label, dateStringBuff);
  }
  
  // 确保hour_minute_label已创建并放置在适当位置
  if (!hour_minute_label) {
    hour_minute_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(hour_minute_label, &lv_font_montserrat_48, 0); // 48px字体
    lv_obj_set_style_text_color(hour_minute_label, lv_color_hex(0xFF69B4), 0); // 粉红色
    lv_obj_align(hour_minute_label, LV_ALIGN_TOP_LEFT, 5, 30); // 放在第一排下方，与标题按钮底对齐，左移5像素
  }
  
  // 更新小时分钟标签
  if (hour_minute_label && strcmp(lv_label_get_text(hour_minute_label), hourMinuteStringBuff) != 0) {
    lv_label_set_text(hour_minute_label, hourMinuteStringBuff);
    // 确保标签可见
    if (lv_obj_has_flag(hour_minute_label, LV_OBJ_FLAG_HIDDEN)) {
      lv_obj_clear_flag(hour_minute_label, LV_OBJ_FLAG_HIDDEN);
    }
  }
  
  // 更新星期几标签
  if (weekday_label && strcmp(lv_label_get_text(weekday_label), weekdayStringBuff) != 0) {
    lv_label_set_text(weekday_label, weekdayStringBuff);
  }
  
  // 检查是否需要更新新闻和天气数据
  if (DataManager::getInstance()->getIsFirstStartup()) {
    // 首次启动，立即获取数据
    getCityWeater();
    getNews();
    getIcibaDailyInfo();
    DataManager::getInstance()->setIsFirstStartup(false);
  } else if (WiFi.status() == WL_CONNECTED) {
    // 检查是否为整点且分钟为0，小时能被2整除
    if (timeinfo.tm_min == 0 && timeinfo.tm_hour % 2 == 0 && timeinfo.tm_sec < 10) {
      // 是整点，且小时能被2整除（即2小时整点）
      // 检查是否已经到了更新时间（避免重复更新）
      static int lastUpdatedHour = -1;
      if (lastUpdatedHour != timeinfo.tm_hour) {
        getCityWeater();
        getNews();
        getIcibaDailyInfo();
        lastUpdatedHour = timeinfo.tm_hour;
        Serial.println("整点更新数据完成");
      }
    }
  }
}

// 更新秒钟显示（每秒更新一次）
void updateSecondDisplay() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    // 获取时间失败时，秒标签显示默认值
    if (second_label) {
      lv_label_set_text(second_label, "--");
    }
    return;
  }
  
  // 格式化秒钟字符串（格式：SS）
  static char secondStringBuff[3]; // 使用static减少堆栈分配
  sprintf(secondStringBuff, "%02d", timeinfo.tm_sec);

  // 确保second_label已创建并放置在适当位置
  if (!second_label) {
    second_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(second_label, &lv_font_montserrat_32, 0); // 32px字体
    lv_obj_set_style_text_color(second_label, lv_color_hex(0x0000FF), 0); // 蓝色
    // 放置在hour_minute_label右侧，保持垂直居中对齐
    lv_obj_align_to(second_label, hour_minute_label, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
  }
  
  // 确保秒标签可见
  if (second_label && lv_obj_has_flag(second_label, LV_OBJ_FLAG_HIDDEN)) {
    lv_obj_clear_flag(second_label, LV_OBJ_FLAG_HIDDEN);
  }
  
  // 更新秒标签
  if (second_label && strcmp(lv_label_get_text(second_label), secondStringBuff) != 0) {
    lv_label_set_text(second_label, secondStringBuff);
  }
}

// 更新时间显示（整合函数，供外部调用）
void updateTimeDisplay() {
  // 每秒更新一次秒显示
  updateSecondDisplay();
  
  // 每秒更新一次分钟显示
  updateMinuteDisplay();
}

// 获取金山词霸每日信息
void getIcibaDailyInfo() {
  // 检查是否需要从网络获取数据（首次启动或整点更新时）
  if (DataManager::getInstance()->getIsFirstStartup() || forceRefreshIciba) {
    HTTPClient httpClient;
    
    // 检查WiFi连接状态
    if (WiFi.status() != WL_CONNECTED) {
      forceRefreshIciba = false;
      return;
    }
    
    // 金山词霸API URL
    String URL = "https://open.iciba.com/dsapi/";
    
    // 创建HTTPClient对象
    httpClient.begin(URL);
    
    // 设置请求头中的User-Agent
    httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
    
    Serial.println("正在获取金山词霸每日信息");
    Serial.println(URL);
    
    // 定义icibaData变量并在函数开始时初始化
    JsonDocument icibaData;
    bool icibaDataValid = false;
    
    // 启动连接并发送HTTP请求
    int httpCode = httpClient.GET();
    
    // 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
    if (httpCode == HTTP_CODE_OK) {
      String icibaStr = httpClient.getString();
      Serial.println("获取金山词霸每日信息成功");
      
      // 解析JSON数据
      Serial.println("完整金山词霸数据响应：");
      Serial.println(icibaStr); // 打印完整的响应
      DeserializationError error = deserializeJson(icibaData, icibaStr);
      if (!error) {
        icibaDataValid = true;
      }
      
      if (error) {
        Serial.print(F("反序列化JSON失败: "));
        Serial.println(error.f_str());
        httpClient.end();
        forceRefreshIciba = false;
        return;
      }
      
      // 检查是否有数据
      if (icibaData["content"].is<const char*>() && icibaData["note"].is<const char*>()) {
        // 更新最后更新时间戳
        lastIcibaUpdateTime = millis();
      } else {
        Serial.println("暂无金山词霸数据");
      }
    } else {
      Serial.printf("HTTP请求失败，错误码: %d\n", httpCode);
    }
    
    // 将数据写入JSON文件
    if (icibaDataValid && icibaData["content"].is<const char*>() && icibaData["note"].is<const char*>()) {
        JsonDocument icibaJson;
        icibaJson["content"] = icibaData["content"];
        icibaJson["note"] = icibaData["note"];
        icibaJson["update_time"] = millis();
        writeJsonToFile("/iciba.json", icibaJson);
    }
    
    httpClient.end();
    forceRefreshIciba = false;
  }
}

// 获取天气信息
void getCityWeater() {
  Serial.println("开始获取天气信息");
  
  // 确保weather_label已创建（用于显示状态信息）
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
  
  // 检查是否需要从网络获取数据（首次启动或整点更新时）
  if (DataManager::getInstance()->getIsFirstStartup() || forceRefreshWeather) {
    // 检查WiFi连接状态
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi未连接，无法获取天气");
      forceRefreshWeather = false;
      return;
    }
    
    // 调用新的天气数据获取函数
    Serial.println("正在获取天气数据");
    int result = Http_Get_wthrcdn();
    
    if (result == 1) {
      Serial.println("获取天气数据成功");
      
      // 将天气数据写入JSON文件
      JsonDocument weatherJson;
      weatherJson["city"] = epd_rtc.city;
      weatherJson["current_temp"] = epd_rtc.Weather_Tmp_Real;
      weatherJson["low_temp"] = epd_rtc.Weather_Tmp_Low;
      weatherJson["high_temp"] = epd_rtc.Weather_Tmp_High;
      weatherJson["weather_type"] = epd_rtc.Weather_Type;
      weatherJson["weather_code"] = epd_rtc.Weather_TypeCode;
      weatherJson["wind"] = epd_rtc.Wind_Direction;
      weatherJson["humidity"] = epd_rtc.humidity;
      weatherJson["limit_number"] = epd_rtc.limitnumber;
      weatherJson["suggestion"] = epd_rtc.ys;
      weatherJson["update_time"] = millis();
      
      // 添加预报数据
      JsonArray forecast = weatherJson["forecast"].to<JsonArray>();
      for (int i = 0; i < 5; i++) {
        JsonObject dayForecast = forecast.add<JsonObject>();
        dayForecast["date"] = epd_rtc.Weather_data[i];
        dayForecast["high_temp"] = epd_rtc.tmp_max_data[i][0];
        dayForecast["low_temp"] = epd_rtc.tmp_min_data[i][0];
        dayForecast["day_code"] = epd_rtc.day_code[i][0];
        dayForecast["night_code"] = epd_rtc.night_code[i][0];
      }
      
      // 添加当前天气详情
      JsonObject current = weatherJson["current"].to<JsonObject>();
      current["temp"] = epd_rtc.Weather_Tmp_Real;
      current["humidity"] = epd_rtc.humidity;
      current["wind_speed"] = epd_rtc.Wind_Direction;
      JsonObject condition = current["condition"].to<JsonObject>();
      condition["text"] = epd_rtc.Weather_Type;
      condition["code"] = epd_rtc.Weather_TypeCode;
      
      writeJsonToFile("/weather.json", weatherJson);
      
      // 更新天气数据的最后更新时间戳
      lastWeatherUpdateTime = millis();
    } else {
      Serial.println("获取天气数据失败");
    }
    
    forceRefreshWeather = false;
  }
}


// 获取新闻
void getNews() {
  // 检查是否需要从网络获取数据（首次启动或整点更新时）
  if (DataManager::getInstance()->getIsFirstStartup() || forceRefreshNews) {
    HTTPClient httpClient;
    
    // 检查WiFi连接状态
    if (WiFi.status() != WL_CONNECTED) {
      // 如果有缓存数据，则使用缓存数据
      forceRefreshNews = false;
      return;
    }
    
    String URL = "http://apis.tianapi.com/bulletin/index?key=" + String(API_KEY);
    
    // 创建HTTPClient对象
    httpClient.begin(URL);
    
    // 设置请求头中的User-Agent
    httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
    httpClient.addHeader("Referer", "http://apis.tianapi.com/");
    
    Serial.println("正在获取今日简报");
    Serial.println(URL);
    
    // 启动连接并发送HTTP请求
    int httpCode = httpClient.GET();
    
    // 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
    if (httpCode == HTTP_CODE_OK) {
      String newsStr = httpClient.getString();
      Serial.println("获取今日简报成功");
      
      // 解析JSON数据
      JsonDocument newsData;
      DeserializationError error = deserializeJson(newsData, newsStr);
      
      if (error) {
        Serial.print(F("反序列化JSON失败: "));
        Serial.println(error.f_str());
        httpClient.end();
        forceRefreshNews = false;
        return;
      }
      
      // 将新闻数据写入JSON文件
      JsonDocument newsJson;
      newsJson["update_time"] = millis();
      JsonArray newsItems = newsJson["news"].to<JsonArray>();
      
      // 获取前10条新闻
      if (newsData["result"].is<JsonObject>() && newsData["result"]["list"].size() > 0) {
        for (int i = 0; i < min(10, static_cast<int>(newsData["result"]["list"].size())); i++) {
          if (newsData["result"]["list"][i]["title"].is<const char*>()) {
            const char* title = newsData["result"]["list"][i]["title"].as<const char*>();
            JsonObject newsItem = newsItems.add<JsonObject>();
            newsItem["id"] = i;
            newsItem["title"] = title;
          }
        }
      }
      
      writeJsonToFile("/news.json", newsJson);
      
      // 更新新闻数据的最后更新时间戳
      lastNewsUpdateTime = millis();
    } else {
      Serial.printf("HTTP请求失败，错误码: %d\n", httpCode);
    }
    
    httpClient.end();
    forceRefreshNews = false;
  }
}

// 标记为强制刷新数据（用于整点更新时调用）
void forceRefreshData() {
  forceRefreshNews = true;
  forceRefreshWeather = true;
  forceRefreshIciba = true;
  // 强制刷新宇航员数据
  lastAstronautsUpdateTime = 0;
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
  // 检查WiFi连接状态
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi未连接，无法获取宇航员信息");
    forceRefreshAstronauts = false;
    return;
  }
  
  // 检查是否需要更新数据（首次启动或整点更新时）
  if (DataManager::getInstance()->getIsFirstStartup() || forceRefreshAstronauts || lastAstronautsUpdateTime == 0) {
    HTTPClient httpClient;
    
    String URL = "http://api.open-notify.org/astros.json";
    
    // 创建HTTPClient对象
    httpClient.begin(URL);
    
    Serial.println("正在获取宇航员信息");
    
    // 启动连接并发送HTTP请求
    int httpCode = httpClient.GET();
    
    // 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
    if (httpCode == HTTP_CODE_OK) {
      String astronautsStr = httpClient.getString();
      Serial.println("获取宇航员信息成功");
      
      // 解析JSON数据
      JsonDocument astronautsData;
      DeserializationError error = deserializeJson(astronautsData, astronautsStr);
      
      if (error) {
        Serial.print(F("反序列化JSON失败: "));
        Serial.println(error.f_str());
        httpClient.end();
        forceRefreshAstronauts = false;
        return;
      }
      
      // 将宇航员数据写入JSON文件
      JsonDocument astronautsJson;
      astronautsJson["update_time"] = millis();
      astronautsJson["total"] = astronautsData["number"];
      
      JsonArray people = astronautsJson["people"].to<JsonArray>();
      
      if (astronautsData["people"].is<JsonArray>()) {
        for (JsonObject person : astronautsData["people"].as<JsonArray>()) {
          JsonObject personObj = people.add<JsonObject>();
          if (person["name"].is<const char*>()) {
            personObj["name"] = person["name"].as<const char*>();
          }
          if (person["craft"].is<const char*>()) {
            personObj["craft"] = person["craft"].as<const char*>();
          }
        }
      }
      
      writeJsonToFile("/astronauts.json", astronautsJson);
      
      // 更新宇航员数据的最后更新时间戳
      lastAstronautsUpdateTime = millis();
    } else {
      Serial.printf("HTTP请求失败，错误码: %d\n", httpCode);
    }
    
    httpClient.end();
    forceRefreshAstronauts = false;
  }
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
  
  // 将APRS数据写入JSON文件
  if (!aprsPacketCache.empty()) {
      JsonDocument aprsJson;
      aprsJson["update_time"] = millis();
      aprsJson["total_packets"] = aprsPacketCache.size();
      JsonArray packetsArray = aprsJson["packets"].to<JsonArray>();
      
      // 倒序添加，最新的在前面
      for (auto it = aprsPacketCache.rbegin(); it != aprsPacketCache.rend(); ++it) {
          JsonObject packetObj = packetsArray.add<JsonObject>();
          packetObj["callsign"] = it->callSign;
          packetObj["info"] = it->info;
          packetObj["timestamp"] = it->timestamp;
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
        
        // 获取当前天气信息
        if (doc["current"].is<JsonObject>()) {
          JsonObject current = doc["current"];
          if (current["temp"].is<float>()) {
            weatherContent += "当前温度: " + String(current["temp"].as<float>()) + "°C\n";
          }
          if (current["humidity"].is<int>()) {
            weatherContent += "湿度: " + String(current["humidity"].as<int>()) + "%\n";
          }
          if (current["wind_speed"].is<float>()) {
            weatherContent += "风速: " + String(current["wind_speed"].as<float>()) + " km/h\n";
          }
          if (current["condition"].is<JsonObject>()) {
            JsonObject condition = current["condition"];
            if (condition["text"].is<const char*>()) {
              weatherContent += "天气状况: " + String(condition["text"].as<const char*>()) + "\n";
            }
          }
          weatherContent += "\n";
        }
        
        // 获取每日天气预报
        if (doc["forecast"].is<JsonObject>() && doc["forecast"]["forecastday"].is<JsonArray>()) {
          JsonArray forecastday = doc["forecast"]["forecastday"];
          
          for (JsonObject day : forecastday) {
            if (day["date"].is<const char*>()) {
              weatherContent += "日期: " + String(day["date"].as<const char*>()) + "\n";
            }
            if (day["day"].is<JsonObject>()) {
              JsonObject dayWeather = day["day"];
              if (dayWeather["maxtemp_c"].is<float>()) {
                weatherContent += "最高温度: " + String(dayWeather["maxtemp_c"].as<float>()) + "°C\n";
              }
              if (dayWeather["mintemp_c"].is<float>()) {
                weatherContent += "最低温度: " + String(dayWeather["mintemp_c"].as<float>()) + "°C\n";
              }
              if (dayWeather["condition"].is<JsonObject>()) {
                JsonObject condition = dayWeather["condition"];
                if (condition["text"].is<const char*>()) {
                  weatherContent += "天气状况: " + String(condition["text"].as<const char*>()) + "\n";
                }
              }
              weatherContent += "\n";
            }
          }
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
        
        // 检查是否有新闻数据
        if (doc["news"].is<JsonArray>()) {
          JsonArray news = doc["news"];
          
          // 显示最多10条新闻
          int displayCount = min((int)news.size(), 10);
          for (int i = 0; i < displayCount; i++) {
            JsonObject newsItem = news[i];
            
            // 检查是否有标题
            if (newsItem["title"].is<const char*>()) {
              newsContent += String(i + 1) + ". " + String(newsItem["title"].as<const char*>()) + "\n\n";
            }
          }
          
          // 如果没有新闻数据，显示提示信息
          if (news.size() == 0) {
            newsContent = "暂无新闻数据";
          }
        } else {
          newsContent = "暂无新闻数据";
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
        
        // 获取句子信息
        if (doc["content"].is<const char*>()) {
          displayText += String(doc["content"].as<const char*>()) + "\n\n";
        }
        
        // 获取翻译信息
        if (doc["note"].is<const char*>()) {
          displayText += String(doc["note"].as<const char*>());
        }
        
        // 获取日期信息
        if (doc["dateline"].is<const char*>()) {
          displayText += "\n\n" + String(doc["dateline"].as<const char*>());
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
        
        // 显示APRS数据
        if (doc["packets"].is<JsonArray>()) {
          JsonArray packets = doc["packets"];
          int row = 1;
          
          // 显示最多APRS_MAX_PACKETS条数据
          int displayCount = min(packets.size(), (size_t)APRS_MAX_PACKETS);
          for (int i = 0; i < displayCount; i++) {
            JsonObject packet = packets[i];
            
            // 获取呼号
            const char* callsign = packet["callsign"].is<const char*>() ? packet["callsign"].as<const char*>() : "未知";
            
            // 获取信息
            const char* info = packet["info"].is<const char*>() ? packet["info"].as<const char*>() : "";
            
            // 设置表格内容
            lv_table_set_cell_value(aprs_table, row, 0, callsign);
            lv_table_set_cell_value(aprs_table, row, 1, info);
            
            row++;
          }
          
          // 如果没有数据，显示提示
          if (packets.size() == 0) {
            lv_table_set_cell_value(aprs_table, 1, 0, "暂无数据");
            lv_table_set_cell_value(aprs_table, 1, 1, "");
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