#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config/config.h"

// API URL常量定义
#define ASTRONAUTS_API_URL "http://api.open-notify.org/astros.json"
#define ICIBA_API_URL      "https://open.iciba.com/dsapi/"
#define NEWS_API_URL       "http://YOU_NEWS_API/hotnews/?sn=test"

// 缓存时间定义（毫秒）
#define CACHE_TIME_2HOURS 2 * 60 * 60 * 1000 // 2小时
#define CACHE_TIME_30MIN 30 * 60 * 1000     // 30分钟
#define CACHE_TIME_1HOUR 1 * 60 * 60 * 1000  // 1小时

// 数据类型枚举
enum DataType {
    ICIBA_DATA,
    ASTRONAUTS_DATA,
    NEWS_DATA
};

/**
 * 数据管理器类
 * 负责管理各种数据的获取、缓存和更新
 */
class DataManager {
private:
    static DataManager* instance; // 单例实例
    
    // 缓存相关变量
    unsigned int lastIcibaUpdateDate;     // 上次ICIBA数据更新日期 (YYYYMMDD格式)
    unsigned int lastAstronautsUpdateDate; // 上次宇航员数据更新日期 (YYYYMMDD格式)
    unsigned int lastNewsUpdateTime;      // 上次新闻数据更新时间
    
    // 强制刷新标志
    bool forceICIBARefresh;               // 强制刷新ICIBA数据
    bool forceAstronautsRefresh;          // 强制刷新宇航员数据
    bool forceNewsRefresh;                // 强制刷新新闻数据
    
    // 数据缓存
    String icibaData;           // ICIBA数据缓存
    String astronautsData;      // 宇航员数据缓存
    String newsData;            // 新闻数据缓存
    
    // 缓存是否有效
    bool isFirstStartup;        // 是否首次启动
    bool isTimeValid;           // NTP时间是否有效
    unsigned long firstStartupWaitTime; // 首次启动等待时间计时器
    
    // FreeRTOS任务句柄
    TaskHandle_t dataTaskHandle; 
    
    // 任务标志
    static volatile bool shouldUpdateAllData; // 全局标志，用于触发数据更新
    static volatile bool shouldUpdateICIBADaily; // 全局标志，用于触发每日一句更新
    static volatile bool shouldUpdateAstronauts; // 全局标志，用于触发宇航员数据更新
    static volatile bool shouldUpdateNews; // 全局标志，用于触发新闻数据更新
    
    // 检查NTP时间是否有效
    bool checkTimeValidity();
    
    // 私有构造函数（单例模式）
    DataManager();
    
    // 通用HTTP请求函数
    String httpGetRequest(const char* url);    
    
    // 数据任务函数（在第二个核心运行）
    static void dataTask(void *pvParameters);
    
    
    // 内部文件操作函数（在任务中调用）
    void internalSaveCacheData();
    void internalLoadCacheData();
    
    // 检查是否是两小时整点时刻
    bool isTwoHourIntervalOnTheHour();
    
    // 准点检查并更新数据
    void checkAndUpdateOnTheHour();
public:
    // 获取单例实例
    static DataManager* getInstance();
    
    // 初始化数据管理器
    void init();
    
    // 获取当前日期 (YYYYMMDD格式)
    unsigned int getCurrentDate();
    
    // 检查并更新所有需要的缓存数据
    void checkAndUpdateAllCaches();
    
    // 检查并更新特定类型的数据缓存
    void checkAndUpdateCache();
    
    // 检查并更新特定类型的数据缓存（带参数版本）
    bool checkAndUpdateCache(DataType type);
    
    // 强制刷新所有数据
    void forceRefreshAllData();
    
    // 通用数据获取接口
    
    // 获取是否首次启动状态
    bool getIsFirstStartup() { return isFirstStartup; }
    
    // 设置是否首次启动状态
    void setIsFirstStartup(bool value) { isFirstStartup = value; }
    
    // 获取NTP时间有效性状态
    bool getIsTimeValid() { return isTimeValid; }
    
    // 设置NTP时间有效性状态
    void setIsTimeValid(bool value) { isTimeValid = value; }
    
    // 通用数据获取函数
    bool fetchData(DataType type, String &result);
    
    // 文件系统相关函数
    void initFileSystem();     // 初始化文件系统
    void saveCacheData();      // 保存缓存数据到文件
    void loadCacheData();      // 从文件加载缓存数据

    // WiFi连接控制函数
    bool connectWiFiForDataFetch();  // 为数据获取连接WiFi
    void disconnectWiFiAfterDataFetch();  // 数据获取后断开WiFi

    // JSON数据处理
    void createStandardJsonResponse(ArduinoJson::V742PB22::JsonDocument& doc, ArduinoJson::V742PB22::JsonDocument& resultData);
    bool saveDataToJsonFile(const String& filename, const String& data, bool needWrapper = true);

};

#endif // DATA_MANAGER_H