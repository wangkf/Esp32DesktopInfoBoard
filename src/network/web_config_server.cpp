#include "web_config_server.h"
#include <ArduinoJson.h>
#include "config/config.h"

// 定义单例实例
WebConfigServer* WebConfigServer::instance = nullptr;

/**
 * 处理系统重启请求
 */
void WebConfigServer::handleRestart() {
    Serial.println("接收到系统重启请求...");
    
    // 发送重启确认页面
    String html = "";
    html += "<!DOCTYPE html><html><body><meta charset='UTF-8'><h1>系统重启中...</h1>";
    html += "<p>设备将在3秒后重启，请稍候...</p>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
    
    // 延迟一小段时间，让客户端有足够的时间接收响应
    delay(3000);
    
    // 重启ESP32
    ESP.restart();
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
    server.on("/config", HTTP_POST, std::bind(&WebConfigServer::handleConfig, this));    server.on("/restart", HTTP_POST, std::bind(&WebConfigServer::handleRestart, this));
    server.on("/restart", HTTP_POST, std::bind(&WebConfigServer::handleRestart, this));
    server.on("/json-files", HTTP_GET, std::bind(&WebConfigServer::handleJsonFile, this));
    server.on("/json-file", HTTP_GET, std::bind(&WebConfigServer::handleJsonFileContent, this));
    server.on("/note", HTTP_POST, std::bind(&WebConfigServer::handleNote, this));
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
    
    if (isAPMode) {
        // 配置模式 - 显示完整配置页面
        readWiFiConfig(ssid, password);
        getNTPServerTimezone(timezone);
        
        html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ESP32信息板配置</title>",
               "<style>body{font-family:Arial,sans-serif;margin:20px;}",
               "h1{color:#333;}",
               "form{max-width:400px;margin:20px 0;}",
               "input[type=text],input[type=password],input[type=number]{width:100%;padding:10px;margin:8px 0;display:inline-block;",
               "border:1px solid #ccc;border-radius:4px;box-sizing:border-box;}",
               "textarea{width:100%;height:200px;padding:10px;margin:8px 0;display:inline-block;",
               "border:1px solid #ccc;border-radius:4px;box-sizing:border-box;resize:vertical;}",
               "input[type=submit]{background-color:#4CAF50;color:white;padding:14px 20px;margin:8px 0;border:none;",
               "border-radius:4px;cursor:pointer;}",
               "input[type=submit]:hover{background-color:#45a049;}",
               "a{color:#0066cc;}</style></head><body>";
        
        html += "<h1>ESP32信息板配置</h1>";
        
        // 合并后的配置表单
        html += "<form action='/config' method='post'>";
        html += "WiFi名称: <input type='text' name='ssid' value='" + ssid + "'><br>";
        html += "WiFi密码: <input type='password' name='password' value='" + password + "'><br>";
        html += "NTP时区: <input type='number' name='timezone' value='" + String(timezone) + "'>(整数，如北京时间为8)<br>";
        html += "<input type='submit' value='保存配置'>";
        html += "</form>";
        
        // 添加重启系统按钮
        html += "<form action='/restart' method='post'>";
        html += "<input type='submit' value='重启系统'>";
        html += "</form>";
        
        html += "<h2>JSON文件查看</h2>";
        html += "<p><a href='/json-files'>查看所有JSON文件</a></p>";
    } else {
        // 普通模式（已联网）- 显示简化页面
        html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ESP32信息板</title>",
               "<style>body{font-family:Arial,sans-serif;margin:20px;}",
               "h1{color:#333;}",
               "form{max-width:400px;margin:20px 0;}",
               "textarea{width:100%;height:200px;padding:10px;margin:8px 0;display:inline-block;",
               "border:1px solid #ccc;border-radius:4px;box-sizing:border-box;resize:vertical;}",
               "input[type=submit]{background-color:#4CAF50;color:white;padding:14px 20px;margin:8px 0;border:none;",
               "border-radius:4px;cursor:pointer;}",
               "input[type=submit]:hover{background-color:#45a049;}",
               "a{color:#0066cc;}</style></head><body>";
        
        html += "<h1>ESP32信息板</h1>";
        html += "<p>设备已成功连接网络，当前IP地址: " + WiFi.localIP().toString() + "</p>";
    }
    
    // 两种模式都显示留言板功能
    html += "<h2>留言板内容配置</h2>";
    html += "<form action='/note' method='post'>";
    html += "留言内容: <br/><textarea name='note'>" + noteContent + "</textarea><br>";
    html += "<input type='submit' value='保存留言内容'>";
    html += "</form>";
    
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
            server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body><h1>留言内容保存成功!</h1><p>下次切换屏幕时将显示新的留言内容。</p><p><a href='/'>返回首页</a></p></body></html>");
        } else {
            Serial.println("无法创建或打开note.json文件");
            server.send(500, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body><h1>留言内容保存失败!</h1><p>无法创建或打开note.json文件</p><p><a href='/'>返回首页</a></p></body></html>");
        }
    } else {
        server.send(400, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body><h1>参数错误!</h1><p>缺少note参数</p><p><a href='/'>返回首页</a></p></body></html>");
    }
}

/**
 * 处理配置请求（合并后的WiFi和时区配置）
 */
void WebConfigServer::handleConfig() {
    if (server.hasArg("ssid") && server.hasArg("password") && server.hasArg("timezone")) {
        String ssid = server.arg("ssid");
        String password = server.arg("password");
        int timezone = server.arg("timezone").toInt();
        
        // 使用ConfigManager保存所有配置
        ConfigManager* configManager = ConfigManager::getInstance();
        if (configManager->isConfigLoaded()) {
            bool wifiSaved = configManager->setWiFiConfig(ssid, password);
            bool timezoneSaved = configManager->setNTPServerTimezone(timezone);
            
            if (wifiSaved && timezoneSaved) {
                Serial.println("配置保存成功");
                server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body><h1>配置保存成功!</h1><p>重启设备后生效。</p><p><a href='/'>返回首页</a></p></body></html>");
            } else {
                Serial.println("配置保存失败");
                server.send(500, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body><h1>配置保存失败!</h1><p><a href='/'>返回首页</a></p></body></html>");
            }
        } else {
            Serial.println("配置管理器未初始化，无法保存配置");
            server.send(500, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body><h1>配置保存失败!</h1><p><a href='/'>返回首页</a></p></body></html>");
        }
    } else {
        server.send(400, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body><h1>参数错误!</h1><p><a href='/'>返回首页</a></p></body></html>");
    }
}

/**
 * 处理JSON文件查看请求
 */
void WebConfigServer::handleJsonFile() {
    String jsonFilesList = getJsonFilesList();
    
    String html = "";
    html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>JSON文件列表</title>";
    html += "<style>body{font-family:Arial,sans-serif;margin:20px;}";
    html += "h1{color:#333;}";
    html += "ul{list-style-type:none;padding:0;}";
    html += "li{margin:10px 0;padding:10px;background-color:#f1f1f1;border-radius:4px;}";
    html += "a{color:#0066cc;text-decoration:none;}";
    html += "a:hover{text-decoration:underline;}";
    html += "pre{background-color:#eee;padding:10px;border-radius:4px;overflow-x:auto;}";
    html += "button{background-color:#4CAF50;color:white;padding:8px 12px;border:none;border-radius:4px;cursor:pointer;}</style></head><body>";
    
    html += "<h1>JSON文件列表</h1>";
    html += jsonFilesList;
    
    html += "<p><a href='/'>返回首页</a></p>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

/**
 * 处理404错误
 */
void WebConfigServer::handleNotFound() {
    String message = "文件未找到: ";
    message += server.uri();
    message += "\n方法: ";
    message += (server.method() == HTTP_GET ? "GET" : "POST");
    message += "\n参数: ";
    message += String(server.args());
    message += "\n";
    
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    
    server.send(404, "text/plain", message);
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
    String result = "<ul>";
    
    // 打开根目录
    File root = SPIFFS.open("/");
    if (!root) {
        return "<p>无法打开文件系统</p>";
    }
    
    // 读取文件列表
    File file = root.openNextFile();
    while (file) {
        String fileName = file.name();
        if (fileName.endsWith(".json")) {
            // 为每个文件创建一个可点击的列表项，点击后通过URL参数请求文件内容
            result += "<li>";
            result += "<a href='/json-file?name=" + urlEncode(fileName) + "'>" + fileName + "</a>";
            result += "</li>";
        }
        file = root.openNextFile();
    }
    
    result += "</ul>";
    
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
        html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>查看JSON文件: " + fileName + "</title>";
        html += "<style>body{font-family:Arial,sans-serif;margin:20px;}";
        html += "h1{color:#333;}";
        html += "pre{background-color:#eee;padding:10px;border-radius:4px;overflow-x:auto;white-space:pre-wrap;word-wrap:break-word;}";
        html += "a{color:#0066cc;text-decoration:none;}";
        html += "a:hover{text-decoration:underline;}";
        html += "</style></head><body>";
        
        html += "<h1>JSON文件内容: " + fileName + "</h1>";
        html += "<pre>" + fileContent + "</pre>";
        html += "<p><a href='/json-files'>返回文件列表</a></p>";
        html += "<p><a href='/'>返回首页</a></p>";
        html += "</body></html>";
        
        server.send(200, "text/html", html);
    } else {
        server.send(400, "text/html", "<!DOCTYPE html><html><body><meta charset='UTF-8'><h1>参数错误!</h1></body></html>");
    }
}

/**
 * 读取JSON文件内容
 */
String WebConfigServer::readJsonFileContent(const String& fileName) {
    String content = "";
    
    File file = SPIFFS.open("/"+fileName, "r");
    if (file) {
        content = file.readString();
        file.close();
        
        // 尝试格式化JSON以提高可读性
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, content);
        if (!error) {
            // 成功解析JSON，可以选择重新格式化输出
            String formattedContent;
            serializeJsonPretty(doc, formattedContent);
            return formattedContent;
        }
        // 如果解析失败，返回原始内容
        return content;
    } else {
        content = "无法打开文件: " + fileName;
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