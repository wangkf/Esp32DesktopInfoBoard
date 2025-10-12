#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"

// 缓存时间定义（毫秒）
#define CACHE_TIME_2HOURS 2 * 60 * 60 * 1000 // 2小时
#define CACHE_TIME_30MIN 30 * 60 * 1000     // 30分钟
#define CACHE_TIME_1HOUR 1 * 60 * 60 * 1000  // 1小时

// 数据类型枚举
enum DataType {
    WEATHER_DATA,
    NEWS_DATA,
    ICBA_DATA,
    ASTRONAUTS_DATA,
    APRS_DATA
};

/**
 * 数据管理器类
 * 负责管理各种数据的获取、缓存和更新
 */
class DataManager {
private:
    static DataManager* instance; // 单例实例
    
    // 缓存相关变量
    unsigned long lastWeatherUpdateTime;  // 上次天气数据更新时间
    unsigned long lastNewsUpdateTime;     // 上次新闻数据更新时间
    unsigned long lastAstronautsUpdateTime; // 上次宇航员数据更新时间
    unsigned long lastAPRSUpdateTime;     // 上次APRS数据更新时间
    
    // 数据缓存
    String weatherData;         // 天气数据缓存
    String newsData;            // 新闻数据缓存
    String icbaData;            // ICBA数据缓存
    String astronautsData;      // 宇航员数据缓存
    String aprsData;            // APRS数据缓存
    
    // 缓存是否有效
    bool isFirstStartup;        // 是否首次启动
    
    // 私有构造函数（单例模式）
    DataManager();
    
    // 通用HTTP请求函数
    String httpGetRequest(const char* url);
    
public:
    // 获取单例实例
    static DataManager* getInstance();
    
    // 初始化数据管理器
    void init();
    
    // 检查并更新所有需要的缓存数据
    void checkAndUpdateAllCaches();
    
    // 检查并更新特定类型的数据缓存
    bool checkAndUpdateCache(DataType type);
    
    // 强制刷新所有数据
    void forceRefreshAllData();
    
    // 获取天气数据
    String getWeatherData();
    
    // 获取新闻数据
    String getNewsData();
    
    // 获取ICBA数据
    String getIcbaData();
    
    // 获取宇航员数据
    String getAstronautsData();
    
    // 获取APRS数据
    String getAprsData();
    
    // 获取是否首次启动状态
    bool getIsFirstStartup();
    
    // 设置是否首次启动状态
    void setIsFirstStartup(bool value);
    
    // 从服务器获取天气数据
    bool fetchWeatherData(String &result);
    
    // 从服务器获取新闻数据
    bool fetchNewsData(String &result);
    
    // 从服务器获取ICBA数据
    bool fetchIcbaData(String &result);
    
    // 从服务器获取宇航员数据
    bool fetchAstronautsData(String &result);
    
    // 从服务器获取APRS数据
    bool fetchAprsData(String &result);
    
    // 文件系统相关函数
    void initFileSystem();     // 初始化文件系统
    void saveCacheData();      // 保存缓存数据到文件
    void loadCacheData();      // 从文件加载缓存数据

};

#endif // DATA_MANAGER_H