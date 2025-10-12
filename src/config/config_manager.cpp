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
        configDoc["api"]["key"] = "myapikey";
        configDoc["display"]["brightness"] = 128;
        configDoc["display"]["refresh_interval"] = 60000;
        configDoc["wifi"]["ssid"] = "Mywifi";
        configDoc["wifi"]["password"] = "12345678";
        configDoc["ntp"]["timezone"] = 8;
        configLoaded = true;
        
        // 保存默认配置到文件
        saveConfigToFile();
    }
    
    return configLoaded;
}

// 保存配置到文件
bool ConfigManager::saveConfigToFile() {
    File file = SPIFFS.open(configFile, "w");
    if (!file) {
        Serial.println("无法打开配置文件进行写入");
        return false;
    }
    
    if (serializeJsonPretty(configDoc, file) == 0) {
        Serial.println("配置序列化失败");
        file.close();
        return false;
    }
    
    file.close();
    Serial.println("配置文件保存成功");
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

// 读取API密钥配置
String ConfigManager::getApiKey() {
    if (!configLoaded || !configDoc.containsKey("api")) {
        return "";
    }
    
    JsonObject apiObj = configDoc["api"];
    if (apiObj.containsKey("key")) {
        return apiObj["key"].as<String>();
    }
    
    return "";
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

// 设置API密钥配置
bool ConfigManager::setApiKey(const String& apiKey) {
    if (!configLoaded) {
        return false;
    }
    
    if (configDoc.containsKey("api")) {
        configDoc["api"]["key"] = apiKey;
    } else {
        JsonObject apiObj = configDoc.createNestedObject("api");
        apiObj["key"] = apiKey;
    }
    
    return saveConfigToFile();
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

// 读取屏幕亮度配置
int ConfigManager::getScreenBrightness() {
    if (!configLoaded || !configDoc.containsKey("display")) {
        return 128; // 默认亮度
    }
    
    JsonObject displayObj = configDoc["display"];
    if (displayObj.containsKey("brightness")) {
        return displayObj["brightness"].as<int>();
    }
    
    return 128; // 默认亮度
}

// 设置屏幕亮度配置
bool ConfigManager::setScreenBrightness(int brightness) {
    if (!configLoaded) {
        return false;
    }
    
    configDoc["display"]["brightness"] = brightness;
    
    return saveConfigToFile();
}

// 读取硬件引脚配置
int ConfigManager::getButtonPin() {
    if (!configLoaded || !configDoc.containsKey("hardware")) {
        return 18; // 默认引脚
    }
    
    JsonObject hardwareObj = configDoc["hardware"];
    if (hardwareObj.containsKey("BUTTON_PIN")) {
        return hardwareObj["BUTTON_PIN"].as<int>();
    }
    
    return 18; // 默认引脚
}

int ConfigManager::getLightSensorPin() {
    if (!configLoaded || !configDoc.containsKey("hardware")) {
        return 35; // 默认引脚
    }
    
    JsonObject hardwareObj = configDoc["hardware"];
    if (hardwareObj.containsKey("LIGHT_SENSOR_PIN")) {
        return hardwareObj["LIGHT_SENSOR_PIN"].as<int>();
    }
    
    return 35; // 默认引脚
}

int ConfigManager::getScreenBrightnessPin() {
    if (!configLoaded || !configDoc.containsKey("hardware")) {
        return 22; // 默认引脚
    }
    
    JsonObject hardwareObj = configDoc["hardware"];
    if (hardwareObj.containsKey("SCREEN_BRIGHTNESS_PIN")) {
        return hardwareObj["SCREEN_BRIGHTNESS_PIN"].as<int>();
    }
    
    return 22; // 默认引脚
}

// 检查配置是否已加载
bool ConfigManager::isConfigLoaded() {
    return configLoaded;
}