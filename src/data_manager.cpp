#include "data_manager.h"

// 添加文件系统支持
#include <FS.h>
#include <LittleFS.h>

// API URL常量定义
#define API_URL "http://apis.juhe.cn/simpleWeather/query"
#define CITY_CODE "101110101"
#define WEATHER_API_KEY "6b153057e392e40a1b39e13b30b925e0"
#define NEWS_API_URL "http://api.tianapi.com/allnews/index"
#define NEWS_API_KEY API_KEY
#define QUOTE_API_URL "http://api.tianapi.com/aphorism/index"
#define ASTRONAUTS_API_URL "http://api.open-notify.org/astros.json"

// 定义单例实例
DataManager* DataManager::instance = nullptr;

/**
 * 私有构造函数
 */
DataManager::DataManager() : 
    lastWeatherUpdateTime(0),
    lastNewsUpdateTime(0),
    lastQuoteUpdateTime(0),
    lastAstronautsUpdateTime(0),
    lastAPRSUpdateTime(0),
    isFirstStartup(true) {
    // 初始化数据缓存
    weatherData = "";
    newsData = "";
    quoteData = "";
    icbaData = "";
    astronautsData = "";
    aprsData = "";

    // 初始化文件系统
    initFileSystem();
    
    // 从文件加载缓存数据
    loadCacheData();
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
    
    // 检查并更新每日一句
    checkAndUpdateCache(QUOTE_DATA);
    
    // 检查并更新ICBA数据
    checkAndUpdateCache(ICBA_DATA);
    
    // 检查并更新宇航员数据
    checkAndUpdateCache(ASTRONAUTS_DATA);
    
    // 检查并更新APRS数据
    checkAndUpdateCache(APRS_DATA);
    
    // 首次启动标志设置为false
    isFirstStartup = false;
    
    // 保存所有缓存数据到文件
    saveCacheData();
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
        
        case QUOTE_DATA:
            if (isFirstStartup || (currentTime - lastQuoteUpdateTime >= CACHE_TIME_2HOURS)) {
                updated = fetchQuoteData(quoteData);
                if (updated) {
                    lastQuoteUpdateTime = currentTime;
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
                    saveCacheData(); // 保存更新后的数据到文件
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
 * 获取每日一句
 */
String DataManager::getQuoteData() {
    // 检查缓存是否需要更新
    checkAndUpdateCache(QUOTE_DATA);
    return quoteData;
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
 * 从服务器获取每日一句
 */
bool DataManager::fetchQuoteData(String &result) {
    String url = String(QUOTE_API_URL);
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
    const int MAX_RETRIES = 3;
    int retry = 0;
    bool initialized = false;
    
    while (retry < MAX_RETRIES && !initialized) {
        if (!LittleFS.begin(true)) { // true表示格式化失败时重新格式化
            Serial.printf("LittleFS初始化失败！重试 (%d/%d)...\n", retry + 1, MAX_RETRIES);
            retry++;
            delay(500); // 等待一段时间后重试
        } else {
            initialized = true;
            Serial.println("LittleFS初始化成功！");
        }
    }
    
    if (!initialized) {
        Serial.println("LittleFS初始化失败，尝试手动格式化...");
        if (LittleFS.format()) {
            Serial.println("文件系统格式化成功，再次尝试初始化...");
            if (LittleFS.begin()) {
                Serial.println("LittleFS初始化成功！");
            } else {
                Serial.println("严重错误：LittleFS初始化彻底失败！");
            }
        } else {
            Serial.println("严重错误：文件系统格式化失败！");
        }
    }
}

/**
 * 保存缓存数据到文件
 */
void DataManager::saveCacheData() {
    const int MAX_RETRIES = 3;
    int retry = 0;
    bool saved = false;
    
    while (retry < MAX_RETRIES && !saved) {
        // 检查文件系统是否已挂载
        if (!LittleFS.exists("/")) {
            Serial.println("文件系统未挂载，尝试重新初始化...");
            initFileSystem();
            delay(500);
            retry++;
            continue;
        }
        
        // 创建一个文件来存储所有缓存数据
        File file = LittleFS.open("/cache_data.json", "w");
        if (!file) {
            Serial.printf("打开缓存文件失败，无法保存数据！重试 (%d/%d)...\n", retry + 1, MAX_RETRIES);
            retry++;
            delay(500);
            continue;
        }
        
        try {
            // 创建JSON对象来存储所有数据
            DynamicJsonDocument doc(4096); // 分配足够的内存
            
            // 存储数据
            doc["weatherData"] = weatherData;
            doc["newsData"] = newsData;
            doc["quoteData"] = quoteData;
            doc["icbaData"] = icbaData;
            doc["astronautsData"] = astronautsData;
            doc["aprsData"] = aprsData;
            
            // 存储更新时间
            doc["lastWeatherUpdateTime"] = lastWeatherUpdateTime;
            doc["lastNewsUpdateTime"] = lastNewsUpdateTime;
            doc["lastQuoteUpdateTime"] = lastQuoteUpdateTime;
            doc["lastAstronautsUpdateTime"] = lastAstronautsUpdateTime;
            doc["lastAPRSUpdateTime"] = lastAPRSUpdateTime;
            
            // 序列化JSON并写入文件
            serializeJson(doc, file);
            file.close();
            
            Serial.println("缓存数据已保存到文件！");
            saved = true;
        } catch (const std::exception& e) {
            Serial.printf("保存缓存数据时发生异常: %s\n", e.what());
            file.close();
            retry++;
            delay(500);
        }
    }
    
    if (!saved) {
        Serial.println("严重错误：缓存数据保存失败！");
    }
}

/**
 * 从文件加载缓存数据
 */
void DataManager::loadCacheData() {
    const int MAX_RETRIES = 3;
    int retry = 0;
    bool loaded = false;
    
    while (retry < MAX_RETRIES && !loaded) {
        // 检查文件系统是否已挂载
        if (!LittleFS.exists("/")) {
            Serial.println("文件系统未挂载，尝试重新初始化...");
            initFileSystem();
            delay(500);
            retry++;
            continue;
        }
        
        // 尝试打开缓存文件
        File file = LittleFS.open("/cache_data.json", "r");
        if (!file) {
            Serial.printf("未找到缓存文件，使用默认值！重试 (%d/%d)...\n", retry + 1, MAX_RETRIES);
            retry++;
            delay(500);
            continue;
        }
        
        try {
            // 读取文件内容
            DynamicJsonDocument doc(4096); // 分配足够的内存
            DeserializationError error = deserializeJson(doc, file);
            file.close();
            
            if (error) {
                Serial.printf("解析缓存文件失败：%s，使用默认值！重试 (%d/%d)...\n", error.c_str(), retry + 1, MAX_RETRIES);
                retry++;
                delay(500);
                continue;
            }
            
            // 从JSON对象中读取数据
            if (doc.containsKey("weatherData")) weatherData = doc["weatherData"].as<String>();
            if (doc.containsKey("newsData")) newsData = doc["newsData"].as<String>();
            if (doc.containsKey("quoteData")) quoteData = doc["quoteData"].as<String>();
            if (doc.containsKey("icbaData")) icbaData = doc["icbaData"].as<String>();
            if (doc.containsKey("astronautsData")) astronautsData = doc["astronautsData"].as<String>();
            if (doc.containsKey("aprsData")) aprsData = doc["aprsData"].as<String>();
            
            // 读取更新时间
            if (doc.containsKey("lastWeatherUpdateTime")) lastWeatherUpdateTime = doc["lastWeatherUpdateTime"].as<unsigned long>();
            if (doc.containsKey("lastNewsUpdateTime")) lastNewsUpdateTime = doc["lastNewsUpdateTime"].as<unsigned long>();
            if (doc.containsKey("lastQuoteUpdateTime")) lastQuoteUpdateTime = doc["lastQuoteUpdateTime"].as<unsigned long>();
            if (doc.containsKey("lastAstronautsUpdateTime")) lastAstronautsUpdateTime = doc["lastAstronautsUpdateTime"].as<unsigned long>();
            if (doc.containsKey("lastAPRSUpdateTime")) lastAPRSUpdateTime = doc["lastAPRSUpdateTime"].as<unsigned long>();
            
            Serial.println("缓存数据已从文件加载！");
            loaded = true;
        } catch (const std::exception& e) {
            Serial.printf("加载缓存数据时发生异常: %s\n", e.what());
            file.close();
            retry++;
            delay(500);
        }
    }
    
    if (!loaded && retry >= MAX_RETRIES) {
        Serial.println("警告：未能从文件加载缓存数据，使用默认值！");
        // 尝试创建一个空的缓存文件，以便下次保存
        File file = LittleFS.open("/cache_data.json", "w");
        if (file) {
            DynamicJsonDocument doc(1024);
            serializeJson(doc, file);
            file.close();
            Serial.println("已创建空的缓存文件！");
        }
    }
}