#include "web_config_server.h"
#include <ArduinoJson.h>
#include "config/config.h"
#include "config/config_manager.h"

// 定义单例实例
WebConfigServer* WebConfigServer::instance = nullptr;

/**
 * 处理系统重启请求
 */
void WebConfigServer::handleRestart() {
    Serial.println("接收到系统重启请求...");
    
    // 发送重启确认页面
    String html = "";
    html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>系统重启 - ESP32信息板</title>";
    html += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
    html += "<style>body { background-color: #f8f9fa; padding-top: 20px; } .container { max-width: 800px; } .card { border: none; box-shadow: 0 4px 8px rgba(0,0,0,0.1); border-radius: 10px; }</style>";
    html += "</head><body>";
    
    // 统一的导航栏样式（亮色主题）
    html += "<div class='container'>";
    html += "<nav class='navbar navbar-expand-lg navbar-light bg-light mb-4'>";
    html += "  <div class='container-fluid'>";
    html += "    <a class='navbar-brand text-white' href='/'>ESP32信息板</a>";
    html += "  </div>";
    html += "</nav>";
    
    // 内容区域
    html += "<div class='card mb-4'>";
    html += "  <div class='card-header bg-success text-white'>系统重启中</div>";
    html += "  <div class='card-body'>";
    html += "    <h2 class='text-success'>设备将在3秒后重启</h2>";
    html += "    <p>请稍候，系统正在准备重启...</p>";
    html += "    <p class='text-muted'>如果设备未自动重启，请手动断开电源后重新连接。</p>";
    html += "    <a href='/' class='btn btn-success'>返回首页（如果尚未重启）</a>";
    html += "  </div>";
    html += "</div>";
    
    // 底部版权信息
    html += "<footer class='text-center text-muted'>";
    html += "  <p>ESP32桌面信息板 - 基于ESP32的多功能信息显示系统</p>";
    html += "  <p><a href='https://github.com/wangkf/Esp32DesktopInfoBoard' class='text-success'>github.com/wangkf/Esp32DesktopInfoBoard</a></p>";
    html += "</footer>";
    
    html += "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>";
    html += "</div>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
    
    // 延迟一小段时间，让客户端有足够的时间接收响应
    delay(3000);
    
    // 重启ESP32
    ESP.restart();
}

/**
 * 处理主题设置请求
 */
void WebConfigServer::handleTheme() {
    if (server.hasArg("theme")) {
        String theme = server.arg("theme");
        bool isLightTheme = (theme == "light");
        
        // 使用ConfigManager保存主题配置
        ConfigManager* configManager = ConfigManager::getInstance();
        if (configManager->isConfigLoaded()) {
            bool themeSaved = configManager->setDisplayTheme(isLightTheme);
            
            if (themeSaved) {
                Serial.println("主题配置保存成功");
                String successHtml = "";
                successHtml += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>配置成功 - ESP32信息板</title>";
                successHtml += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
                successHtml += "<style>body { background-color: #f8f9fa; padding-top: 20px; } .container { max-width: 800px; } .card { border: none; box-shadow: 0 4px 8px rgba(0,0,0,0.1); border-radius: 10px; }</style>";
                successHtml += "</head><body>";
                
                // 统一的导航栏样式
                successHtml += "<div class='container'>";
                successHtml += "<nav class='navbar navbar-expand-lg bg-info mb-4'>";
                successHtml += "  <div class='container-fluid'>";
                successHtml += "    <a class='navbar-brand text-white' href='/'>ESP32信息板</a>";
                successHtml += "  </div>";
                successHtml += "</nav>";
                
                // 内容区域
                successHtml += "<div class='card mb-4'>";
                successHtml += "  <div class='card-header bg-success text-white'>操作成功</div>";
                successHtml += "  <div class='card-body'>";
                successHtml += "    <div class='alert alert-success'>主题设置已成功保存！</div>";
                successHtml += "    <div class='text-center mt-4'>";
                successHtml += "      <a href='/' class='btn btn-primary'>返回首页</a>";
                successHtml += "    </div>";
                successHtml += "  </div>";
                successHtml += "</div>";
                
                successHtml += "</div>";
                successHtml += "</body></html>";
                
                server.send(200, "text/html", successHtml);
            } else {
                Serial.println("主题配置保存失败");
                String errorHtml = "";
                errorHtml += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>配置失败 - ESP32信息板</title>";
                errorHtml += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
                errorHtml += "<style>body { background-color: #f8f9fa; padding-top: 20px; } .container { max-width: 800px; } .card { border: none; box-shadow: 0 4px 8px rgba(0,0,0,0.1); border-radius: 10px; }</style>";
                errorHtml += "</head><body>";
                
                // 统一的导航栏样式
                errorHtml += "<div class='container'>";
                errorHtml += "<nav class='navbar navbar-expand-lg bg-info mb-4'>";
                errorHtml += "  <div class='container-fluid'>";
                errorHtml += "    <a class='navbar-brand text-white' href='/'>ESP32信息板</a>";
                errorHtml += "  </div>";
                errorHtml += "</nav>";
                
                // 内容区域
                errorHtml += "<div class='card mb-4'>";
                errorHtml += "  <div class='card-header bg-danger text-white'>操作失败</div>";
                errorHtml += "  <div class='card-body'>";
                errorHtml += "    <div class='alert alert-danger'>主题设置保存失败，请重试！</div>";
                errorHtml += "    <div class='text-center mt-4'>";
                errorHtml += "      <a href='/' class='btn btn-primary'>返回首页</a>";
                errorHtml += "    </div>";
                errorHtml += "  </div>";
                errorHtml += "</div>";
                
                errorHtml += "</div>";
                errorHtml += "</body></html>";
                
                server.send(200, "text/html", errorHtml);
            }
        } else {
            Serial.println("配置未加载，无法保存主题设置");
            String errorHtml = "";
            errorHtml += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>配置错误 - ESP32信息板</title>";
            errorHtml += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
            errorHtml += "<style>body { background-color: #f8f9fa; padding-top: 20px; } .container { max-width: 800px; } .card { border: none; box-shadow: 0 4px 8px rgba(0,0,0,0.1); border-radius: 10px; }</style>";
            errorHtml += "</head><body>";
            
            // 统一的导航栏样式
            errorHtml += "<div class='container'>";
            errorHtml += "<nav class='navbar navbar-expand-lg bg-info mb-4'>";
            errorHtml += "  <div class='container-fluid'>";
            errorHtml += "    <a class='navbar-brand text-white' href='/'>ESP32信息板</a>";
            errorHtml += "  </div>";
            errorHtml += "</nav>";
            
            // 内容区域
            errorHtml += "<div class='card mb-4'>";
            errorHtml += "  <div class='card-header bg-danger text-white'>配置错误</div>";
            errorHtml += "  <div class='card-body'>";
            errorHtml += "    <div class='alert alert-danger'>配置未加载，无法保存主题设置！</div>";
            errorHtml += "    <div class='text-center mt-4'>";
            errorHtml += "      <a href='/' class='btn btn-primary'>返回首页</a>";
            errorHtml += "    </div>";
            errorHtml += "  </div>";
            errorHtml += "</div>";
            
            errorHtml += "</div>";
            errorHtml += "</body></html>";
            
            server.send(200, "text/html", errorHtml);
        }
    } else {
        server.send(400, "text/html", "<h1>400 Bad Request</h1><p>缺少必要参数: theme</p>");
    }
}

/**
 * 私有构造函数
 */
WebConfigServer::WebConfigServer() : server(80), isRunning(false), 
                                    apSSID("ESP32-InfoBoard"), apPassword("12345678") {
}

/**
 * 获取单例实例
 */
WebConfigServer* WebConfigServer::getInstance() {
    if (instance == nullptr) {
        instance = new WebConfigServer();
    }
    return instance;
}

/**
 * 初始化Web配置服务器
 */
void WebConfigServer::init() {
    // 注册处理函数
    server.on("/", HTTP_GET, std::bind(&WebConfigServer::handleRoot, this));
    server.on("/config", HTTP_POST, std::bind(&WebConfigServer::handleConfig, this));
    server.on("/restart", HTTP_POST, std::bind(&WebConfigServer::handleRestart, this));
    server.on("/json-files", HTTP_GET, std::bind(&WebConfigServer::handleJsonFile, this));
    server.on("/json-file", HTTP_GET, std::bind(&WebConfigServer::handleJsonFileContent, this));
    server.on("/note", HTTP_POST, std::bind(&WebConfigServer::handleNote, this));
    server.on("/theme", HTTP_POST, std::bind(&WebConfigServer::handleTheme, this));
    // 添加各屏幕页面的路由
    server.on("/screen-news", HTTP_GET, std::bind(&WebConfigServer::handleNewsScreen, this));
    server.on("/screen-calendar", HTTP_GET, std::bind(&WebConfigServer::handleCalendarScreen, this));
    server.on("/screen-notes", HTTP_GET, std::bind(&WebConfigServer::handleNotesScreen, this));
    server.on("/screen-iciba", HTTP_GET, std::bind(&WebConfigServer::handleIcibaScreen, this));
    server.on("/screen-astronauts", HTTP_GET, std::bind(&WebConfigServer::handleAstronautsScreen, this));
    server.onNotFound(std::bind(&WebConfigServer::handleNotFound, this));
}

/**
 * 启动Web配置服务器
 * @param useAPMode 是否使用接入点模式（true）或STA模式（false）
 */
bool WebConfigServer::start(bool useAPMode) {
    Serial.println("启动Web配置服务器...");
    if (useAPMode) {
        // 断开当前WiFi连接
        WiFi.disconnect();
        delay(1000);
        // 设置为接入点模式
        WiFi.softAP(apSSID, apPassword);
        // 等待接入点启动
        delay(2000);
        // 获取接入点IP地址
        IPAddress apIP = WiFi.softAPIP();
        Serial.print("接入点IP地址: ");
        Serial.println(apIP);   
        Serial.println("Web配置服务器已启动，访问 192.168.4.1 进行配置");
    } else {
        // 使用STA模式（已连接WiFi的情况下）
        Serial.print("当前设备IP地址: ");
        Serial.println(WiFi.localIP());
        
        Serial.println("Web配置服务器已启动，使用当前IP地址访问");
    }
    
    // 开始Web服务器
    server.begin();
    isRunning = true;
    
    return true;
}

// 保留原有的start方法作为兼容接口
bool WebConfigServer::start() {
    return start(true); // 默认使用接入点模式
}

/**
 * 停止Web配置服务器
 */
void WebConfigServer::stop() {
    if (isRunning) {
        Serial.println("停止Web配置服务器...");
        server.stop();
        WiFi.softAPdisconnect(true);
        isRunning = false;
        Serial.println("Web配置服务器已停止");
    }
}

/**
 * 处理Web服务器请求
 */
void WebConfigServer::handleClient() {
    if (isRunning) {
        server.handleClient();
    }
}

/**
 * 生成通用的屏幕页面模板
 */
String WebConfigServer::generateScreenPage(const String& screenName, const String& screenTitle, const String& content) {
    String html = "";
    html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>" + screenTitle + "</title>";
    html += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
    html += "<style>body { padding-top: 20px; background-color: #f8f9fa; } .container { max-width: 900px; } .navbar-brand { font-weight: bold; } .btn-primary { background-color: #4CAF50; border-color: #4CAF50; } .btn-primary:hover { background-color: #45a049; }</style>";
    html += "</head><body>";
    
    // 统一导航栏样式 - 改为亮色主题
    html += "<div class='container'>";
    html += "<nav class='navbar navbar-expand-lg bg-info shadow-sm mb-4'>";
    html += "  <div class='container-fluid'>";
    html += "    <a class='navbar-brand text-success' href='/'>ESP32信息板</a>";
    html += "    <button class='navbar-toggler' type='button' data-bs-toggle='collapse' data-bs-target='#navbarNav' aria-controls='navbarNav' aria-expanded='false' aria-label='Toggle navigation'>";
    html += "      <span class='navbar-toggler-icon'></span>";
    html += "    </button>";
    html += "    <div class='collapse navbar-collapse' id='navbarNav'>";
    html += "      <ul class='navbar-nav'>";
      html += "        <li class='nav-item'><a class='nav-link text-white";
      if (screenName == "") html += " active bg-primary font-weight-bold";
      html += "' href='/'>首页</a></li>";
      html += "        <li class='nav-item'><a class='nav-link text-white' href='/json-files'>JSON文件</a></li>";
      
      // 处理新闻导航项
      html += "        <li class='nav-item'><a class='nav-link text-white";
      if (screenName == "news") html += " active bg-primary font-weight-bold";
      html += "' href='/screen-news'>新闻</a></li>";
      
      // 处理日历导航项
      html += "        <li class='nav-item'><a class='nav-link text-white";
      if (screenName == "calendar") html += " active bg-primary font-weight-bold";
      html += "' href='/screen-calendar'>日历</a></li>";
  
      // 处理每日一句导航项
      html += "        <li class='nav-item'><a class='nav-link text-white";
      if (screenName == "iciba") html += " active bg-primary font-weight-bold";
      html += "' href='/screen-iciba'>每日一句</a></li>";
      
      // 处理宇航员导航项
      html += "        <li class='nav-item'><a class='nav-link text-white";
      if (screenName == "astronauts") html += " active bg-primary font-weight-bold";
      html += "' href='/screen-astronauts'>太空站宇航员</a></li>";
      html += "      </ul>";
    html += "    </div>";
    html += "  </div>";
    html += "</nav>";
    
    // 内容区域 - 活泼的配色
    html += "<div class='card mb-4 shadow-sm border-0 rounded-lg'>";
    html += "<div class='card-header bg-success text-white font-bold'>" + screenTitle + "</div>";
    html += "<div class='card-body bg-white'>";
    html += content;
    html += "</div>";
    html += "</div>";
    
    html += "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>";
    
    // 底部版权信息
    html += "<footer class='mt-5 py-3 bg-light border-top'>";
    html += "<div class='container'>";
    html += "<div class='row'>";
    html += "<div class='col-md-6'>";
    html += "<p class='text-muted'><strong>ESP32桌面信息板</strong></p>";
    html += "<p class='text-sm text-muted'>基于ESP32微控制器的多功能信息显示系统，可实时展示热搜头条、日历、乌鸡汤、名言警句、金山词霸每日一句、国际空间站宇航员信息等内容，并支持自动换屏、光线感应调节亮度和Web配置等功能。</p>";
    html += "</div>";
    html += "<div class='col-md-6 text-md-end'>";
    html += "<p class='text-muted'>作者:wangkf 邮箱：<a href='mailto:wangkf@qq.com'>wangkf@gmail.com</a></p>";
    html += "<p class='text-muted'>业余无线电呼号：BI9ABS QTH：OM44kf</p>";
    html += "<p class='text-muted'>邮箱：<a href='mailto:wangkf@qq.com'>wangkf@gmail.com</a></p>";
    html += "<p class='text-muted'><a href='https://github.com/wangkf/Esp32DesktopInfoBoard' target='_blank'>GitHub项目地址</a></p>";
    html += "</div>";
    html += "</div>";
    html += "</div>";
    html += "</footer>";
    
    html += "</div>";
    html += "</body></html>";
    return html;
}

/**
 * 处理新闻屏幕页面
 */
void WebConfigServer::handleNewsScreen() {
    String content = "<div class='alert alert-info'>新闻内容将在这里显示</div>";
    
    // 尝试读取新闻数据文件，尝试多种可能的文件名
    File file = SPIFFS.open("/news.json", "r");
    if (!file) {
        file = SPIFFS.open("/data/news.json", "r");
    }
    
    if (file) {
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        
        if (!error) {
            content = "<div class='news-container'>";
            
            // 检查是否有result数组
            if (doc.containsKey("result") && doc["result"].is<JsonArray>()) {
                JsonArray results = doc["result"].as<JsonArray>();
                
                for (size_t i = 0; i < results.size(); i++) {
                    content += "<div class='card mb-3'>";
                    content += "<div class='card-body'>";
                    
                    // 直接显示result条目的内容
                    JsonVariant result = results[i];
                    if (result.is<JsonObject>()) {
                        JsonObject obj = result.as<JsonObject>();
                        // 遍历所有键值对并显示
                        for (JsonPair kv : obj) {
                            content += "<p class='card-text'><strong>" + String(kv.key().c_str()) + ":</strong> " + String(kv.value().as<String>()) + "</p>";
                        }
                    } else if (result.is<String>()) {
                        // 如果是字符串，直接显示
                        content += "<p class='card-text'>" + result.as<String>() + "</p>";
                    } else {
                        // 其他类型，转换为字符串显示
                        content += "<p class='card-text'>" + String(result.as<String>()) + "</p>";
                    }
                    
                    content += "</div></div>";
                }
            } else {
                content = "<div class='alert alert-warning'>新闻数据格式不正确，未找到result数组</div>";
            }
            
            content += "</div>";
        } else {
            content = "<div class='alert alert-danger'>JSON解析失败: " + String(error.c_str()) + "</div>";
        }
    } else {
        content = "<div class='alert alert-danger'>无法打开新闻数据文件</div>";
    }
    
    String html = generateScreenPage("news", "新闻屏幕", content);
    server.send(200, "text/html", html);
}

/**
 * 处理日历屏幕页面
 */
void WebConfigServer::handleCalendarScreen() {
    String content = "";
    
    // 获取当前日期时间
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // 格式化显示当前日期
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y年%m月%d日 %A", &timeinfo);
    String currentDateStr = buffer;
    int currentDay = timeinfo.tm_mday;
    
    // 显示大字号的日期数字
    content += "<div class='text-center mb-5'>";
    content += "<h1 class='display-1'>" + String(currentDay) + "</h1>";
    content += "<p class='lead'>" + currentDateStr + "</p>";
    content += "</div>";
    
    // 生成月历表格
    content += generateMonthCalendar(timeinfo.tm_year, timeinfo.tm_mon);
    
    String html = generateScreenPage("calendar", "日历屏幕", content);
    server.send(200, "text/html", html);
}

/**
 * 生成月历表格
 */
String WebConfigServer::generateMonthCalendar(int year, int month) {
    String calendar = "<div class='calendar-container mx-auto' style='max-width: 400px;'>";
    
    // 月份名称
    const char* monthNames[] = {"一月", "二月", "三月", "四月", "五月", "六月", 
                                "七月", "八月", "九月", "十月", "十一月", "十二月"};
    
    calendar += "<h3 class='text-center mb-3'>" + String(year + 1900) + "年 " + monthNames[month] + "</h3>";
    
    // 星期标题行
    calendar += "<table class='table table-bordered text-center'>";
    calendar += "<thead><tr class='bg-info text-white'>";
    calendar += "<th>日</th><th>一</th><th>二</th><th>三</th><th>四</th><th>五</th><th>六</th>";
    calendar += "</tr></thead><tbody>";
    
    // 计算该月第一天是星期几
    struct tm firstDay;
    firstDay.tm_year = year;
    firstDay.tm_mon = month;
    firstDay.tm_mday = 1;
    firstDay.tm_hour = 0;
    firstDay.tm_min = 0;
    firstDay.tm_sec = 0;
    mktime(&firstDay);
    
    // 获取该月的天数
    int daysInMonth;
    if (month == 1) { // 二月
        // 判断闰年
        bool isLeapYear = (year + 1900) % 4 == 0 && ((year + 1900) % 100 != 0 || (year + 1900) % 400 == 0);
        daysInMonth = isLeapYear ? 29 : 28;
    } else if (month == 3 || month == 5 || month == 8 || month == 10) {
        daysInMonth = 30;
    } else {
        daysInMonth = 31;
    }
    
    // 生成日历单元格
    int day = 1;
    int currentDayOfWeek = firstDay.tm_wday;
    
    // 填充第一行之前的空白
    calendar += "<tr>";
    for (int i = 0; i < currentDayOfWeek; i++) {
        calendar += "<td>&nbsp;</td>";
    }
    
    // 填充日期
    while (day <= daysInMonth) {
        // 标记今天
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        String dayClass = "";
        if (year == timeinfo.tm_year && month == timeinfo.tm_mon && day == timeinfo.tm_mday) {
            dayClass = "bg-primary text-white font-weight-bold";
        }
        
        calendar += "<td class='" + dayClass + "'>" + String(day) + "</td>";
        
        if (currentDayOfWeek == 6 && day < daysInMonth) { // 星期六且不是最后一天，换行
            calendar += "</tr><tr>";
            currentDayOfWeek = 0;
        } else {
            currentDayOfWeek++;
        }
        
        day++;
    }
    
    // 填充最后一行剩余的空白
    while (currentDayOfWeek < 7 && currentDayOfWeek > 0) {
        calendar += "<td>&nbsp;</td>";
        currentDayOfWeek++;
    }
    
    calendar += "</tr></tbody></table>";
    calendar += "</div>";
    
    return calendar;
}

/**
 * 处理留言板屏幕页面 - 重定向到首页，因为首页已有留言板配置
 */
void WebConfigServer::handleNotesScreen() {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
}

/**
 * 处理每日一句屏幕页面
 */
void WebConfigServer::handleIcibaScreen() {
    String content = "<div class='alert alert-info'>每日一句内容将在这里显示</div>";
    
    // 尝试读取每日一句数据文件，尝试多种可能的文件名
    File file = SPIFFS.open("/iciba.json", "r");
    if (!file) {
        file = SPIFFS.open("/data/iciba.json", "r");
    }
    
    if (file) {
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        
        if (!error) {
            content = "<div class='iciba-container card p-4'>";
            
            // 显示content
            if (doc.containsKey("content")) {
                content += "<p class='lead mb-2'>" + doc["content"].as<String>() + "</p>";
            }
            
            // 显示note
            if (doc.containsKey("note")) {
                content += "<p class='mb-3 text-muted'>" + doc["note"].as<String>() + "</p>";
            }
            
            // 显示fenxiang_img
            if (doc.containsKey("fenxiang_img")) {
                String imgUrl = doc["fenxiang_img"].as<String>();
                content += "<div class='mb-3 text-center'>";
                content += "<img src='" + imgUrl + "' class='img-fluid rounded' alt='每日一句图片'>";
                content += "</div>";
            }
            
            // 显示tts播放按钮
            if (doc.containsKey("tts")) {
                String ttsUrl = doc["tts"].as<String>();
                content += "<div class='mb-3'>";
                content += "<audio controls>";
                content += "<source src='" + ttsUrl + "' type='audio/mpeg'>";
                content += "您的浏览器不支持音频元素。";
                content += "</audio>";
                content += "</div>";
            }
            
            // 显示last_updated
            if (doc.containsKey("last_updated")) {
                content += "<p class='text-sm text-muted'>更新时间: " + doc["last_updated"].as<String>() + "</p>";
            }
            
            content += "</div>";
        } else {
            content = "<div class='alert alert-danger'>JSON解析失败: " + String(error.c_str()) + "</div>";
        }
    } else {
        content = "<div class='alert alert-danger'>无法打开每日一句数据文件</div>";
    }
    
    String html = generateScreenPage("iciba", "每日一句屏幕", content);
    server.send(200, "text/html", html);
}

/**
 * 处理太空站宇航员屏幕页面
 */
void WebConfigServer::handleAstronautsScreen() {
    String content = "<div class='alert alert-info'>太空站宇航员内容将在这里显示</div>";
    
    // 尝试读取宇航员数据文件，尝试多种可能的文件名
    File file = SPIFFS.open("/astronauts.json", "r");
    if (!file) {
        file = SPIFFS.open("/data/astronauts.json", "r");
    }
    
    if (file) {
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        
        if (!error) {
            content = "<div class='astronauts-container'>";
            
            int astronautCount = 0;
            
            // 计算宇航员数量并显示
            if (doc.containsKey("people") && doc["people"].is<JsonArray>()) {
                JsonArray people = doc["people"].as<JsonArray>();
                astronautCount = people.size();
                
                content += "<p class='lead mb-4'>太空目前有 <strong>" + String(astronautCount) + "</strong> 名宇航员</p>";
                
                // 按要求格式显示每个宇航员信息
                content += "<div class='list-group'>";
                for (JsonObject person : people) {
                    String name = person.containsKey("name") ? person["name"].as<String>() : "未知";
                    String craft = person.containsKey("craft") ? person["craft"].as<String>() : "未知";
                    
                    content += "<div class='list-group-item'>" + craft + ": " + name + "</div>";
                }
                content += "</div>";
            } else {
                content = "<div class='alert alert-warning'>宇航员数据格式不正确，未找到people数组</div>";
            }
            
            content += "</div>";
        } else {
            content = "<div class='alert alert-danger'>JSON解析失败: " + String(error.c_str()) + "</div>";
        }
    } else {
        content = "<div class='alert alert-danger'>无法打开宇航员数据文件</div>";
    }
    
    String html = generateScreenPage("astronauts", "太空站宇航员屏幕", content);
    server.send(200, "text/html", html);
}

/**
 * 检查服务器是否正在运行
 */
bool WebConfigServer::isServerRunning() {
    return isRunning;
}

/**
 * 处理主页请求
 */
void WebConfigServer::handleRoot() {
    // 检查当前是否为AP模式（配置模式）
    bool isAPMode = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA);
    
    String ssid, password;
    int timezone;
    
    // 读取当前的note内容
    String noteContent = "";
    File noteFile = SPIFFS.open("/note.json", "r");
    if (noteFile) {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, noteFile);
        noteFile.close();
        
        if (!error && doc.containsKey("note")) {
            noteContent = doc["note"].as<String>();
        }
    }
    
    String html = "";
    // 创建带Bootstrap的HTML页面
    html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ESP32信息板</title>";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css\" rel=\"stylesheet\">";
    html += "<style>body { padding-top: 20px; } .container { max-width: 900px; }</style>";
    html += "</head><body>";
    
    // 添加导航栏
    html += "<div class=\"container\">";
    html += "<style>body { background-color: #f8f9fa; } .btn-primary { background-color: #4CAF50; border-color: #4CAF50; }</style>";
    html += "<nav class=\"navbar navbar-expand-lg mb-4 shadow-lg bg-info\">";
    html += "  <div class=\"container-fluid\">";
    html += "    <a class=\"navbar-brand text-white font-weight-bold\" href=\"/\">ESP32信息板</a>";
    html += "    <button class=\"navbar-toggler\" type=\"button\" data-bs-toggle=\"collapse\" data-bs-target=\"#navbarNav\" aria-controls=\"navbarNav\" aria-expanded=\"false\" aria-label=\"Toggle navigation\">";
    html += "      <span class=\"navbar-toggler-icon bg-white\"></span>";
    html += "    </button>";
    html += "    <div class=\"collapse navbar-collapse\" id=\"navbarNav\">";
    html += "      <ul class=\"navbar-nav\">";
    html += "        <li class=\"nav-item\"><a class=\"nav-link active text-white font-weight-bold\" href=\"/\">配置</a></li>";
    html += "        <li class=\"nav-item\"><a class=\"nav-link text-white\" href=\"/json-files\">JSON文件</a></li>";
    html += "        <li class=\"nav-item\"><a class=\"nav-link text-white\" href=\"/screen-news\">新闻</a></li>";
    html += "        <li class=\"nav-item\"><a class=\"nav-link text-white\" href=\"/screen-calendar\">日历</a></li>";
    html += "        <li class=\"nav-item\"><a class=\"nav-link text-white\" href=\"/screen-notes\">留言板</a></li>";
    html += "        <li class=\"nav-item\"><a class=\"nav-link text-white\" href=\"/screen-iciba\">每日一句</a></li>";
    html += "        <li class=\"nav-item\"><a class=\"nav-link text-white\" href=\"/screen-astronauts\">太空站宇航员</a></li>";
    html += "      </ul>";
    html += "    </div>";
    html += "  </div>";
    html += "</nav>";
    
    // 显示当前状态信息
    html += "<div class=\"card mb-4 shadow-sm rounded-lg\">";
    html += "  <div class=\"card-header bg-success text-white\">系统状态</div>";
    html += "  <div class=\"card-body\">";
    if (isAPMode) {
        html += "    <p class=\"text-danger\">当前为配置模式 (AP模式)</p>";
        html += "    <p>接入点IP: " + WiFi.softAPIP().toString() + "</p>";
    } else {
        html += "    <p class=\"text-success\">当前为联网模式 (STA模式)</p>";
        html += "    <p>设备IP: " + WiFi.localIP().toString() + "</p>";
        html += "    <p>WiFi信号强度: " + String(WiFi.RSSI()) + " dBm</p>";
    }
    html += "    <p>软件版本: " SOFTWARE_VERSION "</p>";
    html += "  </div>";
    html += "</div>";
    
    if (isAPMode) {
        // 配置模式 - 显示完整配置页面
        readWiFiConfig(ssid, password);
        getNTPServerTimezone(timezone);
        
        html += "<div class=\"card mb-4 shadow-sm rounded-lg\">";
        html += "  <div class=\"card-header bg-primary text-white\">网络配置</div>";
        html += "  <div class=\"card-body\">";
        html += "    <form action='/config' method='post'>";
        html += "      <div class=\"mb-3\">";
        html += "        <label for=\"ssid\" class=\"form-label\">WiFi名称</label>";
        html += "        <input type=\"text\" class=\"form-control\" id=\"ssid\" name=\"ssid\" value='" + ssid + "'>";
        html += "      </div>";
        html += "      <div class=\"mb-3\">";
        html += "        <label for=\"password\" class=\"form-label\">WiFi密码</label>";
        html += "        <input type=\"password\" class=\"form-control\" id=\"password\" name=\"password\" value='" + password + "'>";
        html += "      </div>";
        html += "      <div class=\"mb-3\">";
        html += "        <label for=\"timezone\" class=\"form-label\">NTP时区</label>";
        html += "        <input type=\"number\" class=\"form-control\" id=\"timezone\" name=\"timezone\" value='" + String(timezone) + "' min='-12' max='14'>";
        html += "        <div class=\"form-text\">整数，如北京时间为8</div>";
        html += "      </div>";
        

        
        html += "      <button type=\"submit\" class=\"btn btn-primary\">保存配置</button>";
        html += "    </form>";
        html += "  </div>";
        html += "</div>";
        
        // 系统操作按钮
        html += "<div class=\"card mb-4 shadow-sm rounded-lg\">";
        html += "  <div class=\"card-header bg-warning text-dark\">系统操作</div>";
        html += "  <div class=\"card-body\">";
        html += "    <form action='/restart' method='post'>";
        html += "      <button type=\"submit\" class=\"btn btn-warning\">重启系统</button>";
        html += "    </form>";
        html += "  </div>";
        html += "</div>";
    }
    
    // 主题选择功能
    html += "<div class=\"card mb-4 shadow-sm rounded-lg\">";
    html += "  <div  class=\"card-header bg-success text-white\">显示主题配置</div>";
    html += "  <div class=\"card-body\">";
    html += "    <form action='/theme' method='post'>";
    html += "      <div class=\"mb-3\">";
    html += "        <label for=\"theme\" class=\"form-label\">选择显示主题</label>";
    html += "        <select class=\"form-control\" id=\"theme\" name=\"theme\">";
    html += "          <option value=\"dark\"";
    // 获取主题配置
    bool isLightTheme = ConfigManager::getInstance()->getDisplayTheme();
if (!isLightTheme) html += " selected";
    html += ">黑夜主题</option>";
    html += "          <option value=\"light\"";
    if (isLightTheme) html += " selected";
    html += ">白天主题</option>";
    html += "        </select>";
    html += "        <div class=\"form-text\">白天主题：白底黑字；黑夜主题：黑底白字</div>";
    html += "      </div>";
    html += "      <button type=\"submit\" class=\"btn btn-primary\">保存主题</button>";
    html += "    </form>";
    html += "  </div>";
    html += "</div>";
    
    // 留言板功能
    html += "<div class=\"card mb-4 shadow-sm rounded-lg\">";
    html += "  <div  class=\"card-header bg-success text-white\">留言板内容配置</div>";
    html += "  <div class=\"card-body\">";
    html += "    <form action='/note' method='post'>";
    html += "      <div class=\"mb-3\">";
    html += "        <label for=\"note\" class=\"form-label\">留言内容</label>";
    html += "        <textarea class=\"form-control\" id=\"note\" name=\"note\" rows=\"6\">" + noteContent + "</textarea>";
    html += "      </div>";
    html += "      <button type=\"submit\" class=\"btn btn-secondary\">保存留言内容</button>";
    html += "    </form>";
    html += "  </div>";
    html += "</div>";
    
    // 底部版权信息
    html += "<footer class=\"text-center text-muted mt-5 pt-4 border-top\">";
    html += "  <div class=\"container\">";
    html += "    <div class=\"row\">";
    html += "      <div class=\"col-md-6\">";
    html += "        <h5 class=\"text-success\">ESP32桌面信息板</h5>";
    html += "        <p>基于ESP32的多功能信息显示系统，支持名言警句等内容展示，以及自动换屏等功能。</p>";
    html += "      </div>";
    html += "      <div class=\"col-md-6\">";
    html += "        <p>作者：wangkf</p>";
    html += "        <p><a href=\"https://github.com/wangkf/Esp32DesktopInfoBoard\" class=\"text-success\">github.com/wangkf/Esp32DesktopInfoBoard</a></p>";
    html += "      </div>";
    html += "    </div>";
    html += "  </div>";
    html += "</footer>";
    
    html += "<script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js\"></script>";
    html += "</div>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

/**
 * 处理留言板内容请求
 */
void WebConfigServer::handleNote() {
    if (server.hasArg("note")) {
        String noteContent = server.arg("note");
        
        // 创建JSON文档
        DynamicJsonDocument doc(1024);
        doc["note"] = noteContent;
        doc["update_time"] = millis(); // 保存时间戳
        
        // 打开文件进行写入
        File noteFile = SPIFFS.open("/note.json", "w");
        if (noteFile) {
            // 序列化JSON到文件
            serializeJson(doc, noteFile);
            noteFile.close();
            
            Serial.println("留言内容保存成功");
            String successHtml = "";
            successHtml += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>操作成功 - ESP32信息板</title>";
            successHtml += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
            successHtml += "<style>body { background-color: #f8f9fa; padding-top: 20px; } .container { max-width: 800px; } .card { border: none; box-shadow: 0 4px 8px rgba(0,0,0,0.1); border-radius: 10px; }</style>";
            successHtml += "</head><body>";
            
            // 统一的导航栏样式（亮色主题）
            successHtml += "<div class='container'>";
            successHtml += "<nav class='navbar navbar-expand-lg navbar-light bg-light mb-4'>";
            successHtml += "  <div class='container-fluid'>";
            successHtml += "    <a class='navbar-brand text-success' href='/'>ESP32信息板</a>";
            successHtml += "  </div>";
            successHtml += "</nav>";
            
            // 内容区域
            successHtml += "<div class='card mb-4'>";
            successHtml += "  <div class='card-header bg-success text-white'>操作成功</div>";
            successHtml += "  <div class='card-body'>";
            successHtml += "    <h2 class='text-success'>留言内容保存成功!</h2>";
            successHtml += "    <p>下次切换屏幕时将显示新的留言内容。</p>";
            successHtml += "    <a href='/' class='btn btn-success'>返回首页</a>";
            successHtml += "  </div>";
            successHtml += "</div>";
            
            // 底部版权信息
            successHtml += "<footer class='text-center text-muted'>";
            successHtml += "  <p>ESP32桌面信息板 - 基于ESP32的多功能信息显示系统</p>";
            successHtml += "  <p><a href='https://github.com/wangkf/Esp32DesktopInfoBoard' class='text-success'>github.com/wangkf/Esp32DesktopInfoBoard</a></p>";
            successHtml += "</footer>";
            
            successHtml += "</div>";
            successHtml += "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>";
            successHtml += "</body></html>";
            
            server.send(200, "text/html", successHtml);
        } else {
            Serial.println("无法创建或打开note.json文件");
            String errorHtml = "";
            errorHtml += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>操作失败 - ESP32信息板</title>";
            errorHtml += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
            errorHtml += "<style>body { background-color: #f8f9fa; padding-top: 20px; } .container { max-width: 800px; } .card { border: none; box-shadow: 0 4px 8px rgba(0,0,0,0.1); border-radius: 10px; }</style>";
            errorHtml += "</head><body>";
            
            // 统一的导航栏样式（亮色主题）
            errorHtml += "<div class='container'>";
            errorHtml += "<nav class='navbar navbar-expand-lg bg-info mb-4'>";
            errorHtml += "  <div class='container-fluid'>";
            errorHtml += "    <a class='navbar-brand text-white' href='/'>ESP32信息板</a>";
            errorHtml += "  </div>";
            errorHtml += "</nav>";
            
            // 内容区域
            errorHtml += "<div class='card mb-4'>";
            errorHtml += "  <div class='card-header bg-danger text-white'>操作失败</div>";
            errorHtml += "  <div class='card-body'>";
            errorHtml += "    <h2 class='text-danger'>留言内容保存失败!</h2>";
            errorHtml += "    <p>无法创建或打开note.json文件</p>";
            errorHtml += "    <a href='/' class='btn btn-success'>返回首页</a>";
            errorHtml += "  </div>";
            errorHtml += "</div>";
            
            // 底部版权信息
            errorHtml += "<footer class='text-center text-muted'>";
            errorHtml += "  <p>ESP32桌面信息板 - 基于ESP32的多功能信息显示系统</p>";
            errorHtml += "  <p><a href='https://github.com/wangkf/Esp32DesktopInfoBoard' class='text-success'>github.com/wangkf/Esp32DesktopInfoBoard</a></p>";
            errorHtml += "</footer>";
            
            errorHtml += "</div>";
            errorHtml += "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>";
            errorHtml += "</body></html>";
            
            server.send(500, "text/html", errorHtml);
        }
    } else {
        String errorHtml = "";
        errorHtml += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>参数错误 - ESP32信息板</title>";
        errorHtml += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
        errorHtml += "<style>body { background-color: #f8f9fa; padding-top: 20px; } .container { max-width: 800px; } .card { border: none; box-shadow: 0 4px 8px rgba(0,0,0,0.1); border-radius: 10px; }</style>";
        errorHtml += "</head><body>";
        
        // 统一的导航栏样式（亮色主题）
        errorHtml += "<div class='container'>";
        errorHtml += "<nav class='navbar navbar-expand-lg navbar-light bg-light mb-4'>";
        errorHtml += "  <div class='container-fluid'>";
        errorHtml += "    <a class='navbar-brand text-success' href='/'>ESP32信息板</a>";
        errorHtml += "  </div>";
        errorHtml += "</nav>";
        
        // 内容区域
        errorHtml += "<div class='card mb-4'>";
        errorHtml += "  <div class='card-header bg-warning text-white'>参数错误</div>";
        errorHtml += "  <div class='card-body'>";
        errorHtml += "    <h2 class='text-warning'>参数错误!</h2>";
        errorHtml += "    <p>缺少note参数</p>";
        errorHtml += "    <a href='/' class='btn btn-success'>返回首页</a>";
        errorHtml += "  </div>";
        errorHtml += "</div>";
        
        // 底部版权信息
        errorHtml += "<footer class='text-center text-muted'>";
        errorHtml += "  <p>ESP32桌面信息板 - 基于ESP32的多功能信息显示系统</p>";
        errorHtml += "  <p><a href='https://github.com/wangkf/Esp32DesktopInfoBoard' class='text-success'>github.com/wangkf/Esp32DesktopInfoBoard</a></p>";
        errorHtml += "</footer>";
        
        errorHtml += "</div>";
        errorHtml += "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>";
        errorHtml += "</body></html>";
        
        server.send(400, "text/html", errorHtml);
    }
}

/**
 * 处理配置请求（合并后的WiFi和时区配置）
 */
void WebConfigServer::handleConfig() {
    if (server.hasArg("ssid") && server.hasArg("password") && server.hasArg("timezone") && server.hasArg("theme")) {
        String ssid = server.arg("ssid");
        String password = server.arg("password");
        int timezone = server.arg("timezone").toInt();
        String theme = server.arg("theme");
        bool isLightTheme = (theme == "light");
        
        // 使用ConfigManager保存所有配置
        ConfigManager* configManager = ConfigManager::getInstance();
        if (configManager->isConfigLoaded()) {
            bool wifiSaved = configManager->setWiFiConfig(ssid, password);
            bool timezoneSaved = configManager->setNTPServerTimezone(timezone);
            bool themeSaved = configManager->setDisplayTheme(isLightTheme);
            
            if (wifiSaved && timezoneSaved && themeSaved) {
                Serial.println("配置保存成功");
                String successHtml = "";
                successHtml += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>配置成功 - ESP32信息板</title>";
                successHtml += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
                successHtml += "<style>body { background-color: #f8f9fa; padding-top: 20px; } .container { max-width: 800px; } .card { border: none; box-shadow: 0 4px 8px rgba(0,0,0,0.1); border-radius: 10px; }</style>";
                successHtml += "</head><body>";
                
                // 统一的导航栏样式（亮色主题）
                successHtml += "<div class='container'>";
                successHtml += "<nav class='navbar navbar-expand-lg bg-info mb-4'>";
                successHtml += "  <div class='container-fluid'>";
                successHtml += "    <a class='navbar-brand text-white' href='/'>ESP32信息板</a>";
                successHtml += "  </div>";
                successHtml += "</nav>";
                
                // 内容区域
                successHtml += "<div class='card mb-4'>";
                successHtml += "  <div class='card-header bg-success text-white'>配置成功</div>";
                successHtml += "  <div class='card-body'>";
                successHtml += "    <h2 class='text-success'>配置保存成功!</h2>";
                successHtml += "    <p>重启设备后生效。</p>";
                successHtml += "    <a href='/' class='btn btn-success'>返回首页</a>";
                successHtml += "  </div>";
                successHtml += "</div>";
                
                // 底部版权信息
                successHtml += "<footer class='text-center text-muted'>";
                successHtml += "  <p>ESP32桌面信息板 - 基于ESP32的多功能信息显示系统</p>";
                successHtml += "  <p><a href='https://github.com/wangkf/Esp32DesktopInfoBoard' class='text-success'>github.com/wangkf/Esp32DesktopInfoBoard</a></p>";
                successHtml += "</footer>";
                
                successHtml += "</div>";
                successHtml += "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>";
                successHtml += "</body></html>";
                
                server.send(200, "text/html", successHtml);
            } else {
                Serial.println("配置保存失败");
                String errorHtml = "";
                errorHtml += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>配置失败 - ESP32信息板</title>";
                errorHtml += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
                errorHtml += "<style>body { background-color: #f8f9fa; padding-top: 20px; } .container { max-width: 800px; } .card { border: none; box-shadow: 0 4px 8px rgba(0,0,0,0.1); border-radius: 10px; }</style>";
                errorHtml += "</head><body>";
                
                // 统一的导航栏样式（亮色主题）
                errorHtml += "<div class='container'>";
                errorHtml += "<nav class='navbar navbar-expand-lg bg-info mb-4'>";
                errorHtml += "  <div class='container-fluid'>";
                errorHtml += "    <a class='navbar-brand text-white' href='/'>ESP32信息板</a>";
                errorHtml += "  </div>";
                errorHtml += "</nav>";
                
                // 内容区域
                errorHtml += "<div class='card mb-4'>";
                errorHtml += "  <div class='card-header bg-danger text-white'>配置失败</div>";
                errorHtml += "  <div class='card-body'>";
                errorHtml += "    <h2 class='text-danger'>配置保存失败!</h2>";
                errorHtml += "    <a href='/' class='btn btn-success'>返回首页</a>";
                errorHtml += "  </div>";
                errorHtml += "</div>";
                
                // 底部版权信息
                errorHtml += "<footer class='text-center text-muted'>";
                errorHtml += "  <p>ESP32桌面信息板 - 基于ESP32的多功能信息显示系统</p>";
                errorHtml += "  <p><a href='https://github.com/wangkf/Esp32DesktopInfoBoard' class='text-success'>github.com/wangkf/Esp32DesktopInfoBoard</a></p>";
                errorHtml += "</footer>";
                
                errorHtml += "</div>";
                errorHtml += "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>";
                errorHtml += "</body></html>";
                
                server.send(500, "text/html", errorHtml);
            }
        } else {
            Serial.println("配置管理器未初始化，无法保存配置");
            String errorHtml = "";
            errorHtml += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>配置失败 - ESP32信息板</title>";
            errorHtml += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
            errorHtml += "<style>body { background-color: #f8f9fa; padding-top: 20px; } .container { max-width: 800px; } .card { border: none; box-shadow: 0 4px 8px rgba(0,0,0,0.1); border-radius: 10px; }</style>";
            errorHtml += "</head><body>";
            
            // 统一的导航栏样式（亮色主题）
            errorHtml += "<div class='container'>";
            errorHtml += "<nav class='navbar navbar-expand-lg navbar-light bg-light mb-4'>";
            errorHtml += "  <div class='container-fluid'>";
            errorHtml += "    <a class='navbar-brand text-success' href='/'>ESP32信息板</a>";
            errorHtml += "  </div>";
            errorHtml += "</nav>";
            
            // 内容区域
            errorHtml += "<div class='card mb-4'>";
            errorHtml += "  <div class='card-header bg-danger text-white'>配置失败</div>";
            errorHtml += "  <div class='card-body'>";
            errorHtml += "    <h2 class='text-danger'>配置保存失败!</h2>";
            errorHtml += "    <a href='/' class='btn btn-success'>返回首页</a>";
            errorHtml += "  </div>";
            errorHtml += "</div>";
            
            // 底部版权信息
            errorHtml += "<footer class='text-center text-muted'>";
            errorHtml += "  <p>ESP32桌面信息板 - 基于ESP32的多功能信息显示系统</p>";
            errorHtml += "  <p><a href='https://github.com/wangkf/Esp32DesktopInfoBoard' class='text-success'>github.com/wangkf/Esp32DesktopInfoBoard</a></p>";
            errorHtml += "</footer>";
            
            errorHtml += "</div>";
            errorHtml += "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>";
            errorHtml += "</body></html>";
            
            server.send(500, "text/html", errorHtml);
        }
    } else {
        String errorHtml = "";
        errorHtml += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>参数错误 - ESP32信息板</title>";
        errorHtml += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
        errorHtml += "<style>body { background-color: #f8f9fa; padding-top: 20px; } .container { max-width: 800px; } .card { border: none; box-shadow: 0 4px 8px rgba(0,0,0,0.1); border-radius: 10px; }</style>";
        errorHtml += "</head><body>";
        
        // 统一的导航栏样式（亮色主题）
        errorHtml += "<div class='container'>";
        errorHtml += "<nav class='navbar navbar-expand-lg navbar-light bg-light mb-4'>";
        errorHtml += "  <div class='container-fluid'>";
        errorHtml += "    <a class='navbar-brand text-success' href='/'>ESP32信息板</a>";
        errorHtml += "  </div>";
        errorHtml += "</nav>";
        
        // 内容区域
        errorHtml += "<div class='card mb-4'>";
        errorHtml += "  <div class='card-header bg-warning text-white'>参数错误</div>";
        errorHtml += "  <div class='card-body'>";
        errorHtml += "    <h2 class='text-warning'>参数错误!</h2>";
        errorHtml += "    <a href='/' class='btn btn-success'>返回首页</a>";
        errorHtml += "  </div>";
        errorHtml += "</div>";
        
        // 底部版权信息
        errorHtml += "<footer class='text-center text-muted'>";
        errorHtml += "  <p>ESP32桌面信息板 - 基于ESP32的多功能信息显示系统</p>";
        errorHtml += "  <p><a href='https://github.com/wangkf/Esp32DesktopInfoBoard' class='text-success'>github.com/wangkf/Esp32DesktopInfoBoard</a></p>";
        errorHtml += "</footer>";
        
        errorHtml += "</div>";
        errorHtml += "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>";
        errorHtml += "</body></html>";
        
        server.send(400, "text/html", errorHtml);
    }
}

/**
 * 处理JSON文件查看请求
 */
void WebConfigServer::handleJsonFile() {
    String jsonFilesList = getJsonFilesList();
    
    String html = "";
    // 创建带Bootstrap的HTML页面
    html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>JSON文件列表 - ESP32信息板</title>";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css\" rel=\"stylesheet\">";
    html += "<style>body { background-color: #f8f9fa; padding-top: 20px; } .container { max-width: 900px; } pre { background-color: #f8f9fa; } .card { border: none; box-shadow: 0 4px 8px rgba(0,0,0,0.1); border-radius: 10px; }</style>";
    html += "</head><body>";
    
    // 添加导航栏（亮色主题）
    html += "<div class=\"container\">";
    html += "<nav class=\"navbar navbar-expand-lg bg-info mb-4\">";
    html += "  <div class=\"container-fluid\">";
    html += "    <a class=\"navbar-brand text-white\" href=\"/\">ESP32信息板</a>";
    html += "    <button class=\"navbar-toggler\" type=\"button\" data-bs-toggle=\"collapse\" data-bs-target=\"#navbarNav\" aria-controls=\"navbarNav\" aria-expanded=\"false\" aria-label=\"Toggle navigation\">";
    html += "      <span class=\"navbar-toggler-icon\"></span>";
    html += "    </button>";
    html += "    <div class=\"collapse navbar-collapse\" id=\"navbarNav\">";
    html += "      <ul class=\"navbar-nav\">";
    html += "        <li class=\"nav-item\"><a class=\"nav-link\" href=\"/\">配置</a></li>";
    html += "        <li class=\"nav-item\"><a class=\"nav-link\" href=\"/screen-news\">新闻</a></li>";
    html += "        <li class=\"nav-item\"><a class=\"nav-link\" href=\"/screen-calendar\">日历</a></li>";
    html += "        <li class=\"nav-item\"><a class=\"nav-link\" href=\"/screen-notes\">留言板</a></li>";
    html += "        <li class=\"nav-item\"><a class=\"nav-link\" href=\"/screen-iciba\">每日一句</a></li>";
    html += "        <li class=\"nav-item\"><a class=\"nav-link\" href=\"/screen-astronauts\">太空站宇航员</a></li>";
    html += "        <li class=\"nav-item\"><a class=\"nav-link text-success font-weight-bold\" href=\"/json-files\">JSON文件</a></li>";
    html += "      </ul>";
    html += "    </div>";
    html += "  </div>";
    html += "</nav>";
    
    html += "<div class=\"card mb-4\">";
    html += "  <div class=\"card-header bg-success text-white\">JSON文件列表</div>";
    html += "  <div class=\"card-body\">";
    html += "    <div class=\"list-group\">";
    html += jsonFilesList;
    html += "    </div>";
    html += "  </div>";
    html += "</div>";
    
    html += "<div class=\"card\">";
    html += "  <div class=\"card-body text-center\">";
    html += "    <a href=\"/\" class=\"btn btn-success\">返回首页</a>";
    html += "  </div>";
    html += "</div>";
    
    // 底部版权信息
    html += "<footer class=\"text-center text-muted mt-4\">";
    html += "  <p>ESP32桌面信息板 - 基于ESP32的多功能信息显示系统</p>";
    html += "  <p><a href=\"https://github.com/wangkf/Esp32DesktopInfoBoard\" class=\"text-success\">github.com/wangkf/Esp32DesktopInfoBoard</a></p>";
    html += "</footer>";
    
    html += "<script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js\"></script>";
    html += "</div>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

/**
 * 处理404错误
 */
void WebConfigServer::handleNotFound() {
    String html = "";
    html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>页面不存在 - ESP32信息板</title>";
    html += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
    html += "<style>body { background-color: #f8f9fa; padding-top: 20px; } .container { max-width: 800px; } .card { border: none; box-shadow: 0 4px 8px rgba(0,0,0,0.1); border-radius: 10px; }</style>";
    html += "</head><body>";
    
    // 统一的导航栏样式（亮色主题）
    html += "<div class=\"container\">";
    html += "<nav class=\"navbar navbar-expand-lg navbar-light bg-light mb-4\">";
    html += "  <div class=\"container-fluid\">";
    html += "    <a class=\"navbar-brand text-success\" href=\"/\">ESP32信息板</a>";
    html += "  </div>";
    html += "</nav>";
    
    // 内容区域
    html += "<div class=\"card mb-4\">";
    html += "  <div class=\"card-header bg-danger text-white\">页面不存在</div>";
    html += "  <div class=\"card-body\">";
    html += "    <h1 class='text-danger'>404 - 页面不存在</h1>";
    html += "    <p>您请求的页面不存在或已被移除</p>";
    html += "    <a href=\"/\" class=\"btn btn-success\">返回首页</a>";
    html += "  </div>";
    html += "</div>";
    
    // 底部版权信息
    html += "<footer class=\"text-center text-muted\">";
    html += "  <p>ESP32桌面信息板 - 基于ESP32的多功能信息显示系统</p>";
    html += "  <p><a href='https://github.com/wangkf/Esp32DesktopInfoBoard' class='text-success'>github.com/wangkf/Esp32DesktopInfoBoard</a></p>";
    html += "</footer>";
    
    html += "</div>";
    html += "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>";
    html += "</body></html>";
    
    server.send(404, "text/html", html);
}

/**
 * 读取WiFi配置
 */
void WebConfigServer::readWiFiConfig(String& ssid, String& password) {
    // 使用ConfigManager获取WiFi配置
    ConfigManager* configManager = ConfigManager::getInstance();
    if (configManager->isConfigLoaded()) {
        if (!configManager->getWiFiConfig(ssid, password)) {
            // 如果无法从配置管理器获取配置，使用默认值
            Serial.println("无法从配置管理器获取WiFi配置，使用默认值");
            ssid = "Mywifi";
            password = "12345678";
        }
    } else {
        // 如果配置管理器未初始化，使用默认值
        Serial.println("配置管理器未初始化，使用默认WiFi配置");
        ssid = "Mywifi";
        password = "12345678";
    }
}

/**
 * 保存WiFi配置
 */
bool WebConfigServer::saveWiFiConfig(const String& ssid, const String& password) {
    // 使用ConfigManager保存WiFi配置
    ConfigManager* configManager = ConfigManager::getInstance();
    if (configManager->isConfigLoaded()) {
        if (configManager->setWiFiConfig(ssid, password)) {
            Serial.println("WiFi配置保存成功");
            return true;
        } else {
            Serial.println("WiFi配置保存失败");
            return false;
        }
    } else {
        Serial.println("配置管理器未初始化，无法保存WiFi配置");
        return false;
    }
}

/**
 * 获取所有JSON文件列表
 */
String WebConfigServer::getJsonFilesList() {
    String result = "";
    
    // 打开根目录
    File root = SPIFFS.open("/");
    if (!root) {
        return "<div class=\"alert alert-danger\">无法打开文件系统</div>";
    }
    
    // 读取文件列表
    File file = root.openNextFile();
    bool hasFiles = false;
    
    while (file) {
        String fileName = file.name();
        if (fileName.endsWith(".json")) {
            hasFiles = true;
            // 为每个文件创建一个可点击的列表项，使用Bootstrap样式
            result += "<a href='/json-file?name=" + urlEncode(fileName) + "' class=\"list-group-item list-group-item-action\">" + fileName + "</a>";
        }
        file = root.openNextFile();
    }
    
    if (!hasFiles) {
        result = "<div class=\"alert alert-info\">当前没有JSON文件</div>";
    }
    
    return result;
}

/**
 * URL编码函数
 */
String WebConfigServer::urlEncode(const String& str) {
    String encodedString = "";
    char c;
    char code0;
    char code1;
    char code2;
    
    for (unsigned int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (c == ' ') {
            encodedString += '+';
        } else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encodedString += c;
        } else {
            code1 = (c >> 4) & 0xf;
            code2 = (c & 0xf);
            code0 = 0x25; // '%'
            encodedString += code0;
            encodedString += (code1 < 10) ? ('0' + code1) : ('A' + (code1 - 10));
            encodedString += (code2 < 10) ? ('0' + code2) : ('A' + (code2 - 10));
        }
    }
    return encodedString;
}

/**
 * 处理单个JSON文件内容请求
 */
void WebConfigServer::handleJsonFileContent() {
    if (server.hasArg("name")) {
        String fileName = server.arg("name");
        String fileContent = readJsonFileContent(fileName);
        
        String html = "";
        html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>查看JSON文件: " + fileName + "</title>";
        html += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
        html += "</head><body>";
        
        // 统一的导航栏样式
        html += "<div class=\"container\">";
        html += "<nav class=\"navbar navbar-expand-lg navbar-dark bg-dark mb-4\">";
        html += "  <div class=\"container-fluid\">";
        html += "    <a class=\"navbar-brand\" href=\"/\">ESP32信息板</a>";
        html += "    <button class=\"navbar-toggler\" type=\"button\" data-bs-toggle=\"collapse\" data-bs-target=\"#navbarNav\" aria-controls=\"navbarNav\" aria-expanded=\"false\" aria-label=\"Toggle navigation\">";
        html += "      <span class=\"navbar-toggler-icon\"></span>";
        html += "    </button>";
        html += "    <div class=\"collapse navbar-collapse\" id=\"navbarNav\">";
        html += "      <ul class=\"navbar-nav\">";
        html += "        <li class=\"nav-item\"><a class=\"nav-link\" href=\"/\">首页</a></li>";
        html += "        <li class=\"nav-item\"><a class=\"nav-link active\" href=\"/json-files\">JSON文件</a></li>";
        html += "        <li class=\"nav-item\"><a class=\"nav-link\" href=\"/screen-news\">新闻</a></li>";
        html += "        <li class=\"nav-item\"><a class=\"nav-link\" href=\"/screen-calendar\">日历</a></li>";
        html += "        <li class=\"nav-item\"><a class=\"nav-link\" href=\"/screen-notes\">留言板</a></li>";
        html += "        <li class=\"nav-item\"><a class=\"nav-link\" href=\"/screen-iciba\">每日一句</a></li>";
        html += "        <li class=\"nav-item\"><a class=\"nav-link\" href=\"/screen-astronauts\">太空站宇航员</a></li>";
        html += "      </ul>";
        html += "    </div>";
        html += "  </div>";
        html += "</nav>";
        
        html += "<div class=\"card mb-4\">";
        html += "  <div class=\"card-header bg-info text-white\">JSON文件内容: " + fileName + "</div>";
        html += "  <div class=\"card-body\">";
        html += "    <pre class=\"bg-light p-4 rounded overflow-auto\" style=\"font-family: monospace; font-size: 14px; line-height: 1.5; max-height: 500px;\">" + fileContent + "</pre>";
        html += "  </div>";
        html += "</div>";
        
        html += "<div class=\"card mb-4\">";
        html += "  <div class=\"card-body text-center\">";
        html += "    <a href=\"/json-files\" class=\"btn btn-secondary\">返回文件列表</a>";
        html += "    <a href=\"/\" class=\"btn btn-primary ms-2\">返回首页</a>";
        html += "  </div>";
        html += "</div>";
        
        html += "<script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js\"></script>";
        html += "</div>";
        html += "</body></html>";
        
        server.send(200, "text/html", html);
    } else {
        String errorHtml = "";
        errorHtml += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'></head>";
        errorHtml += "<body><div class=\"container\"><nav class=\"navbar navbar-expand-lg navbar-dark bg-dark mb-4\"><div class=\"container-fluid\"><a class=\"navbar-brand\" href=\"/\">ESP32信息板</a></div></nav>";
        errorHtml += "<div class=\"card mb-4\"><div class=\"card-header bg-danger text-white\">参数错误</div><div class=\"card-body\"><h1>缺少必要参数</h1><a href=\"/json-files\" class=\"btn btn-secondary\">返回文件列表</a></div></div></div></body></html>";
        server.send(400, "text/html", errorHtml);
    }
}

/**
 * 读取JSON文件内容
 */
String WebConfigServer::readJsonFileContent(const String& fileName) {
    String content = "";
    
    // 尝试多种路径格式打开文件
    String filePath = "/" + fileName;
    File file = SPIFFS.open(filePath, "r");
    
    // 如果直接打开失败，尝试从data目录打开（某些数据可能存放在那里）
    if (!file) {
        filePath = "/data/" + fileName;
        file = SPIFFS.open(filePath, "r");
    }
    
    if (file) {
        content = file.readString();
        file.close();
        
        // 检查文件大小，如果过大，限制缓冲区大小
        const size_t capacity = min(content.length() * 2, (size_t)8192); // 最大8KB缓冲区
        DynamicJsonDocument doc(capacity);
        DeserializationError error = deserializeJson(doc, content);
        
        if (!error) {
            // 成功解析JSON，格式化输出
            String formattedContent;
            serializeJsonPretty(doc, formattedContent);
            return formattedContent;
        } else {
            // 解析失败，返回原始内容但添加错误信息
            return "JSON解析失败: " + String(error.c_str()) + "\n\n原始内容:\n" + content;
        }
    } else {
        content = "无法打开文件: " + fileName + "\n尝试的路径: " + filePath;
        // 添加文件系统信息以帮助调试
        if (!SPIFFS.begin(true)) {
            content += "\n错误: SPIFFS文件系统未初始化";
        }
        return content;
    }
}



/**
 * 读取系统配置（NTP时区）
 */
void WebConfigServer::getNTPServerTimezone(int& timezone) {
    // 使用ConfigManager获取系统配置
    ConfigManager* configManager = ConfigManager::getInstance();
    if (configManager->isConfigLoaded()) {
        timezone = configManager->getNTPServerTimezone();
    } else {
        // 如果配置管理器未初始化，使用默认值
        Serial.println("配置管理器未初始化，使用默认系统配置");
        timezone = 8; // 默认东八区
    }
}

/**
 * 保存系统配置（NTP时区和API密钥）
 */
bool WebConfigServer::setNTPServerTimezone(int timezone) {
    // 使用ConfigManager保存系统配置
    ConfigManager* configManager = ConfigManager::getInstance();
    if (configManager->isConfigLoaded()) {
        // 使用ConfigManager提供的方法来保存配置
        bool timezoneSaved = configManager->setNTPServerTimezone(timezone);
        
        if (timezoneSaved) {
            Serial.println("系统配置保存成功");
            return true;
        } else {
            Serial.println("系统配置保存失败");
            return false;
        }
    } else {
        Serial.println("配置管理器未初始化，无法保存系统配置");
        return false;
    }
}