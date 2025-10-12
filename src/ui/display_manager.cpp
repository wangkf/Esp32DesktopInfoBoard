#include "Arduino.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <lvgl.h>
#include <map>
#include <vector>
#include <string>
#include "weather_display.h"
#include "image_downloader.h"

// 外部变量声明
extern lv_obj_t* weather_label;
extern lv_obj_t* news_label;
extern lv_obj_t* iciba_label;
extern lv_obj_t* astronauts_label;
extern lv_obj_t* aprs_label;
extern lv_obj_t* aprs_table;

// 全局变量
extern const uint32_t screenWidth;
extern const uint32_t screenHeight;

/**
 * 从文件读取JSON数据
 * @param fileName 文件路径
 * @param doc JsonDocument对象，用于存储解析结果
 * @return 是否成功读取和解析
 */
bool readJsonFromFile(const char* fileName, JsonDocument& doc) {
  // 检查文件是否存在
  if (!SPIFFS.exists(fileName)) {
    Serial.printf("文件不存在: %s\n", fileName);
    return false;
  }

  // 打开文件
  File file = SPIFFS.open(fileName, "r");
  if (!file) {
    Serial.printf("打开文件失败: %s\n", fileName);
    return false;
  }

  // 解析JSON
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.printf("解析JSON失败: %s, 错误: %s\n", fileName, error.f_str());
    return false;
  }

  return true;
}

/**
 * 显示天气数据
 */
void displayWeatherDataFromFile() {
  Serial.println("从文件显示天气数据");
  
  JsonDocument doc;
  if (!readJsonFromFile("/weather.json", doc)) {
    if (weather_label && lv_obj_is_valid(weather_label)) {
      lv_label_set_text(weather_label, "无法读取天气数据文件");
    }
    return;
  }

  // 检查是否包含result对象
  if (!doc.containsKey("result") || !doc["result"].is<JsonObject>()) {
    if (weather_label && lv_obj_is_valid(weather_label)) {
      lv_label_set_text(weather_label, "天气数据格式不正确");
    }
    return;
  }

  JsonObject result = doc["result"].as<JsonObject>();

  // 获取更新时间
  String updateTime = "";
  if (result.containsKey("last_updated")) {
    updateTime = result["last_updated"].as<const char*>();
  }
  
  // 构建天气显示文本，在第一行右边括号中显示更新时间
  String weatherText = "城市: " + (result.containsKey("city") ? String(result["city"].as<const char*>()) : "未知");
  if (!updateTime.isEmpty()) {
    weatherText += " (" + updateTime + ")";
  }
  weatherText += "\n";
  
  // 检查realtime对象是否存在
  if (result.containsKey("realtime") && result["realtime"].is<JsonObject>()) {
    JsonObject realtime = result["realtime"].as<JsonObject>();
    
    weatherText += "温度: " + (realtime.containsKey("temperature") ? String(realtime["temperature"].as<const char*>()) : "未知") + "°C\n";
    weatherText += "湿度: " + (realtime.containsKey("humidity") ? String(realtime["humidity"].as<const char*>()) : "未知") + "\n";
    weatherText += "天气: " + (realtime.containsKey("info") ? String(realtime["info"].as<const char*>()) : "未知") + "\n";
    
    String windInfo = "风力: ";
    if (realtime.containsKey("direct")) windInfo += String(realtime["direct"].as<const char*>());
    if (realtime.containsKey("power")) windInfo += " " + String(realtime["power"].as<const char*>());
    if (realtime.containsKey("direct")) weatherText += windInfo + "\n";
    
    weatherText += "空气质量: " + (realtime.containsKey("aqi") ? String(realtime["aqi"].as<const char*>()) : "未知");
    if (realtime.containsKey("pm25")) {
      weatherText += " (PM2.5: " + String(realtime["pm25"].as<const char*>()) + ")";
    }
    weatherText += "\n";
  }
  
  // 更新天气标签
  if (weather_label && lv_obj_is_valid(weather_label)) {
    lv_label_set_text(weather_label, weatherText.c_str());
  }

  // 初始化天气显示组件
  initWeatherDisplay();
  
  // 更新天气预报图标
  if (result.containsKey("future") && result["future"].is<JsonArray>()) {
    JsonArray forecast = result["future"].as<JsonArray>();
    size_t forecastCount = min(static_cast<size_t>(4), forecast.size());
    for (int i = 0; i < forecastCount; i++) {
      if (forecast[i].is<JsonObject>()) {
        JsonObject dailyWeather = forecast[i].as<JsonObject>();
        updateWeatherIcon(dailyWeather, i);
      }
    }
  }
  
  // 显示天气图标和图表
  showWeatherDisplay();
}

/**
 * 显示新闻数据
 */
void displayNewsDataFromFile() {
  Serial.println("从文件显示新闻数据");
  
  JsonDocument doc;
  if (!readJsonFromFile("/news.json", doc)) {
    if (news_label && lv_obj_is_valid(news_label)) {
      lv_label_set_text(news_label, "无法读取新闻数据文件");
    }
    return;
  }

  // 检查是否包含result对象
  if (!doc.containsKey("result") || !doc["result"].is<JsonObject>()) {
    if (news_label && lv_obj_is_valid(news_label)) {
      lv_label_set_text(news_label, "新闻数据格式不正确");
    }
    return;
  }
  
  JsonObject result = doc["result"].as<JsonObject>();
  
  // 获取更新时间
  String updateTime = "";
  if (result.containsKey("last_updated")) {
    updateTime = result["last_updated"].as<const char*>();
  }
  
  // 构建新闻显示文本，在第一行右边括号中显示更新时间
  String newsText = "最新资讯";
  if (!updateTime.isEmpty()) {
    newsText += " (" + updateTime + ")";
  }
  newsText += "\n";
  
  // 检查是否存在list数组
  if (result.containsKey("list") && result["list"].is<JsonArray>()) {
    JsonArray newsList = result["list"].as<JsonArray>();
    
    for (int i = 0; i < newsList.size(); i++) {
        if (newsList[i].containsKey("title") && newsList[i]["title"].is<const char*>()) {
          newsText += String(i + 1) + ". " + String(newsList[i]["title"].as<const char*>()) + "\n";
        }
      }
  }
  
  // 更新新闻标签
  if (news_label && lv_obj_is_valid(news_label)) {
    lv_label_set_text(news_label, newsText.c_str());
  }
}

/**
 * 显示金山词霸每日信息
 */
void displayIcibaDataFromFile() {
  Serial.println("从文件显示金山词霸数据");
  
  JsonDocument doc;
  if (!readJsonFromFile("/iciba.json", doc)) {
    if (iciba_label && lv_obj_is_valid(iciba_label)) {
      lv_label_set_text(iciba_label, "无法读取金山词霸数据文件");
    }
    return;
  }

  // 获取更新时间
  String updateTime = "";
  
  // 检查result对象中的last_updated字段
  if (doc.containsKey("result") && doc["result"].is<JsonObject>()) {
    JsonObject result = doc["result"].as<JsonObject>();
    if (result.containsKey("last_updated")) {
      updateTime = result["last_updated"].as<const char*>();
    }
  }
  
  // 如果result中没有，检查顶层update_time字段（支持时间戳格式）
  if (updateTime.isEmpty() && doc.containsKey("update_time")) {
    if (doc["update_time"].is<unsigned long>()) {
      // 将时间戳转换为可读格式
      unsigned long timestamp = doc["update_time"].as<unsigned long>();
      time_t now = timestamp / 1000; // 转换为秒
      struct tm *timeinfo = localtime(&now);
      if (timeinfo != nullptr) {
        char timeString[20];
        strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", timeinfo);
        updateTime = String(timeString);
      }
    } else if (doc["update_time"].is<const char*>()) {
      updateTime = doc["update_time"].as<const char*>();
    }
  }
  
  // 构建金山词霸显示文本，在第一行右边括号中显示更新时间
  String icibaText = "今日格言：";
  if (!updateTime.isEmpty()) {
    icibaText += " (" + updateTime + ")";
  }
  icibaText += "\n";
  
  // 保持访问顶级字段
  if (doc.containsKey("content") && doc["content"].is<const char*>()) {
    icibaText += doc["content"].as<const char*>();
  } else {
    icibaText += "暂无格言内容";
  }
  
  icibaText += "\n\n";
  icibaText += "翻译：";
  
  if (doc.containsKey("note") && doc["note"].is<const char*>()) {
    icibaText += doc["note"].as<const char*>();
  } else {
    icibaText += "暂无翻译";
  }
  
  // 如果有tts字段，可以显示提示
  if (doc.containsKey("tts")) {
    icibaText += "\n\n[有发音]";
  }
  
  // 更新金山词霸标签
  if (iciba_label && lv_obj_is_valid(iciba_label)) {
    lv_label_set_text(iciba_label, icibaText.c_str());
  }
}

/**
 * 显示宇航员信息
 */
void displayAstronautsDataFromFile() {
  Serial.println("从文件显示宇航员数据");
  
  // 确保astronauts_label已创建
  if (astronauts_label == NULL || !lv_obj_is_valid(astronauts_label)) {
    if (astronauts_label != NULL) {
      // 确保旧标签被正确清理
      lv_obj_del(astronauts_label);
    }
    astronauts_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(astronauts_label, &lvgl_font_song_16, 0);
    lv_obj_set_style_text_color(astronauts_label, lv_color_hex(0x000000), 0); // 默认黑色
    lv_obj_set_style_text_align(astronauts_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(astronauts_label, LV_LABEL_LONG_WRAP); // 设置自动换行
    lv_obj_set_width(astronauts_label, screenWidth - 40); // 设置标签宽度
    lv_obj_set_height(astronauts_label, screenHeight - 120); // 固定高度
    lv_obj_align(astronauts_label, LV_ALIGN_TOP_MID, 0, 100); // 顶部居中对齐，顶部离屏幕顶部100px
    
    // 不添加背景和边框效果
    lv_obj_set_style_bg_opa(astronauts_label, 0, 0); // 完全透明背景
    lv_obj_set_style_border_width(astronauts_label, 0, 0); // 无边框
    lv_obj_set_style_radius(astronauts_label, 0, 0); // 无圆角
    lv_obj_set_style_pad_all(astronauts_label, 10, 0); // 内边距
    
    Serial.println("创建或重新创建了astronauts_label");
  }
  
  JsonDocument doc;
  if (!readJsonFromFile("/astronauts.json", doc)) {
    if (astronauts_label && lv_obj_is_valid(astronauts_label)) {
      lv_label_set_text(astronauts_label, "无法读取宇航员数据文件");
    }
    return;
  }

  // 检查是否包含result对象
  if (!doc.containsKey("result") || !doc["result"].is<JsonObject>()) {
    if (astronauts_label && lv_obj_is_valid(astronauts_label)) {
      lv_label_set_text(astronauts_label, "宇航员数据格式不正确");
    }
    return;
  }
  
  JsonObject result = doc["result"].as<JsonObject>();
  
  // 获取更新时间
  String updateTime = "";
  if (result.containsKey("last_updated")) {
    updateTime = result["last_updated"].as<const char*>();
  }
  
  // 构建宇航员显示文本，在第一行右边括号中显示更新时间
  String astronautsText = "太空宇航员总数:" + String(result["total"].as<int>());
  if (!updateTime.isEmpty()) {
    astronautsText += " (" + updateTime + ")";
  }
  astronautsText += "\n";
  
  // 按航天器分组显示
  std::map<String, std::vector<String>> astronautsByCraft;
  
  if (result.containsKey("astronauts") && result["astronauts"].is<JsonArray>()) {
    JsonArray astronauts = result["astronauts"].as<JsonArray>();
    
    for (JsonVariant astronaut : astronauts) {
      if (astronaut.containsKey("name") && astronaut.containsKey("craft")) {
        String name = astronaut["name"].as<String>();
        String craft = astronaut["craft"].as<String>();
        astronautsByCraft[craft].push_back(name);
      }
    }
  }
  
  // 按照用户要求的格式显示：按航天器分组，每组内一行一个姓名
  for (auto& pair : astronautsByCraft) {
    astronautsText += pair.first + ":\n";
    
    // 每个宇航员姓名一行显示
    for (size_t i = 0; i < pair.second.size(); i++) {
      astronautsText += "- " + pair.second[i] + "\n";
    }
    astronautsText += "\n";
  }
  
  // 检查是否存在code字段（表示API返回状态）
  if (doc.containsKey("code") && doc.containsKey("msg")) {
    astronautsText += "\n状态: " + String(doc["msg"].as<const char*>());
  }
  
  // 更新宇航员标签
  if (astronauts_label && lv_obj_is_valid(astronauts_label)) {
    lv_label_set_text(astronauts_label, astronautsText.c_str());
  }
}

/**
 * 显示APRS数据
 */
void displayAPRSDataFromFile() {
  Serial.println("从文件显示APRS数据");
  
  JsonDocument doc;
  if (!readJsonFromFile("/aprs.json", doc)) {
    if (aprs_label && lv_obj_is_valid(aprs_label)) {
      lv_label_set_text(aprs_label, "无法读取APRS数据文件");
    }
    return;
  }

  // 检查是否包含result对象
  if (!doc.containsKey("result") || !doc["result"].is<JsonObject>()) {
    if (aprs_label && lv_obj_is_valid(aprs_label)) {
      lv_label_set_text(aprs_label, "APRS数据格式不正确");
    }
    return;
  }
  
  JsonObject result = doc["result"].as<JsonObject>();

  // 获取更新时间
  String updateTime = "";
  if (result.containsKey("update_time")) {
    if (result["update_time"].is<unsigned long>()) {
      // 将时间戳转换为可读格式
      unsigned long timestamp = result["update_time"].as<unsigned long>();
      time_t now = timestamp / 1000; // 转换为秒
      struct tm *timeinfo = localtime(&now);
      if (timeinfo != nullptr) {
        char timeString[20];
        strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", timeinfo);
        updateTime = String(timeString);
      }
    } else if (result["update_time"].is<const char*>()) {
      updateTime = String(result["update_time"].as<const char*>());
    }
  }
  
  // 更新APRS标签，在第一行右边括号中显示更新时间
  String aprsStatus = "APRS数据 (";
  if (result.containsKey("total_packets") && result["total_packets"].is<int>()) {
    aprsStatus += String(result["total_packets"].as<int>());
  } else {
    aprsStatus += "0";
  }
  
  // 在括号内添加更新时间
  if (!updateTime.isEmpty()) {
    aprsStatus += ", 更新: " + updateTime;
  }
  aprsStatus += ")\n";
  
  if (aprs_label && lv_obj_is_valid(aprs_label)) {
    lv_label_set_text(aprs_label, aprsStatus.c_str());
  }

  // 更新APRS表格
  if (aprs_table && lv_obj_is_valid(aprs_table)) {
    // 设置表头
    lv_table_set_cell_value(aprs_table, 0, 0, "呼号");
    lv_table_set_cell_value(aprs_table, 0, 1, "数据");
    
    // 设置表头样式（使用LVGL 8.3支持的方式）
    // 简化实现，暂时不设置复杂的样式，确保编译通过
    // 在实际应用中，可以使用lv_table_set_cell_ctrl函数设置单元格属性
    
    // 填充表格数据
    if (result.containsKey("packets") && result["packets"].is<JsonArray>()) {
      JsonArray packets = result["packets"].as<JsonArray>();
      int row = 1;
      
      for (JsonVariant packet : packets) {
        if (row >= lv_table_get_row_cnt(aprs_table)) break;
        
        if (packet.containsKey("callsign") && packet.containsKey("raw_data")) {
          lv_table_set_cell_value(aprs_table, row, 0, packet["callsign"].as<const char*>());
          lv_table_set_cell_value(aprs_table, row, 1, packet["raw_data"].as<const char*>());
        }
        
        row++;
      }
    }
  }
}

/**
 * 初始化显示管理器
 */
void initDisplayManager() {
  Serial.println("初始化显示管理器");
  
  // 确保SPIFFS已初始化
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS初始化失败");
  }
}

/**
 * 测试从URL显示图片的功能
 * @param url 图片的URL地址
 */
void testDisplayImageFromUrl(const char* url) {
  Serial.print("测试从URL显示图片: ");
  Serial.println(url);
  
  // 清除屏幕
  lv_obj_clean(lv_scr_act());
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xFFFFFF), 0);
  
  // 创建状态标签
  lv_obj_t* status_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(status_label, &lvgl_font_song_16, 0);
  lv_obj_set_style_text_color(status_label, lv_color_hex(0x000000), 0);
  lv_label_set_text(status_label, "正在下载图片...");
  lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 10);
  
  // 刷新显示以显示状态信息
  lv_refr_now(lv_disp_get_default());
  lv_task_handler();
  
  // 显示图片（居中显示）
  bool success = displayImageFromUrl(url, (screenWidth - 240) / 2, (screenHeight - 240) / 2);
  
  // 更新状态信息
  if (success) {
    lv_label_set_text(status_label, "图片显示成功");
  } else {
    lv_label_set_text(status_label, "图片显示失败");
  }
  
  // 刷新显示
  lv_refr_now(lv_disp_get_default());
  lv_task_handler();
  
  Serial.println(success ? "图片显示测试完成" : "图片显示测试失败");
}