#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// 配置管理类，负责统一处理所有配置的读取和保存
class ConfigManager {
private:
    static ConfigManager* instance;
    JsonDocument configDoc;
    bool configLoaded;
    const char* configFile = "/config.json";
    
    // 私有构造函数（单例模式）
    ConfigManager();
    
    // 初始化文件系统
    bool initFileSystem();
    
    // 保存配置到文件
    bool saveConfigToFile();
    
public:
    // 获取单例实例
    static ConfigManager* getInstance();
    
    // 初始化配置管理器
    bool init();
    
    // 读取WiFi配置
    bool getWiFiConfig(String& ssid, String& password);
    
    // 保存WiFi配置
    bool setWiFiConfig(const String& ssid, const String& password);

    // 读取NTP时区配置
    int getNTPServerTimezone();
    
    // 设置NTP时区配置
    bool setNTPServerTimezone(int timezone);
    
    // 检查配置是否已加载
    bool isConfigLoaded();
};

#endif // CONFIG_MANAGER_H