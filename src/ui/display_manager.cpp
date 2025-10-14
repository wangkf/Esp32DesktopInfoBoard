#include "includes.h"
#include <map>
#include <vector>
#include <string>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <lvgl.h>
#include <time.h>

// 外部变量声明
extern lv_obj_t* iciba_label;
extern lv_obj_t* astronauts_label;
extern lv_obj_t* news_label;
extern lv_obj_t* calendar_label;

// 全局变量
extern const uint32_t screenWidth;
extern const uint32_t screenHeight;

/**
 * 从SPIFFS加载图片并使用LVGL显示
 * @param filename SPIFFS中的文件名
 * @param x 显示位置的X坐标
 * @param y 显示位置的Y坐标
 * @return 创建的LVGL图片对象指针
 */
lv_obj_t* displayImageFromSPIFFS(const char* filename, int x, int y) {
  Serial.println("尝试从SPIFFS加载图片: " + String(filename));
  
  // 检查文件是否存在
  if (!SPIFFS.exists(filename)) {
    Serial.println("文件不存在: " + String(filename));
    return NULL;
  }
  
  // 获取文件大小
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.println("打开文件失败: " + String(filename));
    return NULL;
  }
  
  size_t fileSize = file.size();
  file.close(); // 只需要检查文件是否存在和大小，不需要保持打开
  
  Serial.printf("文件大小: %u 字节\n", fileSize);
  
  // 创建LVGL图像对象
  lv_obj_t* img = lv_img_create(lv_scr_act());
  if (!img) {
    Serial.println("创建LVGL图像对象失败");
    return NULL;
  }
  
  // 确保图像对象在最顶层显示
  lv_obj_move_foreground(img);
  
  // 使用LVGL内置的PNG解码器加载图片
  // 为SPIFFS文件添加正确的文件系统前缀
  String lvglFilePath = "S:";
  lvglFilePath += filename;
  
  Serial.println("LVGL文件路径: " + lvglFilePath);
  
  // 使用带文件系统前缀的路径设置图像源
  lv_img_set_src(img, lvglFilePath.c_str());
  Serial.println("已设置图像源");
  
  // 设置图像位置
  lv_obj_set_pos(img, x, y);
  
  // 设置图像样式
  lv_obj_set_style_border_width(img, 0, 0);
  lv_obj_set_style_radius(img, 0, 0);
  lv_obj_set_style_bg_opa(img, 0, 0); // 透明背景
  
  // 显式刷新LVGL显示
  lv_refr_now(lv_disp_get_default());
  lv_task_handler();
  
  Serial.println("图片显示成功，已刷新显示");
  
  return img;
}

/**
 * 创建并初始化通用标签
 * @param label 标签指针的引用
 * @param labelName 标签名称，用于调试
 */
void createAndInitLabel(lv_obj_t* &label, const char* labelName) {
  if (label == NULL || !lv_obj_is_valid(label)) {
    if (label != NULL) {
      // 确保旧标签被正确清理
      lv_obj_del(label);
    }
    label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label, &lvgl_font_song_16, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0); // 白色文字，适配黑色背景
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP); // 设置自动换行
    lv_obj_set_width(label, screenWidth - 40); // 设置标签宽度
    lv_obj_set_height(label, screenHeight - 120); // 固定高度
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 100); // 顶部居中对齐，顶部离屏幕顶部100px
    
    // 不添加背景和边框效果
    lv_obj_set_style_bg_opa(label, 0, 0); // 完全透明背景
    lv_obj_set_style_border_width(label, 0, 0); // 无边框
    lv_obj_set_style_radius(label, 0, 0); // 无圆角
    lv_obj_set_style_pad_all(label, 10, 0); // 内边距
    
    Serial.println("创建或重新创建了" + String(labelName));
  }
}

/**
 * 显示日历信息
 */
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
    calendarText += "    "; // 四个空格
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
  
  Serial.println("日历显示完成");
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



/**
 * 显示金山词霸每日信息
 */
void displayIcibaDataFromFile() {
  Serial.println("从文件显示金山词霸数据");
  
  JsonDocument doc;
  if (!readJsonFromFile("/iciba.json", doc)) {
    if (iciba_label && lv_obj_is_valid(iciba_label)) {
      lv_label_set_text(iciba_label, "无法读取金山词霸数据文件");
      lv_obj_clear_flag(iciba_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
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
  }
  
  // 在词霸文字下方显示data目录下的iciba.png图片作为装饰
  const char* imagePath = "/iciba.png";
  
  // 检查图片文件是否存在
  if (SPIFFS.exists(imagePath)) {
    Serial.println("找到iciba.png图片，准备显示");
    
    // 获取标签的位置和大小，以便确定图片显示位置
    lv_area_t labelArea;
    lv_obj_get_coords(iciba_label, &labelArea);
    
    // 计算图片显示位置：在标签下方居中显示
    int imageX = (screenWidth - 240) / 2; // 假设图片宽度为240，居中显示
    int imageY = labelArea.y2 - 50; // 在标签下方10像素处
    
    // 显示图片 - 使用新的LVGL图片显示函数
    displayImageFromSPIFFS(imagePath, imageX, imageY);
  } else {
    Serial.println("未找到iciba.png图片");
  }
}

/**
 * 显示留言板内容
 */
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
  
  // 添加更新时间（如果有）
  /*
  if (doc.containsKey("update_time")) {
    if (doc["update_time"].is<unsigned long>()) {
      // 将时间戳转换为可读格式
      unsigned long timestamp = doc["update_time"].as<unsigned long>();
      time_t now = timestamp / 1000; // 转换为秒
      struct tm *timeinfo = localtime(&now);
      if (timeinfo != nullptr) {
        char timeString[20];
        strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", timeinfo);
        noteText += "\n\n更新时间: " + String(timeString);
      }
    } else if (doc["update_time"].is<const char*>()) {
      noteText += "\n\n更新时间: " + String(doc["update_time"].as<const char*>());
    }
  }
  */
  // 更新留言板标签
  if (note_label && lv_obj_is_valid(note_label)) {
    lv_label_set_text(note_label, noteText.c_str());
    lv_obj_clear_flag(note_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    lv_obj_move_foreground(note_label); // 确保标签显示在最上层
  }
}

/**
 * 显示宇航员信息
 */
void displayAstronautsDataFromFile() {
  Serial.println("从文件显示宇航员数据");
  
  // 确保astronauts_label已创建和初始化
  createAndInitLabel(astronauts_label, "astronauts_label");
  
  JsonDocument doc;
  if (!readJsonFromFile("/astronauts.json", doc)) {
    if (astronauts_label && lv_obj_is_valid(astronauts_label)) {
      lv_label_set_text(astronauts_label, "无法读取宇航员数据文件");
      lv_obj_clear_flag(astronauts_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    }
    return;
  }

  // 先检查doc是否包含"people"
  if (!doc.containsKey("people")) {
    if (astronauts_label && lv_obj_is_valid(astronauts_label)) {
      lv_label_set_text(astronauts_label, "JSON格式错误：缺少people字段");
      lv_obj_clear_flag(astronauts_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    }
    return;
  }
  
  // 检查"people"是否为JsonArray
  if (doc["people"].is<JsonArray>()) {
    JsonArray peopleArray = doc["people"].as<JsonArray>();
    
    // 构建宇航员显示文本
    String astronautsText = "太空宇航员列表\n";
    
    // 遍历宇航员数组
    for (JsonVariant astronaut : peopleArray) {
      if (astronaut.containsKey("name") && astronaut.containsKey("craft")) {
        String name = astronaut["name"].as<String>();
        String craft = astronaut["craft"].as<String>();
        astronautsText += name + " - " + craft + "\n";
      }
    }
    
    // 更新宇航员标签
    if (astronauts_label && lv_obj_is_valid(astronauts_label)) {
      lv_label_set_text(astronauts_label, astronautsText.c_str());
      lv_obj_clear_flag(astronauts_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
      lv_obj_move_foreground(astronauts_label); // 确保标签显示在最上层
    }
  }
  // 如果不是数组，检查是否为对象
  else if (doc["people"].is<JsonObject>()) {
    JsonObject result = doc["people"].as<JsonObject>();
    
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
    if (astronauts_label && lv_obj_is_valid(astronauts_label)) {
      lv_label_set_text(astronauts_label, astronautsText.c_str());
      lv_obj_clear_flag(astronauts_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
      lv_obj_move_foreground(astronauts_label); // 确保标签显示在最上层
    }
  }
  else {
    if (astronauts_label && lv_obj_is_valid(astronauts_label)) {
      lv_label_set_text(astronauts_label, "JSON格式错误：people字段格式不正确");
      lv_obj_clear_flag(astronauts_label, LV_OBJ_FLAG_HIDDEN); // 确保标签可见
    }
  }
}

/**
 * 显示新闻信息
 */
void displayNewsDataFromFile() {
  Serial.println("从文件显示新闻数据");
  
  // 确保news_label已创建和初始化
  if (!news_label) {
    Serial.println("news_label未创建，创建并初始化");
    news_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(news_label, &lvgl_font_song_16, 0);
    lv_obj_set_style_text_color(news_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_width(news_label, screenWidth - 20);
    lv_obj_set_height(news_label, screenHeight - 120);
    lv_obj_align(news_label, LV_ALIGN_TOP_LEFT, 10, 100);
    lv_label_set_long_mode(news_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_radius(news_label, 10, 0);
    lv_obj_set_style_bg_color(news_label, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(news_label, 100, 0);
  }
  
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
    int displayCount = min(static_cast<int>(newsArray.size()), 16); // 最多显示5条新闻
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
    lv_label_set_text(news_label, newsText.c_str());
    lv_obj_clear_flag(news_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(news_label);
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
  Serial.print("测试从URL显示图片功能已移除: ");
  Serial.println(url);
  
  // 清除屏幕
  lv_obj_clean(lv_scr_act());
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xFFFFFF), 0);
  
  // 创建状态标签
  lv_obj_t* status_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(status_label, &lvgl_font_song_16, 0);
  lv_obj_set_style_text_color(status_label, lv_color_hex(0x000000), 0);
  lv_label_set_text(status_label, "图片显示功能已移除");
  lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 10);
  
  // 刷新显示以显示状态信息
  lv_refr_now(lv_disp_get_default());
  lv_task_handler();
  
  Serial.println("图片显示测试完成");
}