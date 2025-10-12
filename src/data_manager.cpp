#include "data_manager.h"

// API URL常量定义
#define API_URL "http://apis.juhe.cn/simpleWeather/query"
#define CITY_CODE "101110101"
#define WEATHER_API_KEY "6b153057e392e40a1b39e13b30b925e0"
#define NEWS_API_URL "http://api.tianapi.com/allnews/index"
#define NEWS_API_KEY API_KEY
#define ASTRONAUTS_API_URL "http://api.open-notify.org/astros.json"

// 定义单例实例
DataManager* DataManager::instance = nullptr;

/**
 * 私有构造函数
 */
DataManager::DataManager() : 
    lastWeatherUpdateTime(0),
    lastNewsUpdateTime(0),
    lastAstronautsUpdateTime(0),
    lastAPRSUpdateTime(0),
    isFirstStartup(true) { 
    // 初始化数据缓存
    weatherData = "";
    newsData = "";
    icbaData = "";
    astronautsData = "";
    aprsData = "";
    
    // 不初始化文件系统
    Serial.println("缓存数据将不存储到LittleFS");
    // 不加载缓存数据
}

/**
 * 获取单例实例
 */
DataManager* DataManager::getInstance() {
    if (instance == nullptr) {
        instance = new DataManager();
    }
    return instance;
}

/**
 * 初始化数据管理器
 */
void DataManager::init() {
    // 首次启动时立即获取所有数据
    checkAndUpdateAllCaches();
}

/**
 * 检查并更新所有需要的缓存数据
 */
void DataManager::checkAndUpdateAllCaches() {
    // 检查并更新天气数据
    checkAndUpdateCache(WEATHER_DATA);
    
    // 检查并更新新闻数据
    checkAndUpdateCache(NEWS_DATA);
    
    // 检查并更新ICBA数据
    checkAndUpdateCache(ICBA_DATA);
    
    // 检查并更新宇航员数据
    checkAndUpdateCache(ASTRONAUTS_DATA);
    
    // 检查并更新APRS数据
    checkAndUpdateCache(APRS_DATA);
    
    // 首次启动标志设置为false
    isFirstStartup = false;
    
    // 不保存缓存数据到文件
}

/**
 * 检查并更新特定类型的数据缓存
 */
bool DataManager::checkAndUpdateCache(DataType type) {
    bool updated = false;
    unsigned long currentTime = millis();
    
    switch (type) {
        case WEATHER_DATA:
            if (isFirstStartup || (currentTime - lastWeatherUpdateTime >= CACHE_TIME_2HOURS)) {
                updated = fetchWeatherData(weatherData);
                if (updated) {
                    lastWeatherUpdateTime = currentTime;
                }
            }
            break;
        
        case NEWS_DATA:
            if (isFirstStartup || (currentTime - lastNewsUpdateTime >= CACHE_TIME_2HOURS)) {
                updated = fetchNewsData(newsData);
                if (updated) {
                    lastNewsUpdateTime = currentTime;
                }
            }
            break;
        
        case ICBA_DATA:
            if (isFirstStartup || (currentTime - lastWeatherUpdateTime >= CACHE_TIME_2HOURS)) {
                updated = fetchIcbaData(icbaData);
                if (updated) {
                    lastWeatherUpdateTime = currentTime; // ICBA数据使用与天气相同的更新时间
                }
            }
            break;
        
        case ASTRONAUTS_DATA:
            if (isFirstStartup || (currentTime - lastAstronautsUpdateTime >= CACHE_TIME_30MIN)) {
                updated = fetchAstronautsData(astronautsData);
                if (updated) {
                    lastAstronautsUpdateTime = currentTime;
                }
            }
            break;
        

        
        case APRS_DATA:
            if (isFirstStartup || (currentTime - lastAPRSUpdateTime >= CACHE_TIME_30MIN)) {
                updated = fetchAprsData(aprsData);
                if (updated) {
                    lastAPRSUpdateTime = currentTime;
                    // 不保存缓存数据到文件
                }
            }
            break;
        
        default:
            break;
    }
    
    return updated;
}

/**
 * 强制刷新所有数据
 */
void DataManager::forceRefreshAllData() {
    // 设置为首次启动状态，强制所有数据重新加载
    isFirstStartup = true;
    checkAndUpdateAllCaches();
}

/**
 * 获取天气数据
 */
String DataManager::getWeatherData() {
    // 检查缓存是否需要更新
    checkAndUpdateCache(WEATHER_DATA);
    return weatherData;
}

/**
 * 获取新闻数据
 */
String DataManager::getNewsData() {
    // 检查缓存是否需要更新
    checkAndUpdateCache(NEWS_DATA);
    return newsData;
}

/**
 * 获取ICBA数据
 */
String DataManager::getIcbaData() {
    // 检查缓存是否需要更新
    checkAndUpdateCache(ICBA_DATA);
    return icbaData;
}

/**
 * 获取宇航员数据
 */
String DataManager::getAstronautsData() {
    // 检查缓存是否需要更新
    checkAndUpdateCache(ASTRONAUTS_DATA);
    return astronautsData;
}

/**
 * 获取APRS数据
 */
String DataManager::getAprsData() {
    // 检查缓存是否需要更新
    checkAndUpdateCache(APRS_DATA);
    return aprsData;
}

/**
 * 通用HTTP请求函数
 */
String DataManager::httpGetRequest(const char* url) {
    HTTPClient http;
    String response = "";
    
    // 检查WiFi连接状态
    if (WiFi.status() != WL_CONNECTED) {
        return response;
    }
    
    // 开始HTTP请求
    http.begin(url);
    int httpCode = http.GET();
    
    // 检查请求是否成功
    if (httpCode > 0) {
        response = http.getString();
    }
    
    // 关闭连接
    http.end();
    
    return response;
}

/**
 * 从服务器获取天气数据
 */
bool DataManager::fetchWeatherData(String &result) {
    String url = String(API_URL) + String(CITY_CODE) + "?key=" + String(WEATHER_API_KEY);
    String response = httpGetRequest(url.c_str());
    
    if (response.length() > 0) {
        result = response;
        return true;
    }
    
    return false;
}

/**
 * 从服务器获取新闻数据
 */
bool DataManager::fetchNewsData(String &result) {
    String url = String(NEWS_API_URL) + "?key=" + String(NEWS_API_KEY);
    String response = httpGetRequest(url.c_str());
    
    if (response.length() > 0) {
        result = response;
        return true;
    }
    
    return false;
}

/**
 * 从服务器获取ICBA数据
 */
bool DataManager::fetchIcbaData(String &result) {
    // 使用与天气相同的API，仅处理方式不同
    String url = String(API_URL) + String(CITY_CODE) + "?key=" + String(WEATHER_API_KEY);
    String response = httpGetRequest(url.c_str());
    
    if (response.length() > 0) {
        result = response;
        return true;
    }
    
    return false;
}

/**
 * 从服务器获取宇航员数据
 */
bool DataManager::fetchAstronautsData(String &result) {
    String url = String(ASTRONAUTS_API_URL);
    String response = httpGetRequest(url.c_str());
    
    if (response.length() > 0) {
        result = response;
        return true;
    }
    
    return false;
}

/**
 * 从服务器获取APRS数据
 */
bool DataManager::fetchAprsData(String &result) {
    // APRS数据获取可能需要特殊处理，这里暂时使用模拟数据
    // 实际项目中应该根据APRS服务器的API实现
    result = "APRS数据暂未实现";
    return true;
}

/**
 * 初始化文件系统
 */
void DataManager::initFileSystem() {
    Serial.println("文件系统初始化已禁用，缓存数据将不存储到LittleFS");
}

/**
 * 保存缓存数据到文件
 */
void DataManager::saveCacheData() {
    Serial.println("缓存数据保存功能已禁用，缓存数据将不存储到LittleFS");
}

/**
 * 从文件加载缓存数据
 */
void DataManager::loadCacheData() {
    Serial.println("缓存数据加载功能已禁用，缓存数据将不存储到LittleFS");
}

/**
 * 获取是否首次启动状态
 */
bool DataManager::getIsFirstStartup() {
    return isFirstStartup;
}

/**
 * 设置是否首次启动状态
 */
void DataManager::setIsFirstStartup(bool value) {
    isFirstStartup = value;
}