#ifndef WEB_CONFIG_SERVER_H
#define WEB_CONFIG_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include "config/config_manager.h"

/**
 * Web配置服务器类
 * 负责创建WiFi热点和提供web配置界面
 */
class WebConfigServer {
private:
    static WebConfigServer* instance; // 单例实例
    WebServer server; // Web服务器实例
    bool isRunning; // 服务器运行状态
    const char* apSSID; // 热点名称
    const char* apPassword; // 热点密码
    
    // 认证相关方法
    bool authenticate();
    String base64Decode(String encoded);

    // 私有构造函数（单例模式）
    WebConfigServer();

    // 处理主页请求
    void handleRoot();

    // 处理配置请求（WiFi和时区）
    void handleConfig();

    // 处理JSON文件查看请求
    void handleJsonFile();
    
    // 处理网址收藏页面请求
    void handleBookmarks();

    // 处理404错误
    void handleNotFound();

    // 处理系统重启请求
    void handleRestart();
    
    // 处理留言板内容请求
    void handleNote();
    
    // 处理主题设置请求
    void handleTheme();

    // 读取WiFi配置
    void readWiFiConfig(String& ssid, String& password);

    // 扫描附近WiFi网络
    String scanWiFiNetworks();
    
    // 保存WiFi配置
    bool saveWiFiConfig(const String& ssid, const String& password);
    
    // 读取NTP服务器时区配置
    void getNTPServerTimezone(int& timezone);
    
    // 保存NTP服务器时区配置
    bool setNTPServerTimezone(int timezone);

    // 获取所有JSON文件列表
    String getJsonFilesList();

    // URL编码函数
    String urlEncode(const String& str);
    
    // 验证URL是否有效并获取页面标题
    String validateUrlAndGetTitle(const String& url);

    // 处理单个JSON文件内容请求
    void handleJsonFileContent();

    // 读取JSON文件内容
    String readJsonFileContent(const String& fileName);

public:
    // 获取单例实例
    static WebConfigServer* getInstance();

    // 初始化Web配置服务器
    void init();
    
    // 启动Web配置服务器
    bool start();
    
    // 启动Web配置服务器（重载版本，支持选择模式）
    bool start(bool useAPMode);
    
    // 停止Web配置服务器
    void stop();

    // 处理Web服务器请求
    void handleClient();

    // 检查服务器是否正在运行
    bool isServerRunning();
    
    // 生成通用的屏幕页面模板
    String generateScreenPage(const String& screenName, const String& screenTitle, const String& content);
    
    // 生成统一的版权信息
    String generateCopyrightInfo();
    
    // 生成月历表格
    String generateMonthCalendar(int year, int month);
    
    // 处理各屏幕类型的页面请求
    void handleNewsScreen();
    void handleCalendarScreen();
    void handleNotesScreen();
    void handleIcibaScreen();
    void handleAstronautsScreen();
    
    // 通用的随机内容处理函数
    void handleRandomContent(const String& screenName, const String& screenTitle, 
                           const String& content, const String& buttonText, 
                           const String& buttonClass, const String& borderClass);
    
    // 处理随机内容页面请求
    void handleRandomToxicSoul();
    void handleRandomMaoSelect();
    void handleRandomSoul();
};

#endif // WEB_CONFIG_SERVER_H