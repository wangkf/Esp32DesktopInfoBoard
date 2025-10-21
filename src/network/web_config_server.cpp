#include "web_config_server.h"
#include <ArduinoJson.h>
#include "config/config.h"
#include "config/config_manager.h"
#include "images/images.h"
// ESP32系统信息相关头文件
#include <esp_system.h>
#include <esp_partition.h>
#include <WiFi.h>
#include <HTTPClient.h>

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
 * 处理主题设置和Web授权请求
 */
void WebConfigServer::handleTheme() {
    if (server.hasArg("theme")) {
        String theme = server.arg("theme");
        bool isLightTheme = (theme == "light");
        
        // 获取Web授权参数
        String webUsername = server.hasArg("web_username") ? server.arg("web_username") : "admin";
        String webPassword = server.hasArg("web_password") ? server.arg("web_password") : "admin";
        
        // 获取设备名称参数
        String deviceName = server.hasArg("device_name") ? server.arg("device_name") : "esp32-infoboard";
        
        // 使用ConfigManager保存配置
        ConfigManager* configManager = ConfigManager::getInstance();
        if (configManager->isConfigLoaded()) {
            bool themeSaved = configManager->setDisplayTheme(isLightTheme);
            bool authSaved = configManager->setWebAuthConfig(webUsername, webPassword);
            bool deviceNameSaved = configManager->setDeviceName(deviceName);
            
            if (themeSaved && authSaved && deviceNameSaved) {
                Serial.println("主题和Web授权配置保存成功");
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
                successHtml += "    <div class='alert alert-success'>主题和Web授权设置已成功保存！</div>";
                successHtml += "    <div class='text-center mt-4 btn-group' role='group'>";
                successHtml += "      <a href='/' class='btn btn-primary'>返回首页</a>";
                successHtml += "      <form action='/restart' method='post' style='display:inline;'>";
                successHtml += "        <button type='submit' class='btn btn-warning'>重启设备</button>";
                successHtml += "      </form>";
                successHtml += "    </div>";
                successHtml += "  </div>";
                successHtml += "</div>";
                
                successHtml += "</div>";
                successHtml += "</body></html>";
                
                server.send(200, "text/html", successHtml);
            } else {
                Serial.println("配置保存失败");
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
                errorHtml += "    <div class='alert alert-danger'>配置保存失败，请重试！</div>";
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
              Serial.println("配置未加载，无法保存设置");
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
            errorHtml += "    <div class='alert alert-danger'>配置未加载，无法保存设置！</div>";
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
                                    apPassword("12345678") {
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
 * 验证HTTP基本认证
 */
bool WebConfigServer::authenticate() {
    // 检查当前是否为AP模式（配置模式）
    bool isAPMode = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA);
    
    // 在AP模式下，不需要进行Web授权验证，直接返回true
    if (isAPMode) {
        return true;
    }
    
    // 非AP模式下，执行正常的授权验证
    // 获取配置的用户名和密码，默认使用admin
    String username = "admin";
    String password = "admin";
    
    // 获取配置，如果配置存在则更新用户名密码
    bool configExists = ConfigManager::getInstance()->getWebAuthConfig(username, password);
    
    // 确保用户名和密码不为空，如果配置中为空则使用默认值
    if (username.isEmpty()) username = "admin";
    if (password.isEmpty()) password = "admin";
    
    // 检查认证头
    if (!server.hasHeader("Authorization")) {
        server.requestAuthentication();
        return false;
    }
    
    String authHeader = server.header("Authorization");
    
    // 检查是否是Basic认证
    if (authHeader.startsWith("Basic ")) {
        String encoded = authHeader.substring(6); // 移除"Basic "前缀
        
        String decoded = base64Decode(encoded);
        
        // 解码后的格式为 "username:password"
        int separatorPos = decoded.indexOf(':');
        if (separatorPos > 0) {
            String user = decoded.substring(0, separatorPos);
            String pass = decoded.substring(separatorPos + 1);
            
            // 验证用户名和密码
            if (user == username && pass == password) {
                return true;
            } else {
                // 临时添加：如果使用默认凭据admin/admin也允许登录
                if (user == "admin" && pass == "admin") {
                    return true;
                }
            }
        }
    }
    
    server.requestAuthentication();
    return false;
}

/**
 * Base64解码函数 - 更可靠的实现
 */
String WebConfigServer::base64Decode(String encoded) {
    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    // 清理输入：移除空格、换行等非base64字符
    String cleanEncoded;
    for (int i = 0; i < encoded.length(); i++) {
        char ch = encoded[i];
        // 只保留base64字符和填充字符
        if ((ch >= 'A' && ch <= 'Z') || 
            (ch >= 'a' && ch <= 'z') || 
            (ch >= '0' && ch <= '9') || 
            ch == '+' || ch == '/' || ch == '=') {
            cleanEncoded += ch;
        }
    }
    
    // 计算解码后的长度
    int padding = 0;
    if (cleanEncoded.endsWith("==")) padding = 2;
    else if (cleanEncoded.endsWith("=")) padding = 1;
    
    int decodedLength = (cleanEncoded.length() * 3) / 4 - padding;
    char decoded[decodedLength + 1]; // +1 for null terminator
    memset(decoded, 0, decodedLength + 1);
    
    int j = 0;
    for (int i = 0; i < cleanEncoded.length(); i += 4) {
        // 获取4个base64字符对应的索引
        int a = -1, b = -1, c = -1, d = -1;
        
        if (i < cleanEncoded.length() && cleanEncoded.charAt(i) != '=') {
            char ch = cleanEncoded.charAt(i);
            for (int m = 0; m < 64; m++) {
                if (base64_chars[m] == ch) {
                    a = m;
                    break;
                }
            }
        }
        
        if (i + 1 < cleanEncoded.length() && cleanEncoded.charAt(i + 1) != '=') {
            char ch = cleanEncoded.charAt(i + 1);
            for (int m = 0; m < 64; m++) {
                if (base64_chars[m] == ch) {
                    b = m;
                    break;
                }
            }
        }
        
        if (i + 2 < cleanEncoded.length() && cleanEncoded.charAt(i + 2) != '=') {
            char ch = cleanEncoded.charAt(i + 2);
            for (int m = 0; m < 64; m++) {
                if (base64_chars[m] == ch) {
                    c = m;
                    break;
                }
            }
        }
        
        if (i + 3 < cleanEncoded.length() && cleanEncoded.charAt(i + 3) != '=') {
            char ch = cleanEncoded.charAt(i + 3);
            for (int m = 0; m < 64; m++) {
                if (base64_chars[m] == ch) {
                    d = m;
                    break;
                }
            }
        }
        
        // 解码并确保结果是正确的字符
        if (a != -1 && b != -1) {
            decoded[j++] = (char)((a << 2) | (b >> 4));
        }
        if (b != -1 && c != -1 && j < decodedLength) {
            decoded[j++] = (char)((b << 4) | (c >> 2));
        }
        if (c != -1 && d != -1 && j < decodedLength) {
            decoded[j++] = (char)((c << 6) | d);
        }
    }
    
    decoded[decodedLength] = '\0';
    String result = String(decoded);
    
    Serial.print("解码后的字符串长度: ");
    Serial.println(result.length());
    Serial.print("解码后的字符串内容: ");
    Serial.println(result);
    
    return result;
}

/**
 * 初始化Web配置服务器
 */
void WebConfigServer::init() {
    // 注册处理函数 - 添加认证检查
    server.on("/", HTTP_GET, [this]() {
        if (authenticate()) handleRoot();
    });
    // 同时处理GET和POST请求
    server.on("/config", HTTP_GET, [this]() {
        if (authenticate()) handleConfig();
    });
    server.on("/config", HTTP_POST, [this]() {
        if (authenticate()) handleConfig();
    });
    server.on("/restart", HTTP_POST, [this]() {
        if (authenticate()) handleRestart();
    });
    server.on("/json-files", HTTP_GET, [this]() {
        if (authenticate()) handleJsonFile();
    });
    server.on("/json-file", HTTP_GET, [this]() {
        if (authenticate()) handleJsonFileContent();
    });
    server.on("/note", HTTP_POST, [this]() {
        if (authenticate()) handleNote();
    });
    server.on("/theme", HTTP_POST, [this]() {
        if (authenticate()) handleTheme();
    });
    // 添加各屏幕页面的路由
    server.on("/screen-news", HTTP_GET, [this]() {
        if (authenticate()) handleNewsScreen();
    });
    server.on("/screen-calendar", HTTP_GET, [this]() {
        if (authenticate()) handleCalendarScreen();
    });
    server.on("/screen-notes", HTTP_GET, [this]() {
        if (authenticate()) handleNotesScreen();
    });
    server.on("/screen-iciba", HTTP_GET, [this]() {
        if (authenticate()) handleIcibaScreen();
    });
    server.on("/screen-astronauts", HTTP_GET, [this]() {
        if (authenticate()) handleAstronautsScreen();
    });
    // 网址收藏页面
    server.on("/bookmarks", HTTP_GET, [this]() {
        if (authenticate()) handleBookmarks();
    });
    server.on("/bookmarks", HTTP_POST, [this]() {
        if (authenticate()) handleBookmarks();
    });
    // 添加随机内容页面的路由
    server.on("/random-taxicsoul", HTTP_GET, [this]() {
        if (authenticate()) handleRandomToxicSoul();
    });
    server.on("/random-maoselect", HTTP_GET, [this]() {
        if (authenticate()) handleRandomMaoSelect();
    });
    server.on("/random-soul", HTTP_GET, [this]() {
        if (authenticate()) handleRandomSoul();
    });
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
        // 从ConfigManager获取设备名作为AP名称
        ConfigManager* configManager = ConfigManager::getInstance();
        // 注意：apSSID是const char*类型，需要使用c_str()方法转换
        String deviceName = configManager->getDeviceName();
        apSSID = deviceName.c_str();
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
// 生成统一的版权信息，包含ESP32芯片技术参数和实时参数
String WebConfigServer::generateCopyrightInfo() {
    String copyrightHtml = "";
    
    // 版权信息部分
    copyrightHtml += "<footer class='mt-5 py-4 bg-light border-top'>";
    copyrightHtml += "<div class='container'>";
    copyrightHtml += "<div class='row'>";
    copyrightHtml += "<div class='col-md-6'>";
    copyrightHtml += "<p class='text-muted mb-2'><strong>ESP32桌面信息板</strong></p>";
    copyrightHtml += "<p class='text-sm text-muted'>基于ESP32微控制器的多功能信息显示系统，支持实时数据展示、自动换屏、光线感应调节亮度和Web配置等功能。</p>";
    copyrightHtml += "</div>";
    copyrightHtml += "<div class='col-md-6 text-md-end'>";
    copyrightHtml += "<p class='text-muted mb-2'>作者: wangkf /项目<a href='https://github.com/wangkf/Esp32DesktopInfoBoard' target='_blank'>GitHub地址</a></p>";
    copyrightHtml += "<p class='text-muted mb-2'>邮箱：<a href='mailto:wangkf@qq.com'>wangkf@gmail.com</a></p>";
    copyrightHtml += "<p class='text-muted'>业余无线电呼号：BI9ABS QTH：OM44kf</p>";
    copyrightHtml += "</div>";
    copyrightHtml += "</div>";
    copyrightHtml += "</div>";
    
    // 芯片技术参数和实时参数部分
    copyrightHtml += "<div class='container mt-4'>";
    copyrightHtml += "<div class='card bg-secondary bg-opacity-50'>";
    copyrightHtml += "<div class='card-header bg-info text-white'><strong>ESP32芯片信息（调试用）</strong></div>";
    copyrightHtml += "<div class='card-body'>";
    copyrightHtml += "<div class='row'>";
    
    // 芯片信息
    copyrightHtml += "<div class='col-md-4'>";
    copyrightHtml += "<h6 class='text-primary'>芯片信息</h6>";
    copyrightHtml += "<table class='table table-sm table-striped'>";
    
    // 获取芯片信息
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    const char* chip_model = (chip_info.model == CHIP_ESP32) ? "ESP32" : 
                           (chip_info.model == CHIP_ESP32S2) ? "ESP32-S2" :
                           (chip_info.model == CHIP_ESP32S3) ? "ESP32-S3" :
                           (chip_info.model == CHIP_ESP32C3) ? "ESP32-C3" : "Unknown";
    
    copyrightHtml += "<tr><td>芯片型号</td><td>" + String(chip_model) + "</td></tr>";
    copyrightHtml += "<tr><td>核心数量</td><td>" + String(chip_info.cores) + "</td></tr>";
    copyrightHtml += "<tr><td>WiFi</td><td>" + String((chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "支持" : "不支持") + "</td></tr>";
    copyrightHtml += "<tr><td>Bluetooth</td><td>" + String((chip_info.features & CHIP_FEATURE_BT) ? "支持" : "不支持") + "</td></tr>";
    copyrightHtml += "<tr><td>BLE</td><td>" + String((chip_info.features & CHIP_FEATURE_BLE) ? "支持" : "不支持") + "</td></tr>";
    
    // 获取芯片频率
    uint32_t cpu_freq = getCpuFrequencyMhz();
    copyrightHtml += "<tr><td>CPU频率</td><td>" + String(cpu_freq) + " MHz</td></tr>";
    
    // 获取Flash大小
    uint32_t flash_size = ESP.getFlashChipSize();
    copyrightHtml += "<tr><td>Flash大小</td><td>" + String(flash_size / (1024 * 1024)) + " MB</td></tr>";
    
    copyrightHtml += "</table>";
    copyrightHtml += "</div>";
    
    // 内存信息
    copyrightHtml += "<div class='col-md-4'>";
    copyrightHtml += "<h6 class='text-primary'>内存信息</h6>";
    copyrightHtml += "<table class='table table-sm table-striped'>";
    
    // 获取内存信息
    size_t heap_size = ESP.getHeapSize();
    size_t free_heap = ESP.getFreeHeap();
    size_t min_free_heap = ESP.getMinFreeHeap();
    size_t psram_size = ESP.getPsramSize();
    size_t free_psram = ESP.getFreePsram();
    
    copyrightHtml += "<tr><td>堆内存总量</td><td>" + String(heap_size / 1024) + " KB</td></tr>";
    copyrightHtml += "<tr><td>可用堆内存</td><td>" + String(free_heap / 1024) + " KB</td></tr>";
    copyrightHtml += "<tr><td>最小空闲堆</td><td>" + String(min_free_heap / 1024) + " KB</td></tr>";
    copyrightHtml += "<tr><td>PSRAM总量</td><td>" + String(psram_size / 1024 / 1024.0, 2) + " MB</td></tr>";
    copyrightHtml += "<tr><td>可用PSRAM</td><td>" + String(free_psram / 1024 / 1024.0, 2) + " MB</td></tr>";
    
    copyrightHtml += "</table>";
    copyrightHtml += "</div>";
    
    // WiFi和系统信息
    copyrightHtml += "<div class='col-md-4'>";
    copyrightHtml += "<h6 class='text-primary'>系统信息</h6>";
    copyrightHtml += "<table class='table table-sm table-striped'>";
    
    // WiFi信息
    copyrightHtml += "<tr><td>WiFi状态</td><td>" + String((WiFi.status() == WL_CONNECTED) ? "已连接" : "未连接") + "</td></tr>";
    if (WiFi.status() == WL_CONNECTED) {
        copyrightHtml += "<tr><td>SSID</td><td>" + WiFi.SSID() + "</td></tr>";
        copyrightHtml += "<tr><td>RSSI</td><td>" + String(WiFi.RSSI()) + " dBm</td></tr>";
        copyrightHtml += "<tr><td>IP地址</td><td>" + WiFi.localIP().toString() + "</td></tr>";
    }
    
    // 运行时间
    uint32_t uptime = millis() / 1000;
    uint32_t hours = uptime / 3600;
    uint32_t minutes = (uptime % 3600) / 60;
    uint32_t seconds = uptime % 60;
    String uptime_str = String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
    copyrightHtml += "<tr><td>运行时间</td><td>" + uptime_str + "</td></tr>";
    
    // 获取SDK版本
    copyrightHtml += "<tr><td>SDK版本</td><td>" + String(ESP.getSdkVersion()) + "</td></tr>";
    
    // 获取芯片ID
    copyrightHtml += "<tr><td>芯片ID</td><td>" + String(ESP.getEfuseMac(), HEX) + "</td></tr>";
    
    copyrightHtml += "</table>";
    copyrightHtml += "</div>";
    
    copyrightHtml += "</div>";
    copyrightHtml += "</div>";
    copyrightHtml += "</div>";
    copyrightHtml += "</div>";
    copyrightHtml += "</footer>";
    
    return copyrightHtml;
}

String WebConfigServer::generateScreenPage(const String& screenName, const String& screenTitle, const String& content) {
    String html = "";
    html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>" + screenTitle + "</title>";
    html += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
    html += "<style>body { padding-top: 20px; background-color: #f8f9fa; } .container { max-width: 900px; } .navbar-brand { font-weight: bold; } .btn-primary { background-color: #4CAF50; border-color: #4CAF50; } .btn-primary:hover { background-color: #45a049; }</style>";
    html += "</head><body>";
    
    // 统一导航栏样式
    html += "<div class='container'>";
    html += "<nav class='navbar navbar-expand-lg bg-info shadow-sm mb-4'>";
    html += "  <div class='container-fluid'>";
    html += "    <a class='navbar-brand text-white' href='/'>ESP32信息板</a>";
    html += "    <button class='navbar-toggler' type='button' data-bs-toggle='collapse' data-bs-target='#navbarNav' aria-controls='navbarNav' aria-expanded='false' aria-label='Toggle navigation'>";
    html += "      <span class='navbar-toggler-icon'></span>";
    html += "    </button>";
    html += "    <div class='collapse navbar-collapse' id='navbarNav'>";
    html += "      <ul class='navbar-nav'>";
    // 首页导航项
    html += "        <li class='nav-item'><a class='nav-link text-white";
    if (screenName == "") html += " active bg-primary font-weight-bold";
    html += "' href='/'>首页</a></li>";
    
    // 数据管理导航项
    html += "        <li class='nav-item'><a class='nav-link text-white";
    if (screenName == "json-files") html += " active bg-primary font-weight-bold";
    html += "' href='/json-files'>JSON文件</a></li>";   
    // 信息展示导航项
    html += "        <li class='nav-item'><a class='nav-link text-white";
    if (screenName == "news") html += " active bg-primary font-weight-bold";
    html += "' href='/screen-news'>新闻</a></li>";
    
    html += "        <li class='nav-item'><a class='nav-link text-white";
    if (screenName == "calendar") html += " active bg-primary font-weight-bold";
    html += "' href='/screen-calendar'>日历</a></li>";
    
    // 网址收藏导航项
    html += "        <li class='nav-item'><a class='nav-link text-white";
    if (screenName == "bookmarks") html += " active bg-primary font-weight-bold";
    html += "' href='/bookmarks'>网址收藏</a></li>";
    
    html += "        <li class='nav-item'><a class='nav-link text-white";
    if (screenName == "iciba") html += " active bg-primary font-weight-bold";
    html += "' href='/screen-iciba'>每日一句</a></li>";
    
    html += "        <li class='nav-item'><a class='nav-link text-white";
    if (screenName == "astronauts") html += " active bg-primary font-weight-bold";
    html += "' href='/screen-astronauts'>太空站宇航员</a></li>";
    
    // 随机内容导航项
    html += "        <li class='nav-item'><a class='nav-link text-white";
    if (screenName == "taxicsoul") html += " active bg-primary font-weight-bold";
    html += "' href='/random-taxicsoul'>乌鸡汤</a></li>";
    
    html += "        <li class='nav-item'><a class='nav-link text-white";
    if (screenName == "maoselect") html += " active bg-primary font-weight-bold";
    html += "' href='/random-maoselect'>毛选</a></li>";
    
    html += "        <li class='nav-item'><a class='nav-link text-white";
    if (screenName == "soul") html += " active bg-primary font-weight-bold";
    html += "' href='/random-soul'>禅语</a></li>";
    
    html += "      </ul>";
    html += "    </div>";
    html += "  </div>";
    html += "</nav>";
    
    // 内容区域
    html += "<div class='card mb-4 shadow-sm border-0 rounded-lg'>";
    html += "<div class='card-header bg-success text-white font-bold'>" + screenTitle + "</div>";
    html += "<div class='card-body bg-white'>";
    html += content;
    html += "</div>";
    html += "</div>";
    
    html += "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>";
    html += "</div>"; // 关闭外部container标签
    
    // 调用统一的版权信息函数 - 移到container外部
    html += generateCopyrightInfo();
    
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

            // 显示fenxiang_img
            if (doc.containsKey("fenxiang_img")) {
                String imgUrl = doc["fenxiang_img"].as<String>();
                content += "<div class='mb-3 text-center'>";
                content += "<img src='" + imgUrl + "' class='img-fluid rounded' alt='每日一句图片'>";
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

    if (isAPMode) {
        // 配置模式 - 简化界面，只保留必要的配置项
        readWiFiConfig(ssid, password);
        getNTPServerTimezone(timezone);
        
        // 生成简化的HTML页面，移除导航栏、其他表单和版权信息
        String html = "";
        html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>ESP32配网</title>";
        html += "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>";
        html += "<style>body { background-color: #f8f9fa; padding-top: 20px; } .container { max-width: 500px; }</style>";
        html += "</head><body>";
        
        html += "<div class='container'>";
        html += "  <div class='text-center mb-4'>";
        html += "    <h2>ESP32配网</h2>";
        html += "    <p>接入点IP: " + WiFi.softAPIP().toString() + "</p>";
        html += "  </div>";
        
        html += "  <div class='card shadow-sm rounded-lg'>";
        html += "    <div class='card-header bg-primary text-white text-center'>WiFi配置</div>";
        html += "    <div class='card-body'>";
        html += "      <form action='/config' method='post'>";
        html += "        <div class='mb-3'>";
        html += "          <label for='ssid' class='form-label'>WiFi名称</label>";
        html += "          <input type='text' class='form-control' id='ssid' name='ssid' value='" + ssid + "'>";
        html += "        </div>";
        html += "        <div class='mb-3'>";
        html += "          <label for='password' class='form-label'>WiFi密码</label>";
        html += "          <input type='password' class='form-control' id='password' name='password' value='" + password + "'>";
        html += "        </div>";
        html += "        <div class='mb-3'>";
        html += "          <label for='timezone' class='form-label'>NTP时区</label>";
        html += "          <input type='number' class='form-control' id='timezone' name='timezone' value='" + String(timezone) + "' min='-12' max='14'>";
        html += "          <div class='form-text'>整数，如北京时间为8</div>";
        html += "        </div>";
        html += "        <button type='submit' class='btn btn-primary w-100'>保存配置</button>";
        html += "      </form>";
        html += "    </div>";
        html += "  </div>";
        
        // 添加WiFi扫描结果
        html += scanWiFiNetworks();
        
        html += "</div>";
        html += "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>";
        html += "</body></html>";
        
        server.send(200, "text/html", html);
        return;
    }
    
    // 非AP模式时，保留原有完整界面
    String contentHtml = "";
    // 显示当前状态信息
    contentHtml += "<div class=\"card mb-4 shadow-sm rounded-lg\">";
    contentHtml += "  <div class=\"card-header bg-success text-white\">系统状态</div>";
    contentHtml += "  <div class=\"card-body\">";
    contentHtml += "    <p class=\"text-success\">当前为联网模式 (STA模式)</p>";
    contentHtml += "    <p>设备IP: " + WiFi.localIP().toString() + "</p>";
    contentHtml += "    <p>WiFi信号强度: " + String(WiFi.RSSI()) + " dBm</p>";
    contentHtml += "    <p>软件版本: " SOFTWARE_VERSION "</p>";
    contentHtml += "  </div>";
    contentHtml += "</div>";
    
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
    
    // 获取当前的Web授权配置
    String webUsername = "admin";
    String webPassword = "admin";
    String deviceName = "esp32-infoboard";
    ConfigManager* configManager = ConfigManager::getInstance();
    configManager->getWebAuthConfig(webUsername, webPassword);
    deviceName = configManager->getDeviceName();
    
    // 主题选择和Web授权功能
    contentHtml += "<div class=\"card mb-4 shadow-sm rounded-lg\">";
    contentHtml += "  <div  class=\"card-header bg-success text-white\">显示主题和Web授权配置</div>";
    contentHtml += "  <div class=\"card-body\">";
    contentHtml += "    <form action='/theme' method='post'>";
    contentHtml += "      <div class=\"mb-3\">";
    contentHtml += "        <label for=\"theme\" class=\"form-label\">选择显示主题</label>";
    contentHtml += "        <select class=\"form-control\" id=\"theme\" name=\"theme\">";
    contentHtml += "          <option value=\"dark\"";
    // 获取主题配置
    bool isLightTheme = ConfigManager::getInstance()->getDisplayTheme();
if (!isLightTheme) contentHtml += " selected";
    contentHtml += ">黑夜主题</option>";
    contentHtml += "          <option value=\"light\"";
    if (isLightTheme) contentHtml += " selected";
    contentHtml += ">白天主题</option>";
    contentHtml += "        </select>";
    contentHtml += "        <div class=\"form-text\">白天主题：白底黑字；黑夜主题：黑底白字</div>";
    contentHtml += "      </div>";
    
    // 设备名称配置
    contentHtml += "      <hr class=\"my-4\">";
    contentHtml += "      <h5 class=\"mb-3\">设备名称配置</h5>";
    contentHtml += "      <div class=\"mb-3\">";
    contentHtml += "        <label for=\"device_name\" class=\"form-label\">设备名称</label>";
    contentHtml += "        <input type=\"text\" class=\"form-control\" id=\"device_name\" name=\"device_name\" value='" + deviceName + "'>";
    contentHtml += "        <div class=\"form-text\">设置设备的识别名称，默认为esp32-infoboard</div>";
    contentHtml += "      </div>";
    
    // Web授权配置
    contentHtml += "      <hr class=\"my-4\">";
    contentHtml += "      <h5 class=\"mb-3\">Web配置访问授权</h5>";
    contentHtml += "      <div class=\"mb-3\">";
    contentHtml += "        <label for=\"web_username\" class=\"form-label\">授权用户名</label>";
    contentHtml += "        <input type=\"text\" class=\"form-control\" id=\"web_username\" name=\"web_username\" value='" + webUsername + "'>";
    contentHtml += "      </div>";
    contentHtml += "      <div class=\"mb-3\">";
    contentHtml += "        <label for=\"web_password\" class=\"form-label\">授权密码</label>";
    contentHtml += "        <input type=\"password\" class=\"form-control\" id=\"web_password\" name=\"web_password\" value='" + webPassword + "'>";
    contentHtml += "        <div class=\"form-text\">设置Web配置页面访问的用户名和密码</div>";
    contentHtml += "      </div>";
    
    contentHtml += "      <div class='btn-group' role='group'>";
    contentHtml += "        <button type=\"submit\" class=\"btn btn-primary\">保存配置</button>";
    contentHtml += "        <form action='/restart' method='post' style='display:inline;'>";
    contentHtml += "          <button type='submit' class='btn btn-warning'>重启设备</button>";
    contentHtml += "        </form>";
    contentHtml += "      </div>";
    contentHtml += "    </form>";
    contentHtml += "  </div>";
    contentHtml += "</div>";
    
    // 留言板功能
    contentHtml += "<div class=\"card mb-4 shadow-sm rounded-lg\">";
    contentHtml += "  <div  class=\"card-header bg-success text-white\">留言板内容配置</div>";
    contentHtml += "  <div class=\"card-body\">";
    contentHtml += "    <form action='/note' method='post'>";
    contentHtml += "      <div class=\"mb-3\">";
    contentHtml += "        <label for=\"note\" class=\"form-label\">留言内容</label>";
    contentHtml += "        <textarea class=\"form-control\" id=\"note\" name=\"note\" rows=\"6\">" + noteContent + "</textarea>";
    contentHtml += "      </div>";
    contentHtml += "      <button type=\"submit\" class=\"btn btn-secondary\">保存留言内容</button>";
    contentHtml += "    </form>";
    contentHtml += "  </div>";
    contentHtml += "</div>";
    
    // 使用统一的generateScreenPage函数生成完整页面
    String html = generateScreenPage("", "ESP32信息板", contentHtml);
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
    if (server.hasArg("ssid") && server.hasArg("password") && server.hasArg("timezone")) {
        String ssid = server.arg("ssid");
        String password = server.arg("password");
        int timezone = server.arg("timezone").toInt();
        
        // 如果没有提供theme参数，使用当前配置的主题或默认值
        bool isLightTheme = true; // 默认亮色主题
        if (server.hasArg("theme")) {
            String theme = server.arg("theme");
            isLightTheme = (theme == "light");
        } else {
            ConfigManager* configManager = ConfigManager::getInstance();
            if (configManager->isConfigLoaded()) {
                isLightTheme = configManager->getDisplayTheme();
            }
        }
        
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
                successHtml += "    <div class='btn-group' role='group'>";
                successHtml += "      <a href='/' class='btn btn-success'>返回首页</a>";
                successHtml += "      <form action='/restart' method='post' style='display:inline;'>";
                successHtml += "        <button type='submit' class='btn btn-warning'>重启设备</button>";
                successHtml += "      </form>";
                successHtml += "    </div>";
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
    
    // 生成内容区域
    String contentHtml = "";
    contentHtml += "<div class=\"card mb-4\">";
    contentHtml += "  <div class=\"card-header bg-success text-white\">JSON文件列表</div>";
    contentHtml += "  <div class=\"card-body\">";
    contentHtml += "    <div class=\"list-group\">";
    contentHtml += jsonFilesList;
    contentHtml += "    </div>";
    contentHtml += "  </div>";
    contentHtml += "</div>";
    
    contentHtml += "<div class=\"card\">";
    contentHtml += "  <div class=\"card-body text-center\">";
    contentHtml += "    <a href=\"/\" class=\"btn btn-success\">返回首页</a>";
    contentHtml += "  </div>";
    contentHtml += "</div>";
    
    // 使用统一的generateScreenPage函数生成完整页面
    String html = generateScreenPage("json-files", "JSON文件列表 - ESP32信息板", contentHtml);
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
 * 扫描附近WiFi网络
 */
String WebConfigServer::scanWiFiNetworks() {
    Serial.println("开始扫描WiFi网络...");
    int numNetworks = WiFi.scanNetworks();
    Serial.printf("发现 %d 个WiFi网络\n", numNetworks);
    
    String wifiListHtml = "";
    if (numNetworks > 0) {
        wifiListHtml += "<div class='mt-3'>";
        wifiListHtml += "<h4 class='text-success'>附近的WiFi网络：</h4>";
        wifiListHtml += "<div class='list-group'>";
        
        for (int i = 0; i < numNetworks; i++) {
            // 获取信号强度（RSSI）
            int32_t rssi = WiFi.RSSI(i);
            String signalStrength = "弱"; // 默认弱信号
            String signalClass = "text-danger"; // 红色表示弱信号
            
            if (rssi > -70) { // 强信号
                signalStrength = "强"; 
                signalClass = "text-success"; 
            } else if (rssi > -85) { // 中等信号
                signalStrength = "中"; 
                signalClass = "text-warning"; 
            }
            
            wifiListHtml += "<a href='#' onclick='document.getElementById(\"ssid\").value=\"" + WiFi.SSID(i) + "\";' class='list-group-item list-group-item-action'>";
            wifiListHtml += "<div class='d-flex justify-content-between align-items-center'>";
            wifiListHtml += "<span>" + WiFi.SSID(i) + "</span>";
            wifiListHtml += "<span class='badge " + signalClass + "'>" + signalStrength + "</span>";
            wifiListHtml += "</div>";
            wifiListHtml += "</a>";
        }
        
        wifiListHtml += "</div>";
        wifiListHtml += "</div>";
    } else {
        wifiListHtml += "<div class='alert alert-info mt-3'>未发现WiFi网络</div>";
    }
    
    return wifiListHtml;
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
 * 验证URL是否有效并获取页面标题
 * 返回标题（如果验证成功），否则返回空字符串
 */
String WebConfigServer::validateUrlAndGetTitle(const String& url) {
    // 确保URL有协议前缀
    String validUrl = url;
    if (!validUrl.startsWith("http://") && !validUrl.startsWith("https://")) {
        validUrl = "http://" + validUrl;
    }
    
    // 发送HTTP请求获取页面内容
    HTTPClient http;
    String response = "";
    
    // 开始HTTP连接
    http.begin(validUrl);
    // 设置超时时间
    http.setTimeout(10000);
    // 设置User-Agent以避免被某些网站拒绝
    http.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36");
    
    // 发送GET请求
    int httpCode = http.GET();
    
    // 检查响应状态
    if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
        // 请求成功，获取响应内容
        response = http.getString();
        
        // 从HTML中提取标题
        int titleStart = response.indexOf("<title>");
        int titleEnd = response.indexOf("</title>");
        
        if (titleStart != -1 && titleEnd != -1) {
            titleStart += 7; // 跳过 "<title>"
            String title = response.substring(titleStart, titleEnd);
            
            // 清理标题中的HTML实体和多余空格
            title.replace("&lt;", "<");
            title.replace("&gt;", ">");
            title.replace("&quot;", "\"");
            title.replace("&amp;", "&");
            
            // 移除多余的空格和换行符
            title.trim();
            
            http.end();
            return title;
        }
    }
    
    // 关闭连接
    http.end();
    return "";
}

/**
 * 处理网址收藏页面请求
 */
void WebConfigServer::handleBookmarks() {
    // 获取ConfigManager实例
    ConfigManager* configManager = ConfigManager::getInstance();
    
    // 处理POST请求（增删改操作）
    if (server.method() == HTTP_POST) {
        String action = server.arg("action");
        
        if (action == "add") {
            // 添加新的收藏网址
            String url = server.arg("url");
            
            if (url.length() > 0) {
                // 验证URL并获取标题
                String title = validateUrlAndGetTitle(url);
                
                if (title.length() > 0) {
                    // URL有效，添加到收藏
                    if (configManager->addBookmark(title, url)) {
                        server.sendHeader("Location", "/bookmarks?status=added");
                        server.send(303);
                        return;
                    } else {
                        server.sendHeader("Location", "/bookmarks?status=save_error");
                        server.send(303);
                        return;
                    }
                } else {
                    server.sendHeader("Location", "/bookmarks?status=invalid_url");
                    server.send(303);
                    return;
                }
            } else {
                server.sendHeader("Location", "/bookmarks?status=empty_url");
                server.send(303);
                return;
            }
        } else if (action == "delete") {
            // 删除收藏网址
            int index = server.arg("index").toInt();
            
            if (configManager->deleteBookmark(index)) {
                server.sendHeader("Location", "/bookmarks?status=deleted");
                server.send(303);
                return;
            } else {
                server.sendHeader("Location", "/bookmarks?status=delete_error");
                server.send(303);
                return;
            }
        } else if (action == "update") {
            // 更新收藏网址
            int index = server.arg("index").toInt();
            String title = server.arg("title");
            String url = server.arg("url");
            
            if (url.length() > 0 && title.length() > 0) {
                // 验证URL
                String validatedTitle = validateUrlAndGetTitle(url);
                
                if (validatedTitle.length() > 0) {
                    // URL有效，更新收藏
                    if (configManager->updateBookmark(index, title, url)) {
                        server.sendHeader("Location", "/bookmarks?status=updated");
                        server.send(303);
                        return;
                    } else {
                        server.sendHeader("Location", "/bookmarks?status=update_error");
                        server.send(303);
                        return;
                    }
                } else {
                    server.sendHeader("Location", "/bookmarks?status=invalid_url_update");
                    server.send(303);
                    return;
                }
            } else {
                server.sendHeader("Location", "/bookmarks?status=empty_fields");
                server.send(303);
                return;
            }
        }
    }
    
    // 处理GET请求（显示页面）
    String content = "";
    
    // 显示状态消息
    String status = server.arg("status");
    if (status == "added") {
        content += "<div class='alert alert-success mb-4'>网址添加成功！</div>";
    } else if (status == "deleted") {
        content += "<div class='alert alert-success mb-4'>网址删除成功！</div>";
    } else if (status == "updated") {
        content += "<div class='alert alert-success mb-4'>网址更新成功！</div>";
    } else if (status == "invalid_url") {
        content += "<div class='alert alert-danger mb-4'>无效的网址，请检查URL是否正确！</div>";
    } else if (status == "empty_url") {
        content += "<div class='alert alert-warning mb-4'>网址不能为空！</div>";
    } else if (status == "save_error" || status == "delete_error" || status == "update_error") {
        content += "<div class='alert alert-danger mb-4'>操作失败，请重试！</div>";
    } else if (status == "empty_fields") {
        content += "<div class='alert alert-warning mb-4'>标题和网址不能为空！</div>";
    } else if (status == "invalid_url_update") {
        content += "<div class='alert alert-danger mb-4'>更新时URL无效，请检查URL是否正确！</div>";
    }
    
    // 添加新网址的表单
    content += "<div class='card mb-4'>";
    content += "  <div class='card-header bg-primary text-white'>添加新网址</div>";
    content += "  <div class='card-body'>";
    content += "    <form action='/bookmarks' method='post'>";
    content += "      <input type='hidden' name='action' value='add'>";
    content += "      <div class='mb-3'>";
    content += "        <label for='url' class='form-label'>网址</label>";
    content += "        <input type='url' class='form-control' id='url' name='url' placeholder='请输入网址，如：https://www.example.com'>";
    content += "        <div class='form-text'>系统将自动验证网址并获取页面标题</div>";
    content += "      </div>";
    content += "      <button type='submit' class='btn btn-primary'>添加网址</button>";
    content += "    </form>";
    content += "  </div>";
    content += "</div>";
    
    // 显示收藏的网址列表
    JsonArray bookmarks = configManager->getBookmarks();
    content += "<div class='card mb-4'>";
    content += "  <div class='card-header bg-secondary text-white'>收藏的网址</div>";
    content += "  <div class='card-body'>";
    
    if (bookmarks.size() > 0) {
        content += "<table class='table table-striped'>";
        content += "  <thead>";
        content += "    <tr>";
        content += "      <th scope='col'>#</th>";
        content += "      <th scope='col'>标题</th>";
        content += "      <th scope='col'>网址</th>";
        content += "      <th scope='col'>操作</th>";
        content += "    </tr>";
        content += "  </thead>";
        content += "  <tbody>";
        
        // 遍历所有收藏的网址
        for (int i = 0; i < bookmarks.size(); i++) {
            JsonObject bookmark = bookmarks[i].as<JsonObject>();
            String title = bookmark["title"].as<String>();
            String url = bookmark["url"].as<String>();
            
            content += "    <tr>";
            content += "      <th scope='row'>" + String(i + 1) + "</th>";
            content += "      <td>" + title + "</td>";
            content += "      <td><a href='" + url + "' target='_blank'>" + url + "</a></td>";
            content += "      <td>";
            content += "        <button type='button' class='btn btn-sm btn-primary' data-bs-toggle='modal' data-bs-target='#editModal' data-index='" + String(i) + "' data-title='" + title + "' data-url='" + url + "'>编辑</button>";
            content += "        <form action='/bookmarks' method='post' style='display: inline;'>";
            content += "          <input type='hidden' name='action' value='delete'>";
            content += "          <input type='hidden' name='index' value='" + String(i) + "'>";
            content += "          <button type='submit' class='btn btn-sm btn-danger'>删除</button>";
            content += "        </form>";
            content += "      </td>";
            content += "    </tr>";
        }
        
        content += "  </tbody>";
        content += "</table>";
    } else {
        content += "    <div class='alert alert-info'>暂无收藏的网址</div>";
    }
    
    content += "  </div>";
    content += "</div>";
    
    // 编辑网址的模态框
    content += "<!-- 编辑模态框 -->";
    content += "<div class='modal fade' id='editModal' tabindex='-1' aria-labelledby='editModalLabel' aria-hidden='true'>";
    content += "  <div class='modal-dialog'>";
    content += "    <div class='modal-content'>";
    content += "      <div class='modal-header'>";
    content += "        <h5 class='modal-title' id='editModalLabel'>编辑网址</h5>";
    content += "        <button type='button' class='btn-close' data-bs-dismiss='modal' aria-label='Close'></button>";
    content += "      </div>";
    content += "      <div class='modal-body'>";
    content += "        <form action='/bookmarks' method='post'>";
    content += "          <input type='hidden' name='action' value='update'>";
    content += "          <input type='hidden' id='editIndex' name='index'>";
    content += "          <div class='mb-3'>";
    content += "            <label for='editTitle' class='form-label'>标题</label>";
    content += "            <input type='text' class='form-control' id='editTitle' name='title'>";
    content += "          </div>";
    content += "          <div class='mb-3'>";
    content += "            <label for='editUrl' class='form-label'>网址</label>";
    content += "            <input type='url' class='form-control' id='editUrl' name='url'>";
    content += "          </div>";
    content += "          <button type='submit' class='btn btn-primary'>保存修改</button>";
    content += "        </form>";
    content += "      </div>";
    content += "    </div>";
    content += "  </div>";
    content += "</div>";
    
    // JavaScript 来填充编辑模态框
    content += "<script>";
    content += "  var editModal = document.getElementById('editModal');";
    content += "  editModal.addEventListener('show.bs.modal', function (event) {";
    content += "    var button = event.relatedTarget;";
    content += "    var index = button.getAttribute('data-index');";
    content += "    var title = button.getAttribute('data-title');";
    content += "    var url = button.getAttribute('data-url');";
    content += "    ";
    content += "    var modalTitle = editModal.querySelector('.modal-title');";
    content += "    var editIndex = editModal.querySelector('#editIndex');";
    content += "    var editTitle = editModal.querySelector('#editTitle');";
    content += "    var editUrl = editModal.querySelector('#editUrl');";
    content += "    ";
    content += "    editIndex.value = index;";
    content += "    editTitle.value = title;";
    content += "    editUrl.value = url;";
    content += "  });";
    content += "</script>";
    
    // 生成完整页面并发送响应
    String html = generateScreenPage("bookmarks", "网址收藏 - ESP32信息板", content);
    server.send(200, "text/html", html);
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



// 外部声明，避免重复定义
extern const char* MaoSelect[];
extern const char* Soul[];
extern const char* ToxicSoul[];
extern const int MaoSelectCount;
// Soul和ToxicSoul数组的大小常量
const int SOUL_COUNT = 108;
const int TOXIC_SOUL_COUNT = 108;

/**
 * 处理随机乌鸡汤内容页面请求
 */
void WebConfigServer::handleRandomToxicSoul() {
    // 生成随机索引
    randomSeed(millis());
    int randomIndex = random(TOXIC_SOUL_COUNT);
    
    // 获取随机内容
    String content = ToxicSoul[randomIndex];
    
    // 生成内容区域
    String contentHtml = "<div class='quote-box p-5 bg-light rounded border border-warning text-center'>";
    
    // 将换行符替换为HTML换行标签
    content.replace("\n", "<br>");
    contentHtml += "  <p class='fs-4 text-gray-800 mb-0'>" + content + "</p>";
    
    contentHtml += "</div>";
    contentHtml += "<div class='text-center mt-4'>";
    contentHtml += "  <a href='/random-taxicsoul' class='btn btn-warning'>再看一条乌鸡汤</a>";
    contentHtml += "  <a href='/random-maoselect' class='btn btn-outline-secondary ms-2'>查看毛选</a>";
    contentHtml += "  <a href='/random-soul' class='btn btn-outline-secondary ms-2'>查看禅语</a>";
    contentHtml += "</div>";
    
    // 生成完整页面
    String html = generateScreenPage("taxicsoul", "随机乌鸡汤", contentHtml);
    
    server.send(200, "text/html", html);
}

/**
 * 处理随机毛选内容页面请求
 */
void WebConfigServer::handleRandomMaoSelect() {
    // 生成随机索引
    randomSeed(millis());
    int randomIndex = random(MaoSelectCount);
    
    // 获取随机内容
    String content = MaoSelect[randomIndex];
    
    // 生成内容区域
    String contentHtml = "<div class='quote-box p-5 bg-light rounded border border-danger text-center'>";
    
    // 将换行符替换为HTML换行标签
    content.replace("\n", "<br>");
    contentHtml += "  <p class='fs-4 text-gray-800 mb-0'>" + content + "</p>";
    
    contentHtml += "</div>";
    contentHtml += "<div class='text-center mt-4'>";
    contentHtml += "  <a href='/random-maoselect' class='btn btn-danger'>再看一条毛选</a>";
    contentHtml += "  <a href='/random-taxicsoul' class='btn btn-outline-secondary ms-2'>查看乌鸡汤</a>";
    contentHtml += "  <a href='/random-soul' class='btn btn-outline-secondary ms-2'>查看禅语</a>";
    contentHtml += "</div>";
    
    // 生成完整页面
    String html = generateScreenPage("maoselect", "随机毛选内容", contentHtml);
    
    server.send(200, "text/html", html);
}

/**
 * 处理随机禅语内容页面请求
 */
void WebConfigServer::handleRandomSoul() {
    // 生成随机索引
    randomSeed(millis());
    int randomIndex = random(SOUL_COUNT);
    
    // 获取随机内容
    String content = Soul[randomIndex];
    
    // 生成内容区域
    String contentHtml = "<div class='quote-box p-5 bg-light rounded border border-info text-center'>";
    
    // 将换行符替换为HTML换行标签
    content.replace("\n", "<br>");
    contentHtml += "  <p class='fs-4 text-gray-800 mb-0'>" + content + "</p>";
    
    contentHtml += "</div>";
    contentHtml += "<div class='text-center mt-4'>";
    contentHtml += "  <a href='/random-soul' class='btn btn-info'>再看一条禅语</a>";
    contentHtml += "  <a href='/random-taxicsoul' class='btn btn-outline-secondary ms-2'>查看乌鸡汤</a>";
    contentHtml += "  <a href='/random-maoselect' class='btn btn-outline-secondary ms-2'>查看毛选</a>";
    contentHtml += "</div>";
    
    // 生成完整页面
    String html = generateScreenPage("soul", "随机禅语内容", contentHtml);
    
    server.send(200, "text/html", html);
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