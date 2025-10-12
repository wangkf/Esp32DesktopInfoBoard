#include "Arduino.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <lvgl.h>
#include <map>
#include <vector>
#include <string>
#include "weather_display.h"

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
  if (!readJsonFromFile("/weather_data.json", doc)) {
    if (weather_label) {
      lv_label_set_text(weather_label, "无法读取天气数据文件");
    }
    return;
  }

  // 构建天气显示文本
  String weatherText = "城市: " + String(doc["city"].as<const char*>()) + "\n";
  weatherText += "温度: " + String(doc["current_temp"].as<const char*>()) + "°C\n";
  weatherText += "湿度: " + String(doc["humidity"].as<const char*>()) + "%\n";
  weatherText += "天气: " + String(doc["weather_desc"].as<const char*>()) + "\n";
  weatherText += "风力: " + String(doc["wind"].as<const char*>()) + "\n";
  weatherText += "限行: " + String(doc["limit_info"].as<const char*>()) + "\n";
  weatherText += "建议: " + String(doc["suggestion"].as<const char*>()) + "\n";
  
  // 更新天气标签
  if (weather_label) {
    lv_label_set_text(weather_label, weatherText.c_str());
  }

  // 初始化天气显示组件
  initWeatherDisplay();
  
  // 更新天气预报图标
  JsonArray forecast = doc["forecast"].as<JsonArray>();
  size_t forecastCount = min(static_cast<size_t>(4), forecast.size());
  for (int i = 0; i < forecastCount; i++) {
    JsonObject dailyWeather = forecast[i].as<JsonObject>();
    updateWeatherIcon(dailyWeather, i);
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
  if (!readJsonFromFile("/news_data.json", doc)) {
    if (news_label) {
      lv_label_set_text(news_label, "无法读取新闻数据文件");
    }
    return;
  }

  // 构建新闻显示文本
  String newsText = "最新资讯\n\n";
  JsonArray newsList = doc["news"].as<JsonArray>();
  
  for (int i = 0; i < newsList.size(); i++) {
    newsText += String(i + 1) + ". " + String(newsList[i].as<const char*>()) + "\n\n";
  }
  
  // 更新新闻标签
  if (news_label) {
    lv_label_set_text(news_label, newsText.c_str());
  }
}

/**
 * 显示金山词霸每日信息
 */
void displayIcibaDataFromFile() {
  Serial.println("从文件显示金山词霸数据");
  
  JsonDocument doc;
  if (!readJsonFromFile("/iciba_info.json", doc)) {
    if (iciba_label) {
      lv_label_set_text(iciba_label, "无法读取金山词霸数据文件");
    }
    return;
  }

  // 构建金山词霸显示文本
  String icibaText = "今日格言：\n";
  icibaText += doc["content"].as<const char*>();
  icibaText += "\n\n";
  icibaText += "翻译：";
  icibaText += doc["note"].as<const char*>();
  
  // 更新金山词霸标签
  if (iciba_label) {
    lv_label_set_text(iciba_label, icibaText.c_str());
  }
}

/**
 * 显示宇航员信息
 */
void displayAstronautsDataFromFile() {
  Serial.println("从文件显示宇航员数据");
  
  JsonDocument doc;
  if (!readJsonFromFile("/astronauts_data.json", doc)) {
    if (astronauts_label) {
      lv_label_set_text(astronauts_label, "无法读取宇航员数据文件");
    }
    return;
  }

  // 构建宇航员显示文本
  String astronautsText = "太空中的宇航员总数: " + String(doc["total"].as<int>()) + "\n\n";
  
  JsonArray people = doc["people"].as<JsonArray>();
  
  // 按航天器分组显示
  std::map<String, std::vector<String>> astronautsByCraft;
  
  for (JsonVariant person : people) {
    String name = person["name"].as<String>();
    String craft = person["craft"].as<String>();
    astronautsByCraft[craft].push_back(name);
  }
  
  for (auto& pair : astronautsByCraft) {
    astronautsText += pair.first + ": " + String(pair.second.size()) + "人\n";
    for (String name : pair.second) {
      astronautsText += "- " + name + "\n";
    }
    astronautsText += "\n";
  }
  
  // 更新宇航员标签
  if (astronauts_label) {
    lv_label_set_text(astronauts_label, astronautsText.c_str());
  }
}

/**
 * 显示APRS数据
 */
void displayAPRSDataFromFile() {
  Serial.println("从文件显示APRS数据");
  
  JsonDocument doc;
  if (!readJsonFromFile("/aprs_data.json", doc)) {
    if (aprs_label) {
      lv_label_set_text(aprs_label, "无法读取APRS数据文件");
    }
    return;
  }

  // 更新APRS标签
  String aprsStatus = "APRS数据 (" + String(doc["total_packets"].as<int>()) + " 条)\n";
  aprsStatus += "最后更新: " + String(doc["update_time"].as<const char*>());
  
  if (aprs_label) {
    lv_label_set_text(aprs_label, aprsStatus.c_str());
  }

  // 更新APRS表格
  if (aprs_table) {
    // 设置表头
    lv_table_set_cell_value(aprs_table, 0, 0, "呼号");
    lv_table_set_cell_value(aprs_table, 0, 1, "信息");
    
    // 设置表头样式（使用LVGL 8.3支持的方式）
    // 简化实现，暂时不设置复杂的样式，确保编译通过
    // 在实际应用中，可以使用lv_table_set_cell_ctrl函数设置单元格属性
    
    // 填充表格数据
    JsonArray packets = doc["packets"].as<JsonArray>();
    int row = 1;
    
    for (JsonVariant packet : packets) {
      if (row >= lv_table_get_row_cnt(aprs_table)) break;
      
      lv_table_set_cell_value(aprs_table, row, 0, packet["callsign"].as<const char*>());
      lv_table_set_cell_value(aprs_table, row, 1, packet["info"].as<const char*>());
      
      row++;
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