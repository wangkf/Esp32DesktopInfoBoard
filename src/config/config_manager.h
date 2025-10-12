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
    
    // 读取API密钥配置
    String getApiKey();
    
    // 读取NTP时区配置
    int getNTPServerTimezone();
    
    // 设置API密钥配置
    bool setApiKey(const String& apiKey);
    
    // 设置NTP时区配置
    bool setNTPServerTimezone(int timezone);
    
    // 读取屏幕亮度配置
    int getScreenBrightness();
    
    // 设置屏幕亮度配置
    bool setScreenBrightness(int brightness);
    
    // 读取硬件引脚配置
    int getButtonPin();
    int getLightSensorPin();
    int getScreenBrightnessPin();
    
    // 检查配置是否已加载
    bool isConfigLoaded();
};

#endif // CONFIG_MANAGER_H