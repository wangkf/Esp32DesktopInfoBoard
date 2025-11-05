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
    ConfigManager();// 私有构造函数（单例模式）
    bool initFileSystem();// 初始化文件系统
    bool saveConfigToFile();// 保存配置到文件  
public:
    static ConfigManager* getInstance();// 获取单例实例
    bool init();// 初始化配置管理器
    bool getWiFiConfig(String& ssid, String& password);// 读取WiFi配置
    bool setWiFiConfig(const String& ssid, const String& password);// 保存WiFi配置
    int getNTPServerTimezone();// 获取NTP时区配置
    bool setNTPServerTimezone(int timezone);// 设置NTP时区配置
    // 主题常量定义
    static const int THEME_DARK = 0;    // 黑夜主题
    static const int THEME_LIGHT = 1;   // 白天主题
    static const int THEME_AUTO = 2;    // 自动主题（示例，可扩展）
    int getDisplayTheme();// 获取显示主题配置 (返回主题ID: 0=黑夜, 1=白天, 2=自动等)
    bool setDisplayTheme(int themeId);// 设置显示主题配置
    bool getWebAuthConfig(String& username, String& password);// 获取Web授权配置
    bool setWebAuthConfig(const String& username, const String& password);// 设置Web授权配置
    String getDeviceName();// 获取设备名称配置
    bool setDeviceName(const String& deviceName);// 设置设备名称配置
    bool isConfigLoaded();// 检查配置是否已加载
    // 网址收藏相关方法
    JsonArray getBookmarks();    // 获取所有收藏网址
    bool addBookmark(const String& title, const String& url);// 添加收藏网址
    bool deleteBookmark(int index);// 删除收藏网址
    bool updateBookmark(int index, const String& title, const String& url);// 更新收藏网址
};
#endif // CONFIG_MANAGER_H