#include "config/config.h"
#include "includes.h"
#include <map>
#include <vector>
#include <string>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <lvgl.h>
#include <time.h>
#include "config/config_manager.h"
#include "ui_utils.h"
// 外部变量声明
extern lv_obj_t* iciba_label;
extern lv_obj_t* astronauts_label;
extern lv_obj_t* news_label;
extern lv_obj_t* calendar_label;
// 全局变量
extern const uint32_t screenWidth;
extern const uint32_t screenHeight;
//*** 显示日历信息
void displayCalendar() {
  Serial.println("显示日历信息");
  // 检查calendar_label是否已创建和有效
  if (!calendar_label || !lv_obj_is_valid(calendar_label)) {
    Serial.println("calendar_label无效，无法显示日历");
    return;
  }
  
  // 获取当前时间
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  // 获取当前年份、月份和日期
  int year = timeinfo.tm_year + 1900;
  int month = timeinfo.tm_mon + 1;
  int day = timeinfo.tm_mday;
  
  // 计算当月第一天是星期几
  struct tm firstDayOfMonth = timeinfo;
  firstDayOfMonth.tm_mday = 1;
  mktime(&firstDayOfMonth);
  int firstDayWeekday = firstDayOfMonth.tm_wday;
  
  // 计算当月有多少天
  struct tm lastDayOfMonth = timeinfo;
  lastDayOfMonth.tm_mday = 1;
  lastDayOfMonth.tm_mon += 1;
  mktime(&lastDayOfMonth);
  lastDayOfMonth.tm_mday = 0; // 设置为0，回退到上个月的最后一天
  mktime(&lastDayOfMonth);
  int daysInMonth = lastDayOfMonth.tm_mday;
  
  // 构建日历文本
  String calendarText = "";
  // 添加月份标题
  calendarText += String(year) + "年" + String(month) + "月日历\n\n";
  // 添加星期标题
  calendarText += "日  一  二  三  四  五  六\n";
  
  // 添加日期，确保当前日期突出显示
  int dayCount = 1;
  // 填充第一行的空格
  for (int i = 0; i < firstDayWeekday; i++) {
    calendarText += "     "; // 五个空格
  }
  
  // 填充日期
  for (int i = firstDayWeekday; i < 7; i++) {
    if (dayCount == day) {
      // 突出显示当前日期
      calendarText += "【";
      if (dayCount < 10) {
        calendarText += "0";
      }
      calendarText += String(dayCount) + "】";
    } else {
      if (dayCount < 10) {
        calendarText += "0";
      }
      calendarText += String(dayCount) + "  ";
    }
    dayCount++;
  }
  calendarText += "\n";
  
  // 填充剩余的日期
  while (dayCount <= daysInMonth) {
    for (int i = 0; i < 7 && dayCount <= daysInMonth; i++) {
      if (dayCount == day) {
        // 突出显示当前日期
        calendarText += "【";
        if (dayCount < 10) {
          calendarText += "0";
        }
        calendarText += String(dayCount) + "】";
      } else {
        if (dayCount < 10) {
          calendarText += "0";
        }
        calendarText += String(dayCount) + "  ";
      }
      dayCount++;
    }
    calendarText += "\n";
  }
  
  // 更新标签文本
  lv_label_set_text(calendar_label, calendarText.c_str());
  
  // 强制显示日历标签
  lv_obj_clear_flag(calendar_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(calendar_label);
  
  // 尝试通过UIManager显示日历相关元素
  UIManager::getInstance()->showElements({"calendar_label", "calendar_img", "today_date_label"});
  
  // 设置today_date_label显示当前日期，使用两位数格式
  String dayStr;
  if (day < 10) {
    dayStr = "0" + String(day);
  } else {
    dayStr = String(day);
  }
  lv_obj_t* todayDateLabel = UIManager::getInstance()->getElement("today_date_label");
  if (todayDateLabel && lv_obj_is_valid(todayDateLabel)) {
    lv_label_set_text(todayDateLabel, dayStr.c_str());
  }
  
  Serial.println("日历信息显示完成");
}
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
//*** 显示金山词霸每日信息
void displayIcibaDataFromFile() {
  Serial.println("从文件显示金山词霸数据");
  
  // 确保iciba_label有效
  if (!iciba_label || !lv_obj_is_valid(iciba_label)) {
    Serial.println("iciba_label无效，无法显示每日一句");
    return;
  }
  
  JsonDocument doc;
  if (!readJsonFromFile("/iciba.json", doc)) {
    lv_label_set_text(iciba_label, "无法读取金山词霸数据文件");
  } else {

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
  String icibaText = "";
  // 保存图片URL变量
  String fenxiangImgUrl = "";

    // 保持向后兼容，访问顶级字段
    if (doc.containsKey("content") && doc["content"].is<const char*>()) {
      icibaText += doc["content"].as<const char*>();
    } else {
      icibaText += "暂无格言内容";
    }
    
    icibaText += "\n\n";
    if (doc.containsKey("note") && doc["note"].is<const char*>()) {
      icibaText += doc["note"].as<const char*>();
    } else {
      icibaText += "暂无翻译";
    }
 
  // 更新金山词霸标签
  if (iciba_label && lv_obj_is_valid(iciba_label)) {
    lv_label_set_text(iciba_label, icibaText.c_str());
    lv_obj_clear_flag(iciba_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    lv_obj_move_foreground(iciba_label); // 确保标签显示在最上层
  }
  
  // 尝试通过UIManager显示每日一句相关元素
  UIManager::getInstance()->showElements({"iciba_label", "iciba_img"});
  }
}
//*** 显示留言板内容
void displayNoteDataFromFile() {
  Serial.println("从文件显示留言板内容");
  
  // 外部声明note_label
  extern lv_obj_t* note_label;
  
  // 确保note_label已创建和初始化
  if (!note_label || !lv_obj_is_valid(note_label)) {
    Serial.println("note_label无效，无法显示留言板内容");
    return;
  }
  
  JsonDocument doc;
  if (!readJsonFromFile("/note.json", doc)) {
    if (note_label && lv_obj_is_valid(note_label)) {
      lv_label_set_text(note_label, "暂无留言内容");
      lv_obj_clear_flag(note_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    }
    return;
  }

  // 构建留言板显示文本
  String noteText = "";
  
  // 检查是否包含note字段
  if (doc.containsKey("note") && doc["note"].is<const char*>()) {
    String noteContent = doc["note"].as<const char*>();
    
    // 如果note内容为空，显示提示信息
    if (noteContent.isEmpty()) {
      noteText = "留言板\n\n暂无留言内容";
    } else {
      // 添加note内容，确保格式良好
      noteText += noteContent;
    }
  } else {
    noteText += "暂无留言内容";
  }
  // 更新留言板标签
  if (note_label && lv_obj_is_valid(note_label)) {
    lv_label_set_text(note_label, noteText.c_str());
    lv_obj_clear_flag(note_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    lv_obj_move_foreground(note_label); // 确保标签显示在最上层
  }
}
//*** 显示宇航员信息
void displayAstronautsDataFromFile() {
  Serial.println("从文件显示宇航员数据");
  
  // 确保astronauts_label有效
  if (!astronauts_label || !lv_obj_is_valid(astronauts_label)) {
    Serial.println("astronauts_label无效，无法显示宇航员信息");
    return;
  }
  
  JsonDocument doc;
  if (!readJsonFromFile("/astronauts.json", doc)) {
    Serial.println("无法读取宇航员数据文件");
    lv_label_set_text(astronauts_label, "无法读取宇航员数据文件");
  } else {
    // 先检查doc是否包含"people"
    if (!doc.containsKey("people")) {
      Serial.println("JSON格式错误：缺少people字段");
      lv_label_set_text(astronauts_label, "JSON格式错误：缺少people字段");
    } else {
      // 检查"people"是否为JsonArray
      if (doc["people"].is<JsonArray>()) {
        JsonArray peopleArray = doc["people"].as<JsonArray>();
        Serial.println("检测到宇航员数组格式数据");
        
        // 构建宇航员显示文本
        String astronautsText = "太空宇航员列表\n\n";
        
        // 遍历宇航员数组
        for (JsonVariant astronaut : peopleArray) {
          if (astronaut.containsKey("name") && astronaut.containsKey("craft")) {
            String name = astronaut["name"].as<String>();
            String craft = astronaut["craft"].as<String>();
            astronautsText += name + " - " + craft + "\n";
          }
        }  
        // 更新宇航员标签
        lv_label_set_text(astronauts_label, astronautsText.c_str());
      }
      // 如果不是数组，检查是否为对象
      else if (doc["people"].is<JsonObject>()) {
        JsonObject result = doc["people"].as<JsonObject>();
        Serial.println("检测到宇航员对象格式数据");
        
        // 获取更新时间
        String updateTime = "";
        if (result.containsKey("last_updated")) {
          updateTime = result["last_updated"].as<const char*>();
        }
        
        // 构建宇航员显示文本，在第一行右边括号中显示更新时间
        String astronautsText = "太空宇航员总数:" + String(result["number"].as<int>());
        if (!updateTime.isEmpty()) {
          astronautsText += " (" + updateTime + ")";
        }
        astronautsText += "\n";
        
        // 按航天器分组显示
        std::map<String, std::vector<String>> astronautsByCraft;
        
        if (result.containsKey("people") && result["people"].is<JsonArray>()) {
          JsonArray astronauts = result["people"].as<JsonArray>();
          
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

        // 更新宇航员标签
        lv_label_set_text(astronauts_label, astronautsText.c_str());
      }
      else {
        Serial.println("JSON格式错误：people字段格式不正确");
        lv_label_set_text(astronauts_label, "JSON格式错误：people字段格式不正确");
      }
    }
  }
  
  // 强制显示宇航员标签
  lv_obj_clear_flag(astronauts_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(astronauts_label);
  
  // 尝试通过UIManager显示宇航员相关元素
  UIManager::getInstance()->showElements({"astronauts_label", "astronauts_img"});
  
  Serial.println("宇航员数据显示完成");
}
//*** 显示新闻信息
void displayNewsDataFromFile() {
  Serial.println("从文件显示新闻数据");
    JsonDocument doc;
  if (!readJsonFromFile("/news.json", doc)) {
    if (news_label && lv_obj_is_valid(news_label)) {
      lv_label_set_text(news_label, "无法读取新闻数据文件");
      lv_obj_clear_flag(news_label, LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }
  // 获取更新时间
  String updateTime = "";
  if (doc.containsKey("update_time")) {
    if (doc["update_time"].is<unsigned long>()) {
      // 将时间戳转换为可读格式
      unsigned long timestamp = doc["update_time"].as<unsigned long>();
      time_t now = timestamp / 1000;
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
  String newsText = ""; 
  // 检查是否有新闻列表
  if (doc.containsKey("result") && doc["result"].is<JsonArray>()) {
    JsonArray newsArray = doc["result"].as<JsonArray>();
    // 显示前几条新闻
    int displayCount = min(static_cast<int>(newsArray.size()), 18); // 最多显示18条新闻
    for (int i = 0; i < displayCount; i++) {
      String newsItem = newsArray[i].as<String>();      
        newsText += "" + newsItem + "\n";
    }
  } else {
    // 处理简单的字符串格式新闻数据
    if (doc.containsKey("result") && doc["result"].is<const char*>()) {
      newsText += doc["result"].as<const char*>();
    } else {
      newsText += "暂无新闻内容";
    }
  }
  // 更新新闻标签
  if (news_label && lv_obj_is_valid(news_label)) {
    // 更新标签颜色和背景色
    lv_label_set_text(news_label, newsText.c_str());
    lv_obj_clear_flag(news_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(news_label);
  }
}
//*** 初始化显示管理器
void initDisplayManager() {
  Serial.println("初始化显示管理器");
  
  // 确保SPIFFS已初始化
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS初始化失败");
  }
}