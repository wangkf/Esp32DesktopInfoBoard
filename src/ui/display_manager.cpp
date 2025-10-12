#include "includes.h"
#include <map>
#include <vector>
#include <string>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <lvgl.h>

// 外部变量声明
extern lv_obj_t* news_label;
extern lv_obj_t* iciba_label;
extern lv_obj_t* astronauts_label;

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
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), 0); // 默认黑色
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
 * 显示新闻数据
 */
void displayNewsDataFromFile() {
  Serial.println("从文件显示新闻数据");
  
  // 确保news_label已创建和初始化
  createAndInitLabel(news_label, "news_label");
  
  JsonDocument doc;
  if (!readJsonFromFile("/news.json", doc)) {
    if (news_label && lv_obj_is_valid(news_label)) {
      lv_label_set_text(news_label, "无法读取新闻数据文件");
    }
    return;
  }

  // 获取更新时间
  String updateTime = "";
  bool hasNewsList = false;
  
  // 构建新闻显示文本，在第一行右边括号中显示更新时间
  String newsText = "最新资讯";
  
  // 优先检查是否包含result对象（标准格式）
  if (doc.containsKey("result") && doc["result"].is<JsonObject>()) {
    JsonObject result = doc["result"].as<JsonObject>();
    
    // 从result中获取更新时间
    if (result.containsKey("last_updated")) {
      updateTime = result["last_updated"].as<const char*>();
    }
    
    // 更新时间显示
    if (!updateTime.isEmpty()) {
      newsText += " (" + updateTime + ")";
    }
    newsText += "\n";
    
    // 从result中获取newslist
    if (result.containsKey("newslist") && result["newslist"].is<JsonArray>()) {
      JsonArray newsList = result["newslist"].as<JsonArray>();
      hasNewsList = true;
      
      for (int i = 0; i < newsList.size() && i < 5; i++) {  // 限制显示最多5条新闻
        if (newsList[i].containsKey("title") && newsList[i]["title"].is<const char*>()) {
          newsText += String(i + 1) + ". " + String(newsList[i]["title"].as<const char*>()) + "\n";
        }
      }
    } 
    // 向后兼容：检查list数组
    else if (result.containsKey("list") && result["list"].is<JsonArray>()) {
      JsonArray newsList = result["list"].as<JsonArray>();
      hasNewsList = true;
      
      for (int i = 0; i < newsList.size() && i < 5; i++) {  // 限制显示最多5条新闻
        if (newsList[i].containsKey("title") && newsList[i]["title"].is<const char*>()) {
          newsText += String(i + 1) + ". " + String(newsList[i]["title"].as<const char*>()) + "\n";
        }
      }
    }
    
    if (!hasNewsList) {
      newsText += "暂无新闻数据";
    }
  }
  // 否则直接检查根节点（天聚数行API原始格式）
  else {
    // 从根节点获取更新时间
    if (doc.containsKey("last_updated")) {
      updateTime = doc["last_updated"].as<const char*>();
    }
    
    // 更新时间显示
    if (!updateTime.isEmpty()) {
      newsText += " (" + updateTime + ")";
    }
    newsText += "\n";
    
    // 从根节点获取newslist
    if (doc.containsKey("newslist") && doc["newslist"].is<JsonArray>()) {
      JsonArray newsList = doc["newslist"].as<JsonArray>();
      
      for (int i = 0; i < newsList.size() && i < 5; i++) {  // 限制显示最多5条新闻
        if (newsList[i].containsKey("title") && newsList[i]["title"].is<const char*>()) {
          newsText += String(i + 1) + ". " + String(newsList[i]["title"].as<const char*>()) + "\n";
        }
      }
    } else {
      // 没有找到有效的新闻列表
      newsText += "暂无新闻数据";
    }
  }
  
  // 更新新闻标签
  if (news_label && lv_obj_is_valid(news_label)) {
    lv_label_set_text(news_label, newsText.c_str());
    lv_obj_move_foreground(news_label); // 确保标签显示在最上层
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
    }
    return;
  }

  
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
    lv_obj_move_foreground(astronauts_label); // 确保标签显示在最上层
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