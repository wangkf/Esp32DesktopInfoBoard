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
    
    // 获取显示主题配置 (true: 白天主题, false: 黑夜主题)
    bool getDisplayTheme();
    
    // 设置显示主题配置
    bool setDisplayTheme(bool isLightTheme);
    
    // 获取Web授权配置
    bool getWebAuthConfig(String& username, String& password);
    
    // 设置Web授权配置
    bool setWebAuthConfig(const String& username, const String& password);
    
    // 获取设备名称配置
    String getDeviceName();
    
    // 设置设备名称配置
    bool setDeviceName(const String& deviceName);
    
    // 检查配置是否已加载
    bool isConfigLoaded();
    
    // 网址收藏相关方法
    // 获取所有收藏网址
    JsonArray getBookmarks();
    // 添加收藏网址
    bool addBookmark(const String& title, const String& url);
    // 删除收藏网址
    bool deleteBookmark(int index);
    // 更新收藏网址
    bool updateBookmark(int index, const String& title, const String& url);
};

#endif // CONFIG_MANAGER_H