#include "manager/data_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "data/mock_data.h"
#include "network/net_http.h"

// 外部函数声明
extern bool writeJsonToFile(const String& filename, const JsonDocument& doc);
extern bool readJsonFromFile(const String& filename, JsonDocument& doc);

/**
 * 获取当前日期，格式为YYYYMMDD
 */
unsigned int DataManager::getCurrentDate() {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // 构造YYYYMMDD格式的日期
    return (timeinfo.tm_year + 1900) * 10000 + (timeinfo.tm_mon + 1) * 100 + timeinfo.tm_mday;
}

// 定义单例实例
DataManager* DataManager::instance = nullptr;

/**
 * 私有构造函数
 */
DataManager::DataManager() : 
    lastWeatherUpdateTime(0),
    lastNewsUpdateTime(0),
    lastIcibaUpdateTime(0),
    lastAstronautsUpdateTime(0),
    lastAPRSUpdateTime(0),
    lastIcibaUpdateDate(0),
    lastAstronautsUpdateDate(0),
    isFirstStartup(true),
    forceWeatherRefresh(false),
    forceNewsRefresh(false),
    forceIcibaRefresh(false),
    forceAstronautsRefresh(false),
    forceAPRSRefresh(false) { 
    // 初始化数据缓存
    weatherData = "";
    newsData = "";
    icbaData = "";
    astronautsData = "";
    aprsData = "";
    
    // 初始化文件系统
    initFileSystem();
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
    Serial.println("DataManager: 初始化数据管理器...");
    
    // 初始化缓存时间
    lastWeatherUpdateTime = 0;
    lastNewsUpdateTime = 0;
    lastIcibaUpdateTime = 0;
    lastAstronautsUpdateTime = 0;
    lastAPRSUpdateTime = 0;
    
    // 初始化日期变量为0，表示尚未更新过
    lastIcibaUpdateDate = 0;
    lastAstronautsUpdateDate = 0;
    
    // 初始化强制刷新标志
    forceWeatherRefresh = false;
    forceNewsRefresh = false;
    forceIcibaRefresh = false;
    forceAstronautsRefresh = false;
    forceAPRSRefresh = false;
    
    // 系统启动后只初始化变量，不立即获取所有数据
    // 其他数据将从本地JSON文件读取
    Serial.println("DataManager: 系统启动后，只初始化变量，将从本地JSON文件读取数据");
}

/**
 * 检查并更新所有需要的缓存数据
 */
void DataManager::checkAndUpdateAllCaches() {
    // 后台异步更新数据，避免阻塞UI
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
            {
                unsigned int currentDate = getCurrentDate();
                if (isFirstStartup || currentDate != lastIcibaUpdateDate) {
                    updated = fetchIcbaData(icbaData);
                    if (updated) {
                        lastIcibaUpdateDate = currentDate;
                        lastWeatherUpdateTime = currentTime; // 保留原有时间戳以便兼容
                    }
                }
            }
            break;
        
        case ASTRONAUTS_DATA:
            {
                unsigned int currentDate = getCurrentDate();
                if (isFirstStartup || currentDate != lastAstronautsUpdateDate) {
                    updated = fetchAstronautsData(astronautsData);
                    if (updated) {
                        lastAstronautsUpdateDate = currentDate;
                        lastAstronautsUpdateTime = currentTime; // 保留原有时间戳以便兼容
                    }
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
    
    // 保留更新状态，但不进行文件保存
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
    
    // 设置请求头中的User-Agent和Referer
    http.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
    http.addHeader("Referer", "http://apis.tianapi.com/");
    
    // 发送HTTP请求
    int httpCode = http.GET();
    
    // 检查请求是否成功
    if (httpCode > 0) {
        response = http.getString();
    }
    
    // 关闭连接
    http.end();
    
    return response;
}
String DataManager::httpGetRequestNews(const char* url) {
    HTTPClient http;
    String response = "";
    
    // 检查WiFi连接状态
    if (WiFi.status() != WL_CONNECTED) {
        return response;
    }
    
    // 开始HTTP请求
    http.begin(url);
    
    // 发送HTTP请求
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
    String url = String(API_URL) + "?city=" + String(CITY_CODE) + "&key=" + String(WEATHER_API_KEY);
    String response = httpGetRequest(url.c_str());
    
    if (response.length() > 0) {
        result = response;
        return true;
    }
    
    return false;
}

/**
 * 从服务器获取新闻数据（重构版）
 * 每2小时从apis.tianapi.com获取每日新闻简报列表
 */
bool DataManager::fetchNewsData(String &result) {
    // 检查WiFi连接状态
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("DataManager: WiFi未连接，无法获取新闻数据");
        return false;
    }
    
    // 构造API请求URL
    String url = String(NEWS_API_URL) + "?key=" + String(NEWS_API_KEY);
    Serial.print("DataManager: 请求新闻数据URL: ");
    Serial.println(url);
    
    // 发送HTTP请求
    String response = httpGetRequestNews(url.c_str());
    
    // 检查响应是否为空
    if (response.length() == 0) {
        Serial.println("DataManager: 获取新闻数据失败，响应为空");
        return false;
    }
    
    // 解析JSON数据
    JsonDocument newsData;
    DeserializationError error = deserializeJson(newsData, response);
    
    if (error) {
        Serial.print("DataManager: 反序列化新闻JSON失败: ");
        Serial.println(error.f_str());
        Serial.print("DataManager: 原始响应数据: ");
        Serial.println(response);
        return false;
    }
    
    // 检查API响应状态
    if (newsData.containsKey("code") && newsData.containsKey("msg")) {
        int code = newsData["code"].as<int>();
        const char* msg = newsData["msg"].as<const char*>();
        
        if (code != 200) {
            Serial.printf("DataManager: API返回错误 - 代码: %d, 消息: %s\n", code, msg);
            return false;
        }
        
        Serial.printf("DataManager: API返回成功 - 消息: %s\n", msg);
    }
    
    // 创建用于保存的JSON对象
    JsonDocument saveDoc;
    
    // 提取新闻列表数据
    if (newsData.containsKey("result") && newsData["result"].is<JsonObject>() && 
        newsData["result"].containsKey("list") && newsData["result"]["list"].is<JsonArray>()) {
        
        // 创建result结构
        JsonObject resultObj = saveDoc.createNestedObject("result");
        
        // 限制新闻数量为前16条
        JsonArray newsList = resultObj.createNestedArray("list");
        JsonArray sourceList = newsData["result"]["list"].as<JsonArray>();
        int newsCount = min(16, static_cast<int>(sourceList.size()));
        
        // 仅提取每条新闻的标题字段
        for (int i = 0; i < newsCount; i++) {
            JsonObject newsItem = sourceList[i].as<JsonObject>();
            JsonObject saveItem = newsList.createNestedObject();
            
            if (newsItem.containsKey("title")) {
                saveItem["title"] = newsItem["title"].as<const char*>();
            }
        }
        
        // 添加最后更新时间
        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        char timeString[20];
        sprintf(timeString, "%04d-%02d-%02d %02d:%02d:%02d", 
                timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        resultObj["last_updated"] = timeString;
        
        Serial.printf("DataManager: 成功提取%d条新闻（仅标题）\n", newsCount);
    } else {
        Serial.println("DataManager: 新闻数据格式不正确，不包含result.list数组");
        return false;
    }
    
    // 将处理后的新闻数据写入JSON文件
    bool writeResult = writeJsonToFile("/news.json", saveDoc);
    if (writeResult) {
        Serial.println("DataManager: 新闻数据（仅标题）成功写入news.json文件");
        
        // 记录缓存更新时间
        lastNewsUpdateTime = millis();
        return true;
    } else {
        Serial.println("DataManager: 新闻数据写入news.json文件失败");
        return false;
    }
}

/**
 * 从服务器获取ICBA数据
 */
bool DataManager::fetchIcbaData(String &result) {
    String url = ICIBA_API_URL;
    String response = httpGetRequest(url.c_str());
    
    if (response.length() > 0) {
        // 直接返回原始响应
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
    Serial.println("DataManager: 初始化文件系统...");
    
    // 初始化SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("DataManager: SPIFFS挂载失败");
        return;
    }
    
    Serial.println("DataManager: SPIFFS挂载成功");
    
    // 加载缓存数据
    loadCacheData();
    
    // 创建模拟数据文件
    createAllMockData();
}

/**
 * 保存缓存数据到文件
 */
void DataManager::saveCacheData() {
    Serial.println("DataManager: 保存缓存数据到文件...");
    
    // 获取当前时间作为更新时间戳
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char timeString[20];
    sprintf(timeString, "%04d-%02d-%02d %02d:%02d:%02d", 
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    // 保存天气数据
    if (!weatherData.isEmpty()) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, weatherData);
        if (!error) {
            // 添加或更新last_updated字段
            if (doc.containsKey("result")) {
                doc["result"]["last_updated"] = timeString;
            }
            writeJsonToFile("/weather.json", doc);
        }
    }
    
    // 保存新闻数据
    if (!newsData.isEmpty()) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, newsData);
        if (!error) {
            // 检查是否需要包装成标准格式
            if (!doc.containsKey("result")) {
                // 为新闻数据创建标准的result结构
                JsonDocument wrappedDoc;
                wrappedDoc["result"] = doc;
                
                // 添加更新时间
                wrappedDoc["result"]["last_updated"] = timeString;
                
                writeJsonToFile("/news.json", wrappedDoc);
            } else {
                // 如果已经有result结构，直接添加更新时间
                if (doc.containsKey("result")) {
                    doc["result"]["last_updated"] = timeString;
                }
                writeJsonToFile("/news.json", doc);
            }
        }
    }
    
    // 保存ICBA数据
    if (!icbaData.isEmpty()) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, icbaData);
        if (!error) {
            // 添加或更新last_updated字段
            if (doc.containsKey("result")) {
                doc["result"]["last_updated"] = timeString;
            }
            writeJsonToFile("/iciba.json", doc);
        }
    }
    
    // 保存宇航员数据
    if (!astronautsData.isEmpty()) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, astronautsData);
        if (!error) {
            // 检查是否需要包装成标准格式
            if (!doc.containsKey("result")) {
                // 为宇航员数据创建标准的result结构
                JsonDocument wrappedDoc;
                wrappedDoc["result"]["astronauts"] = doc["people"];
                wrappedDoc["result"]["total"] = doc["number"];
                
                // 添加更新时间
                wrappedDoc["result"]["last_updated"] = timeString;
                
                writeJsonToFile("/astronauts.json", wrappedDoc);
            } else {
                // 如果已经有result结构，直接添加更新时间
                if (doc.containsKey("result")) {
                    doc["result"]["last_updated"] = timeString;
                }
                writeJsonToFile("/astronauts.json", doc);
            }
        }
    }
    
    // 保存APRS数据
    if (!aprsData.isEmpty() && aprsData != "APRS数据暂未实现") {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, aprsData);
        if (!error) {
            // 添加或更新last_updated字段
            doc["last_updated"] = timeString;
            writeJsonToFile("/aprs.json", doc);
        }
    }
    
    Serial.println("DataManager: 缓存数据保存完成");
}

/**
 * 从文件加载缓存数据
 */
void DataManager::loadCacheData() {
    Serial.println("DataManager: 从文件加载缓存数据...");
    
    // 从文件加载天气数据
    JsonDocument weatherDoc;
    if (readJsonFromFile("/weather.json", weatherDoc)) {
        String jsonString;
        serializeJson(weatherDoc, jsonString);
        weatherData = jsonString;
        Serial.println("DataManager: 已加载天气数据");
    }
    
    // 从文件加载新闻数据
    JsonDocument newsDoc;
    if (readJsonFromFile("/news.json", newsDoc)) {
        String jsonString;
        serializeJson(newsDoc, jsonString);
        newsData = jsonString;
        Serial.println("DataManager: 已加载新闻数据");
    }
    
    // 从文件加载ICBA数据
    JsonDocument icibaDoc;
    if (readJsonFromFile("/iciba.json", icibaDoc)) {
        String jsonString;
        serializeJson(icibaDoc, jsonString);
        icbaData = jsonString;
        Serial.println("DataManager: 已加载ICBA数据");
    }
    
    // 从文件加载宇航员数据
    JsonDocument astronautsDoc;
    if (readJsonFromFile("/astronauts.json", astronautsDoc)) {
        String jsonString;
        serializeJson(astronautsDoc, jsonString);
        astronautsData = jsonString;
        Serial.println("DataManager: 已加载宇航员数据");
    }
    
    // 从文件加载APRS数据
    JsonDocument aprsDoc;
    if (readJsonFromFile("/aprs.json", aprsDoc)) {
        String jsonString;
        serializeJson(aprsDoc, jsonString);
        aprsData = jsonString;
        Serial.println("DataManager: 已加载APRS数据");
    }
    
    Serial.println("DataManager: 缓存数据加载完成");
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