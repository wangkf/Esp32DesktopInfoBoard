#include "net_http.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include "weather_display.h"
#include <time.h>
#include <map>
#include <vector>

// 外部变量声明
extern lv_obj_t* hour_minute_label;
extern lv_obj_t* second_label;
extern lv_obj_t* weekday_label;
extern lv_obj_t* date_label;
extern lv_obj_t* wifi_icon_label;
extern const char* ntpServer;

extern unsigned long lastUpdateTime;
extern const long updateInterval;

// 全局变量：标记是否首次启动
bool isFirstStartup = true;

// 城市代码定义
String cityCode = "101010100"; // 默认北京

// 强制刷新标志
bool forceRefreshNews = false;
bool forceRefreshWeather = false;
bool forceRefreshIciba = false;

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
  if (isFirstStartup) {
    // 首次启动，立即获取数据
    getCityWeater();
    getNews();
    getIcibaDailyInfo();
    isFirstStartup = false;
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
    // 更新WiFi图标状态
    if (wifi_icon_label) {
      if (WiFi.status() == WL_CONNECTED) {
        lv_label_set_text(wifi_icon_label, LV_SYMBOL_WIFI);
        lv_obj_set_style_text_color(wifi_icon_label, lv_color_hex(0x00AA00), 0); // 连接成功为绿色
      } else {
        lv_label_set_text(wifi_icon_label, LV_SYMBOL_WIFI);
        lv_obj_set_style_text_color(wifi_icon_label, lv_color_hex(0xAA0000), 0); // 未连接为红色
      }
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
  
  // 更新WiFi图标状态
  if (wifi_icon_label) {
    if (WiFi.status() == WL_CONNECTED) {
      lv_label_set_text(wifi_icon_label, LV_SYMBOL_WIFI);
      lv_obj_set_style_text_color(wifi_icon_label, lv_color_hex(0x00AA00), 0); // 连接成功为绿色
    } else {
      lv_label_set_text(wifi_icon_label, LV_SYMBOL_WIFI);
      lv_obj_set_style_text_color(wifi_icon_label, lv_color_hex(0xAA0000), 0); // 未连接为红色
    }
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
  if (isFirstStartup || forceRefreshIciba) {
    HTTPClient httpClient;
    
    // 检查WiFi连接状态
    if (WiFi.status() != WL_CONNECTED) {
      // 如果有缓存数据，则使用缓存数据
      if (iciba_label != NULL && cachedIcibaData.length() > 0) {
        lv_label_set_text(iciba_label, cachedIcibaData.c_str());
        Serial.println("WiFi未连接，使用缓存金山词霸数据");
      } else {
        Serial.println("WiFi未连接，无法获取金山词霸每日信息");
      }
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
    
    // 启动连接并发送HTTP请求
    int httpCode = httpClient.GET();
    
    // 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
    if (httpCode == HTTP_CODE_OK) {
      String icibaStr = httpClient.getString();
      Serial.println("获取金山词霸每日信息成功");
      
      // 解析JSON数据
      Serial.println("完整金山词霸数据响应：");
      Serial.println(icibaStr); // 打印完整的响应
      DynamicJsonDocument icibaData(4096);
      DeserializationError error = deserializeJson(icibaData, icibaStr);
      
      if (error) {
        Serial.print(F("反序列化JSON失败: "));
        Serial.println(error.f_str());
        // 如果有缓存数据，则使用缓存数据
        if (iciba_label != NULL && cachedIcibaData.length() > 0) {
          lv_label_set_text(iciba_label, cachedIcibaData.c_str());
          Serial.println("解析失败，使用缓存金山词霸数据");
        }
        httpClient.end();
        forceRefreshIciba = false;
        return;
      }
      
      // 检查是否有数据
      if (icibaData.containsKey("content") && icibaData.containsKey("note")) {
        const char* content = icibaData["content"].as<const char*>();
        const char* note = icibaData["note"].as<const char*>();
        
        // 创建标签（如果尚未创建）
        if (iciba_label == NULL || !lv_obj_is_valid(iciba_label)) {
          if (iciba_label != NULL) {
            // 确保旧标签被正确清理
            lv_obj_del(iciba_label);
          }
          iciba_label = lv_label_create(lv_scr_act());
          lv_label_set_long_mode(iciba_label, LV_LABEL_LONG_WRAP);
          lv_obj_set_size(iciba_label, screenWidth - 20, screenHeight - 150); // 调整大小，确保不超出屏幕
          lv_obj_align(iciba_label, LV_ALIGN_TOP_MID, 0, 120); // 上对齐，离顶部120px
          lv_obj_set_style_text_font(iciba_label, &lvgl_font_song_16, 0);
          lv_obj_set_style_text_color(iciba_label, lv_color_hex(0x333333), 0); // 深灰色文字
          
          // 添加背景和边框效果
          lv_obj_set_style_bg_color(iciba_label, lv_color_hex(0xFFFFFF), 0); // 白色背景
          lv_obj_set_style_bg_opa(iciba_label, 70, 0); // 半透明背景
          lv_obj_set_style_border_width(iciba_label, 2, 0); // 边框宽度
          lv_obj_set_style_border_color(iciba_label, lv_color_hex(0xCCCCCC), 0); // 灰色边框
          lv_obj_set_style_radius(iciba_label, 10, 0); // 圆角
          lv_obj_set_style_pad_all(iciba_label, 10, 0); // 内边距
        }
        
        // 构建显示内容
        String displayText = "今日格言：\n";
        displayText += content;
        displayText += "\n\n";
        displayText += "翻译：";
        displayText += note;
        
        // 显示内容
        lv_label_set_text(iciba_label, displayText.c_str());
        
        // 保存到缓存
        cachedIcibaData = displayText;
        
        // 更新最后更新时间戳
        lastIcibaUpdateTime = millis();
      } else {
        Serial.println("暂无金山词霸数据");
        // 如果有缓存数据，则使用缓存数据
        if (iciba_label != NULL && cachedIcibaData.length() > 0) {
          lv_label_set_text(iciba_label, cachedIcibaData.c_str());
          Serial.println("暂无数据，使用缓存金山词霸数据");
        }
      }
    } else {
      Serial.printf("HTTP请求失败，错误码: %d\n", httpCode);
      // 如果有缓存数据，则使用缓存数据
      if (iciba_label != NULL && cachedIcibaData.length() > 0) {
        lv_label_set_text(iciba_label, cachedIcibaData.c_str());
        Serial.println("获取失败，使用缓存金山词霸数据");
      }
    }
    
    httpClient.end();
    forceRefreshIciba = false;
  } else if (iciba_label != NULL && cachedIcibaData.length() > 0) {
    // 非更新时间，直接显示缓存数据
    lv_label_set_text(iciba_label, cachedIcibaData.c_str());
  }
}

// 获取天气信息
void getCityWeater() {
  Serial.println("开始获取天气信息");
  
  // 检查是否需要从网络获取数据（首次启动或整点更新时）
  if (isFirstStartup || forceRefreshWeather) {
    HTTPClient httpClient;
    
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
      
      // 添加背景和边框效果
      lv_obj_set_style_bg_color(weather_label, lv_color_hex(0xFFFFFF), 0); // 白色背景
      lv_obj_set_style_bg_opa(weather_label, 70, 0); // 半透明背景
      lv_obj_set_style_border_width(weather_label, 2, 0); // 边框宽度
      lv_obj_set_style_border_color(weather_label, lv_color_hex(0xCCCCCC), 0); // 灰色边框
      lv_obj_set_style_radius(weather_label, 10, 0); // 圆角
      lv_obj_set_style_pad_all(weather_label, 10, 0); // 内边距
      
      Serial.println("创建或重新创建了weather_label");
    }
    
    // 检查WiFi连接状态
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi未连接，无法获取天气");
      lv_label_set_text(weather_label, "WiFi未连接，无法获取天气数据");
      lv_obj_clear_flag(weather_label, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(weather_label);
      
      // 如果有缓存数据，则使用缓存数据
      if (weatherDisplay.isInitialized()) {
        showWeatherDisplay();
        Serial.println("WiFi未连接，使用缓存天气数据");
      }
      forceRefreshWeather = false;
      return;
    }
    
    // 使用城市代码调用天气API
    String URL = "http://apis.tianapi.com/tianqi/index?key=" + String(API_KEY) + "&city=" + cityCode;
    
    // 创建HTTPClient对象
    httpClient.begin(URL);
    
    // 设置请求头中的User-Agent
    httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
    httpClient.addHeader("Referer", "http://apis.tianapi.com/");
    
    Serial.println("正在获取天气数据");
    Serial.println(URL);
    
    // 启动连接并发送HTTP请求
    int httpCode = httpClient.GET();
    
    // 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
    if (httpCode == HTTP_CODE_OK) {
      String weatherStr = httpClient.getString();
      Serial.println("获取天气数据成功");
      
      // 解析JSON数据
      Serial.println("完整天气数据响应：");
      Serial.println(weatherStr); // 打印完整的天气数据响应
      DynamicJsonDocument weatherData(4096);
      DeserializationError error = deserializeJson(weatherData, weatherStr);
      
      if (error) {
        Serial.print(F("反序列化JSON失败: "));
        Serial.println(error.f_str());
        lv_label_set_text(weather_label, "解析天气数据失败");
        lv_obj_clear_flag(weather_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(weather_label);
        
        // 如果有缓存数据，则使用缓存数据
        if (weatherDisplay.isInitialized()) {
          showWeatherDisplay();
          Serial.println("解析失败，使用缓存天气数据");
        }
        httpClient.end();
        forceRefreshWeather = false;
        return;
      }
      
      // 检查是否有天气数据
      if (weatherData.containsKey("result") && weatherData["result"].containsKey("list")) {
        // 获取今日天气数据
        JsonObject todayWeather = weatherData["result"]["list"][0];
        const char* date = todayWeather["date"].as<const char*>();
        const char* week = todayWeather["week"].as<const char*>();
        const char* weather = todayWeather["weather"].as<const char*>();
        const char* temp_low = todayWeather["lowest"].as<const char*>();
        const char* temp_high = todayWeather["highest"].as<const char*>();
        const char* temp_now = todayWeather.containsKey("real") ? todayWeather["real"].as<const char*>() : "";
        const char* humidity = todayWeather.containsKey("humidity") ? todayWeather["humidity"].as<const char*>() : "";
        const char* wind = todayWeather["wind"].as<const char*>();
        const char* wind_level = todayWeather["windsc"].as<const char*>();
        
        Serial.printf("天气数据解析成功: 日期=%s, 星期=%s, 天气=%s, 温度范围=%s-%s, 当前温度=%s\n", 
                     date, week, weather, temp_low, temp_high, temp_now);
        
        // 构建完整天气信息
        String weatherContent = "";
        // 添加详细天气信息
        if (strlen(temp_now) > 0) {
          weatherContent += "当前温度:" + String(temp_now);
        }
        
        if (strlen(humidity) > 0) {
          weatherContent += "/湿度:" + String(humidity) + "% ";
        }
        weatherContent += String(weather) + String("\n");
        weatherContent += String(temp_low) + String("-") + String(temp_high);
        weatherContent += String(wind) + String(" ") + String(wind_level) + String("级\n");
        
        // 检查并创建weather_label
        if (weather_label == NULL || !lv_obj_is_valid(weather_label)) {
          if (weather_label != NULL) {
            // 确保旧标签被正确清理
            lv_obj_del(weather_label);
          }
          weather_label = lv_label_create(lv_scr_act());
          lv_obj_set_style_text_font(weather_label, &lvgl_font_song_16, 0);
          // 根据天气状况设置不同的文字颜色
          if (String(weather).indexOf("晴") != -1) {
            lv_obj_set_style_text_color(weather_label, lv_color_hex(0xFF6600), 0); // 橙色
          } else if (String(weather).indexOf("雨") != -1) {
            lv_obj_set_style_text_color(weather_label, lv_color_hex(0x0066CC), 0); // 蓝色
          } else if (String(weather).indexOf("阴") != -1 || String(weather).indexOf("云") != -1) {
            lv_obj_set_style_text_color(weather_label, lv_color_hex(0x666666), 0); // 灰色
          } else {
            lv_obj_set_style_text_color(weather_label, lv_color_hex(0x0000FF), 0); // 默认蓝色
          }
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
        }
        
        // 确保天气内容不为空
        if (weatherContent.length() > 0) {
            Serial.printf("设置天气标签内容: %s\n", weatherContent.c_str());
            
            // 设置天气内容
            lv_label_set_text(weather_label, weatherContent.c_str());
            
            // 确保标签可见
            lv_obj_clear_flag(weather_label, LV_OBJ_FLAG_HIDDEN);
            
            // 确保标签在最上层
            lv_obj_move_foreground(weather_label);
            
            // 初始化天气显示组件
            if (!weatherDisplay.isInitialized()) {
                Serial.println("初始化天气显示组件");
                initWeatherDisplay();
            }
            
            // 更新未来四天天气数据到图标和图表
            for (int i = 0; i < 4 && i < weatherData["result"]["list"].size() - 1; i++) {
                JsonObject dailyWeather = weatherData["result"]["list"][i+1]; // 从第二天开始显示
                updateWeatherIcon(dailyWeather, i);
            }
            
            // 刷新天气显示
            Serial.println("刷新天气显示");
            refreshWeatherDisplay();
            
            // 更新天气数据的最后更新时间戳
            lastWeatherUpdateTime = millis();
            
            // 保存原始天气数据到缓存（可选，用于复杂的缓存恢复）
            cachedWeatherData = weatherStr;
        } else {
            // 如果天气内容为空，使用默认信息
            Serial.println("天气内容为空，使用默认信息");
            lv_label_set_text(weather_label, "暂无天气数据，请稍后刷新");
            lv_obj_clear_flag(weather_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(weather_label);
        }
      } else {
        Serial.println("暂无天气数据");
        lv_label_set_text(weather_label, "暂无天气数据");
        lv_obj_clear_flag(weather_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(weather_label);
        
        // 如果有缓存数据，则使用缓存数据
        if (weatherDisplay.isInitialized()) {
          showWeatherDisplay();
          Serial.println("暂无天气数据，使用缓存天气数据");
        }
      }
    } else {
      Serial.printf("HTTP请求失败，错误码: %d\n", httpCode);
      lv_label_set_text(weather_label, "获取天气数据失败");
      lv_obj_clear_flag(weather_label, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(weather_label);
      
      // 如果有缓存数据，则使用缓存数据
        if (weatherDisplay.isInitialized()) {
          showWeatherDisplay();
          Serial.println("获取失败，使用缓存天气数据");
        }
    }
    
    httpClient.end();
    forceRefreshWeather = false;
  } else {
    // 非更新时间，检查weather_label并显示缓存数据
    if (weather_label == NULL || !lv_obj_is_valid(weather_label)) {
      Serial.println("非更新时间，但weather_label为空，创建它");
      weather_label = lv_label_create(lv_scr_act());
      lv_obj_set_style_text_font(weather_label, &lvgl_font_song_16, 0);
      lv_obj_set_style_text_color(weather_label, lv_color_hex(0x0000FF), 0); // 默认蓝色
      lv_obj_set_style_text_align(weather_label, LV_TEXT_ALIGN_LEFT, 0);
      lv_label_set_long_mode(weather_label, LV_LABEL_LONG_WRAP);
      lv_obj_set_width(weather_label, screenWidth - 40);
      lv_obj_align(weather_label, LV_ALIGN_TOP_MID, 0, 50);
      
      // 不添加背景和边框效果
      lv_obj_set_style_bg_opa(weather_label, 0, 0); // 完全透明背景
      lv_obj_set_style_border_width(weather_label, 0, 0); // 无边框
      lv_obj_set_style_radius(weather_label, 0, 0); // 无圆角
      lv_obj_set_style_pad_all(weather_label, 10, 0);
    }
    
    // 显示缓存的天气数据
    if (weatherDisplay.isInitialized()) {
      showWeatherDisplay();
      Serial.println("非更新时间，显示缓存天气数据");
    } else if (cachedWeatherData.length() > 0) {
      // 如果weatherDisplay未初始化但有缓存的原始数据，尝试使用它
      lv_label_set_text(weather_label, "使用缓存数据");
      lv_obj_clear_flag(weather_label, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(weather_label);
      Serial.println("非更新时间，显示缓存文本数据");
    } else {
      // 如果没有任何缓存数据，显示默认信息
      lv_label_set_text(weather_label, "加载天气数据中...");
      lv_obj_clear_flag(weather_label, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(weather_label);
      Serial.println("非更新时间，显示加载提示");
    }
  }
}

// 获取新闻
void getNews() {
  // 检查是否需要从网络获取数据（首次启动或整点更新时）
  if (isFirstStartup || forceRefreshNews) {
    HTTPClient httpClient;
    
    // 检查并创建news_label
    if (news_label == NULL || !lv_obj_is_valid(news_label)) {
      if (news_label != NULL) {
        // 确保旧标签被正确清理
        lv_obj_del(news_label);
      }
      news_label = lv_label_create(lv_scr_act());
      lv_label_set_long_mode(news_label, LV_LABEL_LONG_WRAP);
      lv_obj_set_size(news_label, screenWidth - 20, screenHeight - 150); // 调整大小，确保不超出屏幕
      lv_obj_align(news_label, LV_ALIGN_TOP_MID, 0, 120); // 上对齐，离顶部120px
      lv_obj_set_style_text_font(news_label, &lvgl_font_song_16, 0);
      lv_obj_set_style_text_color(news_label, lv_color_hex(0x333333), 0); // 深灰色文字
      
      // 添加背景和边框效果
      lv_obj_set_style_bg_color(news_label, lv_color_hex(0xFFFFFF), 0); // 白色背景
      lv_obj_set_style_bg_opa(news_label, 70, 0); // 半透明背景
      lv_obj_set_style_border_width(news_label, 2, 0); // 边框宽度
      lv_obj_set_style_border_color(news_label, lv_color_hex(0xCCCCCC), 0); // 灰色边框
      lv_obj_set_style_radius(news_label, 10, 0); // 圆角
      lv_obj_set_style_pad_all(news_label, 10, 0); // 内边距
    }
    
    // 检查WiFi连接状态
    if (WiFi.status() != WL_CONNECTED) {
      // 如果有缓存数据，则使用缓存数据
      if (cachedNewsData.length() > 0) {
        lv_label_set_text(news_label, cachedNewsData.c_str());
        Serial.println("WiFi未连接，使用缓存新闻数据");
      } else {
        lv_label_set_text(news_label, "WiFi未连接，无法获取新闻");
        Serial.println("WiFi未连接，无法获取新闻");
      }
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
      DynamicJsonDocument newsData(4096);
      DeserializationError error = deserializeJson(newsData, newsStr);
      
      if (error) {
        Serial.print(F("反序列化JSON失败: "));
        Serial.println(error.f_str());
        // 如果有缓存数据，则使用缓存数据
        if (cachedNewsData.length() > 0) {
          lv_label_set_text(news_label, cachedNewsData.c_str());
          Serial.println("解析失败，使用缓存新闻数据");
        } else {
          lv_label_set_text(news_label, "解析新闻数据失败");
        }
        httpClient.end();
        forceRefreshNews = false;
        return;
      }
      
      // 检查是否有新闻数据
      if (newsData.containsKey("result") && newsData["result"]["list"].size() > 0) {
        String newsContent = "最新新闻：\n";
        
        // 获取前3条新闻
        for (int i = 0; i < min(10, static_cast<int>(newsData["result"]["list"].size())); i++) {
          if (newsData["result"]["list"][i]["title"].is<const char*>()) {
            const char* title = newsData["result"]["list"][i]["title"].as<const char*>();
            newsContent += String(i + 1) + "、" + String(title) + "\n";
          }
        }
        
        lv_label_set_text(news_label, newsContent.c_str());
        
        // 保存到缓存
        cachedNewsData = newsContent;
      } else {
        // 如果有缓存数据，则使用缓存数据
        if (cachedNewsData.length() > 0) {
          lv_label_set_text(news_label, cachedNewsData.c_str());
          Serial.println("暂无新闻数据，使用缓存新闻数据");
        } else {
          lv_label_set_text(news_label, "暂无新闻数据");
        }
      }
      
      // 更新新闻数据的最后更新时间戳
      lastNewsUpdateTime = millis();
    } else {
      Serial.printf("HTTP请求失败，错误码: %d\n", httpCode);
      // 如果有缓存数据，则使用缓存数据
      if (cachedNewsData.length() > 0) {
        lv_label_set_text(news_label, cachedNewsData.c_str());
        Serial.println("获取失败，使用缓存新闻数据");
      } else {
        lv_label_set_text(news_label, "获取新闻失败");
      }
    }
    
    httpClient.end();
    forceRefreshNews = false;
  } else if (news_label != NULL && cachedNewsData.length() > 0) {
    // 非更新时间，直接显示缓存数据
    lv_label_set_text(news_label, cachedNewsData.c_str());
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

// 获取太空宇航员信息
void getAstronautsInfo() {
  if (!astronauts_label) {
    Serial.println("宇航员标签未初始化");
    return;
  }
  
  // 检查WiFi连接
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi未连接，无法获取宇航员信息");
    // 如果有缓存数据，则使用缓存数据
    if (cachedAstronautsData.length() > 0) {
      lv_label_set_text(astronauts_label, cachedAstronautsData.c_str());
      Serial.println("使用缓存的宇航员数据");
    } else {
      lv_label_set_text(astronauts_label, "WiFi未连接，无法获取宇航员信息");
    }
    return;
  }
  
  // 检查是否需要更新数据（24小时更新一次）
  const unsigned long ASTRONAUTS_UPDATE_INTERVAL = 24 * 60 * 60 * 1000; // 24小时
  bool shouldUpdate = (millis() - lastAstronautsUpdateTime >= ASTRONAUTS_UPDATE_INTERVAL) || lastAstronautsUpdateTime == 0;
  
  if (!shouldUpdate && cachedAstronautsData.length() > 0) {
    // 非更新时间，直接显示缓存数据
    lv_label_set_text(astronauts_label, cachedAstronautsData.c_str());
    return;
  }
  
  Serial.println("正在获取宇航员信息...");
  
  // 创建HTTP客户端
  HTTPClient httpClient;
  
  // 连接到API
  httpClient.begin("http://api.open-notify.org/astros.json");
  
  // 发送GET请求
  int httpCode = httpClient.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    // 获取响应体
    String response = httpClient.getString();
    
    // 解析JSON数据
    JsonDocument jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, response);
    
    if (error) {
      Serial.print(F("解析宇航员JSON数据失败: "));
      Serial.println(error.f_str());
      // 如果有缓存数据，则使用缓存数据
      if (cachedAstronautsData.length() > 0) {
        lv_label_set_text(astronauts_label, cachedAstronautsData.c_str());
        Serial.println("使用缓存的宇航员数据");
      } else {
        lv_label_set_text(astronauts_label, "解析宇航员数据失败");
      }
      httpClient.end();
      return;
    }
    
    // 提取宇航员信息
    int peopleCount = jsonDoc["number"];
    String astronautsInfo = "太空现有人数: " + String(peopleCount) + "人\n";
    
    // 简单方式按航天器分类显示宇航员
    JsonArray people = jsonDoc["people"];
    
    // 先收集所有航天器名称
    std::vector<String> crafts;
    for (JsonObject person : people) {
      String craft = person["craft"].as<const char*>();
      if (std::find(crafts.begin(), crafts.end(), craft) == crafts.end()) {
        crafts.push_back(craft);
      }
    }
    
    // 按航天器输出宇航员信息
    for (const String& craft : crafts) {
      astronautsInfo += craft + ":\n";
      for (JsonObject person : people) {
        if (String(person["craft"].as<const char*>()) == craft) {
          astronautsInfo += "- " + String(person["name"].as<const char*>()) + "\n";
        }
      }
      astronautsInfo += "\n";
    }
    
    // 更新标签显示
    lv_label_set_text(astronauts_label, astronautsInfo.c_str());
    
    // 保存到缓存
    cachedAstronautsData = astronautsInfo;
    
    // 更新最后更新时间
    lastAstronautsUpdateTime = millis();
    
    Serial.println("成功获取宇航员信息");
  } else {
    Serial.printf("获取宇航员信息失败，错误码: %d\n", httpCode);
    // 如果有缓存数据，则使用缓存数据
    if (cachedAstronautsData.length() > 0) {
      lv_label_set_text(astronauts_label, cachedAstronautsData.c_str());
      Serial.println("使用缓存的宇航员数据");
    } else {
      lv_label_set_text(astronauts_label, "获取宇航员信息失败");
    }
  }
  
  httpClient.end();
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
  // 创建APRS标签（如果尚未创建）
  if (!aprs_label) {
    aprs_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(aprs_label, &lvgl_font_song_16, 0); // 16px字体
    lv_obj_set_style_text_color(aprs_label, lv_color_hex(0x000000), 0); // 黑色文字
    lv_obj_set_width(aprs_label, screenWidth - 20); // 宽度为屏幕宽度减20px
    lv_obj_align(aprs_label, LV_ALIGN_TOP_LEFT, 10, 70); // 左上角对齐，边距10,70
    lv_label_set_text(aprs_label, "正在连接APRS服务器...");
    lv_label_set_long_mode(aprs_label, LV_LABEL_LONG_WRAP); // 自动换行
  }
  
  // 确保APRS标签可见
  lv_obj_clear_flag(aprs_label, LV_OBJ_FLAG_HIDDEN);
  
  // 创建APRS表格（如果尚未创建）
  if (!aprs_table) {
    aprs_table = lv_table_create(lv_scr_act());
    lv_table_set_col_cnt(aprs_table, 2); // 2列：呼号、信息
    lv_table_set_row_cnt(aprs_table, APRS_MAX_PACKETS + 1); // +1 用于表头
    
    // 设置表格样式
    lv_obj_set_style_text_font(aprs_table, &lvgl_font_song_16, 0);
    lv_obj_set_style_text_color(aprs_table, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_color(aprs_table, lv_color_hex(0xFFFFFF), 0);
    
    // 设置表格宽度为屏幕宽度
    lv_obj_set_width(aprs_table, screenWidth - 20);
    lv_obj_set_height(aprs_table, 300); // 设置表格高度
    
    // 设置列宽，适应屏幕宽度
    lv_table_set_col_width(aprs_table, 0, (screenWidth - 30) * 0.30); // 呼号列
    lv_table_set_col_width(aprs_table, 1, (screenWidth - 30) * 0.70); // 信息列
    
    // 设置表格位置（在标签下方）
    lv_obj_align(aprs_table, LV_ALIGN_TOP_MID, 0, 100);
  }
  
  // 确保APRS表格可见
  lv_obj_clear_flag(aprs_table, LV_OBJ_FLAG_HIDDEN);
  
  // 检查WiFi连接状态
  if (WiFi.status() != WL_CONNECTED) {
    // 如果有缓存数据，则显示缓存数据
    if (!aprsPacketCache.empty()) {
      showCachedAPRSData();
      Serial.println("WiFi未连接，显示缓存的APRS数据");
    } else {
      lv_label_set_text(aprs_label, "WiFi未连接，无法获取APRS数据");
      Serial.println("WiFi未连接，无法获取APRS数据");
    }
    return;
  }
  
  // 连接到APRS服务器
  WiFiClient aprsClient;
  Serial.print("尝试连接APRS服务器：");
  Serial.print(APRS_SERVER);
  Serial.print(":");
  Serial.println(APRS_PORT);
  
  if (!aprsClient.connect(APRS_SERVER, APRS_PORT)) {
    // 如果有缓存数据，则显示缓存数据
    if (!aprsPacketCache.empty()) {
      showCachedAPRSData();
      Serial.println("无法连接到APRS服务器，显示缓存数据");
    } else {
      lv_label_set_text(aprs_label, "无法连接到APRS服务器");
      Serial.println("无法连接到APRS服务器");
    }
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
  
  lv_label_set_text(aprs_label, "正在接收APRS数据...");
  
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
        
        // 只显示信息部分的前50个字符
        if (info.length() > 50) {
          info = info.substring(0, 50) + "...";
        }
        
        // 缓存新的数据包
        cacheAPRSPacket(callSign, info);
        
        newPacketCount++;
      }
    }
    
    // 处理LVGL事件
    lv_timer_handler();
  }
  
  // 断开连接
  aprsClient.stop();
  
  // 显示缓存的数据包（包括新收到的和之前缓存的）
  showCachedAPRSData();
  
  Serial.print("APRS数据接收完成，新增：");
  Serial.print(newPacketCount);
  Serial.println("条数据包");
}