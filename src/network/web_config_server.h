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

    // 私有构造函数（单例模式）
    WebConfigServer();

    // 处理主页请求
    void handleRoot();

    // 处理WiFi配置请求
    void handleWiFiConfig();
    
    // 处理系统配置请求（NTP时区和API密钥）
    void handleSystemConfig();

    // 处理JSON文件查看请求
    void handleJsonFile();

    // 处理404错误
    void handleNotFound();

    // 处理系统重启请求
    void handleRestart();

    // 读取WiFi配置
    void readWiFiConfig(String& ssid, String& password);

    // 保存WiFi配置
    bool saveWiFiConfig(const String& ssid, const String& password);
    
    // 读取系统配置（NTP时区和API密钥）
    void readSystemConfig(String& apiKey, int& timezone);
    
    // 保存系统配置（NTP时区和API密钥）
    bool saveSystemConfig(const String& apiKey, int timezone);

    // 获取所有JSON文件列表
    String getJsonFilesList();

    // URL编码函数
    String urlEncode(const String& str);

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
};

#endif // WEB_CONFIG_SERVER_H