#include "config_manager.h"

// 初始化静态成员
ConfigManager* ConfigManager::instance = nullptr;

// 私有构造函数
ConfigManager::ConfigManager() : configLoaded(false) {
    // 初始化时清空配置文档
    configDoc.clear();
}

// 获取单例实例
ConfigManager* ConfigManager::getInstance() {
    if (instance == nullptr) {
        instance = new ConfigManager();
    }
    return instance;
}

// 初始化文件系统
bool ConfigManager::initFileSystem() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS挂载失败");
        return false;
    }
    Serial.println("SPIFFS挂载成功");
    return true;
}

// 初始化配置管理器
bool ConfigManager::init() {
    // 初始化文件系统
    if (!initFileSystem()) {
        return false;
    }
    
    // 加载配置文件
    if (SPIFFS.exists(configFile)) {
        File file = SPIFFS.open(configFile, "r");
        if (file) {
            DeserializationError error = deserializeJson(configDoc, file);
            if (!error) {
                configLoaded = true;
                Serial.println("配置文件加载成功");
                
                // 确保配置中包含web_auth部分，如果没有则添加默认值
                if (!configDoc.containsKey("web_auth")) {
                    Serial.println("配置中缺少web_auth部分，添加默认值");
                    JsonObject webAuthObj = configDoc.createNestedObject("web_auth");
                    webAuthObj["username"] = "admin";
                    webAuthObj["password"] = "admin";
                    // 保存更新后的配置
                    saveConfigToFile();
                }
            } else {
                Serial.print("配置文件解析失败: ");
                Serial.println(error.c_str());
            }
            file.close();
        } else {
            Serial.println("无法打开配置文件");
        }
    } else {
        Serial.println("配置文件不存在，使用默认配置");
        // 使用默认配置
        configDoc["wifi"]["ssid"] = "Mywifi";
        configDoc["wifi"]["password"] = "12345678";
        configDoc["ntp"]["timezone"] = 8;
        configDoc["display"]["theme_id"] = THEME_LIGHT; // 默认使用浅色主题 (0)
        configDoc["web_auth"]["username"] = "admin";
        configDoc["web_auth"]["password"] = "admin";
        configLoaded = true;   
        // 保存默认配置到文件
        saveConfigToFile();
    }   
    return configLoaded;
}
// 保存配置到文件
bool ConfigManager::saveConfigToFile() {
    Serial.printf("信息：正在保存配置文件到 %s\n", configFile);
    
    File file = SPIFFS.open(configFile, "w");
    if (!file) {
        Serial.printf("错误：无法打开配置文件 %s 进行写入，检查文件系统权限或空间\n", configFile);
        return false;
    }
    
    // 尝试序列化JSON到文件
    size_t bytesWritten = serializeJsonPretty(configDoc, file);
    if (bytesWritten == 0) {
        Serial.println("错误：配置序列化失败，可能是JSON数据结构错误");
        file.close();
        return false;
    }
    
    // 确保所有数据都写入
    file.flush();
    
    file.close();
    Serial.printf("信息：配置文件保存成功，写入 %u 字节数据\n", bytesWritten);
    return true;
}
// 读取WiFi配置
bool ConfigManager::getWiFiConfig(String& ssid, String& password) {
    if (!configLoaded) {
        return false;
    }   
    if (configDoc.containsKey("wifi")) {
        JsonObject wifiObj = configDoc["wifi"];
        if (wifiObj.containsKey("ssid")) {
            ssid = wifiObj["ssid"].as<String>();
        }
        if (wifiObj.containsKey("password")) {
            password = wifiObj["password"].as<String>();
        }
        return true;
    }
    return false;
}
// 保存WiFi配置
bool ConfigManager::setWiFiConfig(const String& ssid, const String& password) {
    if (!configLoaded) {
        return false;
    }   
    configDoc["wifi"]["ssid"] = ssid;
    configDoc["wifi"]["password"] = password;
    return saveConfigToFile();
}
// 读取NTP时区配置
int ConfigManager::getNTPServerTimezone() {
    if (!configLoaded || !configDoc.containsKey("ntp")) {
        return 8; // 默认东八区
    }   
    JsonObject ntpObj = configDoc["ntp"];
    if (ntpObj.containsKey("timezone")) {
        return ntpObj["timezone"].as<int>();
    }
    return 8; // 默认东八区
}
// 设置NTP时区配置
bool ConfigManager::setNTPServerTimezone(int timezone) {
    if (!configLoaded) {
        return false;
    }   
    if (configDoc.containsKey("ntp")) {
        configDoc["ntp"]["timezone"] = timezone;
    } else {
        JsonObject ntpObj = configDoc.createNestedObject("ntp");
        ntpObj["timezone"] = timezone;
    }
    return saveConfigToFile();
}
// 获取显示主题配置
int ConfigManager::getDisplayTheme() {
    if (!configLoaded) {
        Serial.println("信息：配置未加载，使用默认浅色主题");
        return THEME_LIGHT;
    }
    
    if (configDoc.containsKey("display") && configDoc["display"].containsKey("theme_id")) {
        int themeId = configDoc["display"]["theme_id"].as<int>();
        // 验证主题ID是否在有效范围内
        if (themeId >= 0 && themeId <= 2) {
            String themeName;
            switch(themeId) {
                case THEME_LIGHT: themeName = "浅色主题";
                    break;
                case THEME_DARK: themeName = "深色主题";
                    break;
                case THEME_AUTO: themeName = "特殊主题"; // 注意：这里的名称可以根据需要修改
                    break;
                default: themeName = "未知主题";
            }
            Serial.printf("信息：获取到主题配置 %s (ID: %d)\n", themeName.c_str(), themeId);
            return themeId;
        }
        Serial.printf("警告：发现无效的主题ID: %d，使用默认浅色主题\n", themeId);
        return THEME_LIGHT;
    }
    
    // 向后兼容：处理旧的light_theme配置
    if (configDoc.containsKey("display") && configDoc["display"].containsKey("light_theme")) {
        bool lightTheme = configDoc["display"]["light_theme"].as<bool>();
        int themeId = lightTheme ? THEME_LIGHT : THEME_DARK;
        String themeName = lightTheme ? "浅色主题" : "深色主题";
        Serial.printf("警告：检测到旧版light_theme配置，映射到 %s (ID: %d)，建议更新为新版theme_id格式\n", 
                     themeName.c_str(), themeId);
        return themeId;
    }
    
    // 未找到任何主题配置，使用默认主题
    Serial.println("信息：未找到主题配置，使用默认浅色主题");
    return THEME_LIGHT;
}

// 获取Web授权配置
bool ConfigManager::getWebAuthConfig(String& username, String& password) {
    if (!configLoaded) {
        return false;
    }
    
    if (configDoc.containsKey("web_auth")) {
        JsonObject webAuthObj = configDoc["web_auth"];
        if (webAuthObj.containsKey("username")) {
            username = webAuthObj["username"].as<String>();
        }
        if (webAuthObj.containsKey("password")) {
            password = webAuthObj["password"].as<String>();
        }
        return true;
    }
    
    return false;
}

// 设置Web授权配置
bool ConfigManager::setWebAuthConfig(const String& username, const String& password) {
    if (!configLoaded) {
        return false;
    }
    
    if (configDoc.containsKey("web_auth")) {
        configDoc["web_auth"]["username"] = username;
        configDoc["web_auth"]["password"] = password;
    } else {
        JsonObject webAuthObj = configDoc.createNestedObject("web_auth");
        webAuthObj["username"] = username;
        webAuthObj["password"] = password;
    }
    
    return saveConfigToFile();
}

// 获取设备名称配置
String ConfigManager::getDeviceName() {
    if (!configLoaded) {
        return "esp32-infoboard"; // 默认名称
    }
    
    if (configDoc.containsKey("device_name")) {
        return configDoc["device_name"].as<String>();
    }
    
    // 如果不存在，设置默认值并保存
    configDoc["device_name"] = "esp32-infoboard";
    saveConfigToFile();
    return "esp32-infoboard";
}

// 设置设备名称配置
bool ConfigManager::setDeviceName(const String& deviceName) {
    if (!configLoaded) {
        return false;
    }
    
    configDoc["device_name"] = deviceName;
    return saveConfigToFile();
}

// 设置显示主题配置
bool ConfigManager::setDisplayTheme(int themeId) {
    if (!configLoaded) {
        Serial.println("错误：配置文件未加载，无法设置主题");
        return false;
    }
    
    // 验证主题ID是否在有效范围内
    if (themeId < 0 || themeId > 2) { // 当前支持0=浅色, 1=深色, 2=特殊
        Serial.printf("错误：无效的主题ID %d，主题ID必须在0-2范围内\n", themeId);
        return false;
    }
    
    String themeName;
    switch(themeId) {
        case THEME_LIGHT: themeName = "浅色主题";
            break;
        case THEME_DARK: themeName = "深色主题";
            break;
        case THEME_AUTO: themeName = "特殊主题"; // 注意：这里的名称可以根据需要修改
            break;
        default: themeName = "未知主题";
    }
    
    if (configDoc.containsKey("display")) {
        // 检查是否需要更新
        int currentThemeId = configDoc["display"]["theme_id"].as<int>();
        if (currentThemeId == themeId) {
            Serial.printf("提示：主题已设置为 %s，无需更改\n", themeName.c_str());
            return true;
        }
        
        // 使用新的数字主题ID格式
        configDoc["display"]["theme_id"] = themeId;
        Serial.printf("信息：更新主题配置为 %s (ID: %d)\n", themeName.c_str(), themeId);
        
            if (configDoc["display"].containsKey("light_theme")) {
                configDoc["display"].remove("light_theme");
                Serial.println("信息：已移除旧的light_theme配置项，完全迁移到新版主题ID格式");
            }
        } else {
            JsonObject displayObj = configDoc.createNestedObject("display");
            displayObj["theme_id"] = themeId;
            Serial.printf("信息：创建新的显示配置，主题设置为 %s (ID: %d)\n", themeName.c_str(), themeId);
        }
    
    // 保存配置到文件
    if (saveConfigToFile()) {
        Serial.println("信息：主题配置保存成功");
        return true;
    } else {
        Serial.println("错误：主题配置保存失败");
        return false;
    }
}

// 检查配置是否已加载
bool ConfigManager::isConfigLoaded() {
    return configLoaded;
}

// 获取所有收藏网址
JsonArray ConfigManager::getBookmarks() {
    if (!configLoaded) {
        // 如果配置未加载，返回一个空数组
        return JsonArray();
    }
    
    // 如果bookmarks不存在，创建一个空数组
    if (!configDoc.containsKey("bookmarks")) {
        configDoc.createNestedArray("bookmarks");
        saveConfigToFile();
    }
    
    return configDoc["bookmarks"].as<JsonArray>();
}

// 添加收藏网址
bool ConfigManager::addBookmark(const String& title, const String& url) {
    if (!configLoaded) {
        return false;
    }
    
    // 确保bookmarks数组存在
    if (!configDoc.containsKey("bookmarks")) {
        configDoc.createNestedArray("bookmarks");
    }
    
    // 添加新的书签对象
    JsonArray bookmarks = configDoc["bookmarks"].as<JsonArray>();
    JsonObject bookmark = bookmarks.createNestedObject();
    bookmark["title"] = title;
    bookmark["url"] = url;
    
    return saveConfigToFile();
}

// 删除收藏网址
bool ConfigManager::deleteBookmark(int index) {
    if (!configLoaded || !configDoc.containsKey("bookmarks")) {
        return false;
    }
    
    JsonArray bookmarks = configDoc["bookmarks"].as<JsonArray>();
    if (index < 0 || index >= bookmarks.size()) {
        return false; // 索引超出范围
    }
    
    bookmarks.remove(index);
    return saveConfigToFile();
}

// 更新收藏网址
bool ConfigManager::updateBookmark(int index, const String& title, const String& url) {
    if (!configLoaded || !configDoc.containsKey("bookmarks")) {
        return false;
    }
    
    JsonArray bookmarks = configDoc["bookmarks"].as<JsonArray>();
    if (index < 0 || index >= bookmarks.size()) {
        return false; // 索引超出范围
    }
    
    JsonObject bookmark = bookmarks[index].as<JsonObject>();
    bookmark["title"] = title;
    bookmark["url"] = url;
    
    return saveConfigToFile();
}