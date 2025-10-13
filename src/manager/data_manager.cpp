/**
 * 数据管理器实现
 */
#include "data_manager.h"
#include "config/config.h"
#include "config/config_manager.h"
#include "network/net_http.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <time.h>
#include <algorithm>

// 定义单例实例
DataManager* DataManager::instance = nullptr;

// 定义任务标志
volatile bool DataManager::shouldUpdateAllData = false;
volatile bool DataManager::shouldUpdateICIBADaily = false;
volatile bool DataManager::shouldUpdateAstronauts = false;
volatile bool DataManager::shouldUpdateNews = false;

/**
 * 私有构造函数
 */
DataManager::DataManager() {
    // 初始化缓存更新时间
    lastIcibaUpdateDate = 0;
    lastAstronautsUpdateDate = 0;
    lastNewsUpdateTime = 0;
    
    // 初始化强制刷新标志
    forceICIBARefresh = false;
    forceAstronautsRefresh = false;
    forceNewsRefresh = false;
    
    // 初始化数据缓存
    icibaData = "";
    astronautsData = "";
    newsData = "";
    
    // 初始化状态标志
    isFirstStartup = true;
    isTimeValid = false;
    firstStartupWaitTime = 0;
    
    // 初始化任务句柄
    dataTaskHandle = nullptr;
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
    Serial.println("初始化数据管理器");
    
    // 初始化文件系统
    initFileSystem();
    
    // 加载缓存数据
    loadCacheData();
    
    // 创建数据任务
    xTaskCreatePinnedToCore(
        dataTask,
        "dataTask",
        4096,
        this,
        1,
        &dataTaskHandle,
        1 // 在CORE_1上运行
    );
    
    // 记录首次启动时间
    firstStartupWaitTime = millis();
}

/**
 * 获取当前日期 (YYYYMMDD格式)
 */
unsigned int DataManager::getCurrentDate() {
    if (!checkTimeValidity()) {
        return 0;
    }
    
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    return (timeinfo.tm_year + 1900) * 10000 + 
           (timeinfo.tm_mon + 1) * 100 + 
           timeinfo.tm_mday;
}

/**
 * 检查NTP时间是否有效
 */
bool DataManager::checkTimeValidity() {
    if (isTimeValid) {
        return true;
    }
    
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // 检查年份是否大于2000年
    if (timeinfo.tm_year > 100) {
        isTimeValid = true;
        return true;
    }
    
    return false;
}

/**
 * 检查并更新所有需要的缓存数据
 */
void DataManager::checkAndUpdateAllCaches() {
    checkAndUpdateCache(ICIBA_DATA);
    checkAndUpdateCache(ASTRONAUTS_DATA);
    checkAndUpdateCache(NEWS_DATA);
}

/**
 * 检查并更新特定类型的数据缓存（无参数版本）
 */
void DataManager::checkAndUpdateCache() {
    // 这个函数可能是为了向后兼容保留的
    checkAndUpdateAllCaches();
}

/**
 * 检查并更新特定类型的数据缓存（带参数版本）
 */
bool DataManager::checkAndUpdateCache(DataType type) {
    // 首次启动时等待一段时间
    if (isFirstStartup && millis() - firstStartupWaitTime < 5000) {
        return false;
    }
    
    bool result = false;
    unsigned long currentTime = millis();
    
    switch (type) {
        case ICIBA_DATA: {
            unsigned int currentDate = getCurrentDate();
            if (forceICIBARefresh || 
                (isTimeValid && (currentDate != 0 && currentDate != lastIcibaUpdateDate))) {
                result = fetchData(ICIBA_DATA, icibaData);
                if (result) {
                    lastIcibaUpdateDate = currentDate;
                    forceICIBARefresh = false;
                    Serial.println("每日一句更新成功");
                    saveCacheData();
                }
            }
            break;
        }
            
        case ASTRONAUTS_DATA: {
            unsigned int currentAstroDate = getCurrentDate();
            if (forceAstronautsRefresh || 
                (isTimeValid && (currentAstroDate != 0 && currentAstroDate != lastAstronautsUpdateDate))) {
                result = fetchData(ASTRONAUTS_DATA, astronautsData);
                if (result) {
                    lastAstronautsUpdateDate = currentAstroDate;
                    forceAstronautsRefresh = false;
                    Serial.println("宇航员数据更新成功");
                    saveCacheData();
                }
            }
            break;
        }
            
        case NEWS_DATA: {
            unsigned long currentTime = millis();
            if (forceNewsRefresh || 
                (isTimeValid && (lastNewsUpdateTime == 0 || currentTime - lastNewsUpdateTime >= CACHE_TIME_30MIN))) {
                result = fetchData(NEWS_DATA, newsData);
                if (result) {
                    lastNewsUpdateTime = currentTime;
                    forceNewsRefresh = false;
                    Serial.println("新闻数据更新成功");
                    saveCacheData();
                }
            }
            break;
        }
            
        default:
            break;
    }
    
    return result;
}

/**
 * 强制刷新所有数据
 */
void DataManager::forceRefreshAllData() {
    forceICIBARefresh = true;
    forceAstronautsRefresh = true;
    forceNewsRefresh = true;
    
    // 立即检查更新
    checkAndUpdateAllCaches();
}

/**
 * 通用HTTP请求函数
 * 发送GET请求并返回响应内容
 */
String DataManager::httpGetRequest(const char* url) {
    // 检查WiFi连接状态
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("DataManager: WiFi未连接，无法发送HTTP请求");
        return "";
    }
    HTTPClient http;
    String response = "";
    Serial.printf("DataManager: 发送HTTP请求到 %s\n", url);
    // 开始HTTP连接
    http.begin(url);
    // 设置超时时间
    http.setTimeout(10000);
    // 发送GET请求
    int httpCode = http.GET();
    // 检查响应状态
    if (httpCode > 0) {
        // 请求成功
        if (httpCode == HTTP_CODE_OK) {
            // 获取响应内容
            response = http.getString();
            Serial.println("DataManager: HTTP请求成功");
        } else {
            Serial.printf("DataManager: HTTP请求失败，错误码: %d\n", httpCode);
        }
    } else {
        // 请求失败
        Serial.printf("DataManager: HTTP连接失败，错误: %s\n", http.errorToString(httpCode).c_str());
    }
    // 关闭连接
    http.end();  
    return response;
}

/**
 * 数据获取函数 - 支持不同类型的数据获取
 */
bool DataManager::fetchData(DataType type, String &result) {
    String url = "";
    
    switch (type) {
        case ICIBA_DATA:     url = ICIBA_API_URL;     break;// 实现每日一句数据获取逻辑 
        case ASTRONAUTS_DATA:url = ASTRONAUTS_API_URL;break;// 实现宇航员数据获取逻辑 
        case NEWS_DATA:      url = NEWS_API_URL;      break;// 实现新闻数据获取逻辑 
        default:
            Serial.println("DataManager: 未知的数据类型");
            return false;
    }
    
    // 使用通用HTTP请求函数
    String response = httpGetRequest(url.c_str());
    if (response.length() > 0) {
        result = response;
        return true;
    }
    return false;
}

/**
 * 数据任务函数（在第二个核心运行）
 */
void DataManager::dataTask(void *pvParameters) {
    DataManager* dataManager = static_cast<DataManager*>(pvParameters);
    
    while (true) {
        // 检查并更新数据（根据标志或时间间隔）
        if (shouldUpdateAllData) {
            dataManager->checkAndUpdateAllCaches();
            shouldUpdateAllData = false;
        }
        
        if (shouldUpdateICIBADaily) {
            dataManager->checkAndUpdateCache(ICIBA_DATA);
            shouldUpdateICIBADaily = false;
        }
        
        if (shouldUpdateAstronauts) {
            dataManager->checkAndUpdateCache(ASTRONAUTS_DATA);
            shouldUpdateAstronauts = false;
        }
        
        if (shouldUpdateNews) {
            dataManager->checkAndUpdateCache(NEWS_DATA);
            shouldUpdateNews = false;
        }
        
        // 检查准点更新
        dataManager->checkAndUpdateOnTheHour();
        
        // 短暂延迟，降低CPU使用率
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * 初始化文件系统
 */
void DataManager::initFileSystem() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS挂载失败");
        return;
    }
    
    Serial.println("SPIFFS挂载成功");
    
    // 检查是否是首次启动
    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("首次启动，创建配置文件");
        isFirstStartup = true;
    } else {
        configFile.close();
        isFirstStartup = false;
    }
}

/**
 * 保存缓存数据到文件
 */
void DataManager::saveCacheData() {
    internalSaveCacheData();
}

/**
 * 从文件加载缓存数据
 */
void DataManager::loadCacheData() {
    internalLoadCacheData();
}

/**
 * 内部文件保存函数
 */
void DataManager::internalSaveCacheData() {
    // 使用新的saveDataToJsonFile函数重构数据保存逻辑
    if (!icibaData.isEmpty()) {
        saveDataToJsonFile("/iciba.json", icibaData, false);
    }
    
    if (!astronautsData.isEmpty()) {
        saveDataToJsonFile("/astronauts.json", astronautsData, false);
    }
    
    if (!newsData.isEmpty()) {
        saveDataToJsonFile("/news.json", newsData, false);
    }
    
    Serial.println("缓存数据保存完成");
}

/**
 * 内部文件加载函数
 */
void DataManager::internalLoadCacheData() {
    // 从SPIFFS加载缓存数据
    File icibaFile = SPIFFS.open("/iciba.json", "r");
    if (icibaFile) {
        icibaData = icibaFile.readString();
        icibaFile.close();
        Serial.println("每日一句缓存加载成功");
    }
    
    File astronautsFile = SPIFFS.open("/astronauts.json", "r");
    if (astronautsFile) {
        astronautsData = astronautsFile.readString();
        astronautsFile.close();
        Serial.println("宇航员缓存加载成功");
    }
    
    File newsFile = SPIFFS.open("/news.json", "r");
    if (newsFile) {
        newsData = newsFile.readString();
        newsFile.close();
        Serial.println("新闻缓存加载成功");
    }
}

/**
 * 检查是否是两小时整点时刻
 */
bool DataManager::isTwoHourIntervalOnTheHour() {
    if (!checkTimeValidity()) {
        return false;
    }
    
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // 检查是否是整点，且是偶数小时或0点
    return (timeinfo.tm_min == 0 && timeinfo.tm_sec < 10) && 
           (timeinfo.tm_hour % 2 == 0);
}

/**
 * 准点检查并更新数据
 */
void DataManager::checkAndUpdateOnTheHour() {
    if (isTwoHourIntervalOnTheHour()) {
        // 在整点时更新数据 - 新闻功能已移除
        shouldUpdateAllData = true;
    }
}

/**
 * 为数据获取连接WiFi
 */
bool DataManager::connectWiFiForDataFetch() {
    // 实现WiFi连接逻辑
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }
    
    Serial.println("正在连接WiFi进行数据获取...");
    
    // 使用ConfigManager获取WiFi配置
    ConfigManager* configManager = ConfigManager::getInstance();
    String ssid = "";
    String password = "";
    
    if (configManager->isConfigLoaded()) {
        configManager->getWiFiConfig(ssid, password);
    }
    
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int retry = 0;
    const int maxRetry = 10;
    
    while (WiFi.status() != WL_CONNECTED && retry < maxRetry) {
        delay(500);
        Serial.print(".");
        retry++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi连接成功");
        return true;
    } else {
        Serial.println("WiFi连接失败");
        return false;
    }
}

/**
 * 数据获取后断开WiFi
 * 注意：仅在特殊情况下断开，正常模式下保持连接
 */
void DataManager::disconnectWiFiAfterDataFetch() {
    // 实现WiFi断开逻辑 - 仅在特定情况下断开WiFi
    // 正常模式下保持WiFi连接，避免频繁重连导致系统不稳定
    // WiFi.disconnect(true);
    // Serial.println("WiFi已断开连接");
}

/**
 * 创建标准JSON响应结构
 * 统一的格式: {"code": 200, "msg": "success", "result": {}}
 */
void DataManager::createStandardJsonResponse(JsonDocument& doc, JsonDocument& resultData) {
    doc.clear();
    doc["code"] = 200;
    doc["msg"] = "success";
    
    // 将resultData的内容复制到doc["result"]
    if (resultData.is<JsonObject>()) {
        JsonObjectConst resultObj = resultData.as<JsonObjectConst>();
        for (JsonPairConst kv : resultObj) {
            doc["result"][kv.key().c_str()] = kv.value();
        }
    } else if (resultData.is<JsonArray>()) {
        JsonArrayConst resultArray = resultData.as<JsonArrayConst>();
        for (JsonVariantConst element : resultArray) {
            doc["result"].add(element);
        }
    }
    
    // 添加更新时间
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char timeString[20];
    sprintf(timeString, "%04d-%02d-%02d %02d:%02d:%02d", 
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    doc["result"]["last_updated"] = timeString;
}

/**
 * 保存数据到JSON文件
 * 直接将原始JSON数据写入文件，不进行包装处理
 * 仅在数据有效且处理成功的情况下才覆盖原始文件
 */
bool DataManager::saveDataToJsonFile(const String& filename, const String& data, bool needWrapper) {
    // 检查数据是否为空
    if (data.isEmpty()) {
        Serial.println("DataManager: 数据为空，不保存到" + filename);
        return false;
    }
    
    // 验证数据是否有效
    // 检查是否包含错误信息
    if (data.indexOf("请求失败") != -1 || 
        data.indexOf("error") != -1 || 
        data.indexOf("failed") != -1 || 
        data.indexOf("ERROR") != -1 || 
        data.indexOf("FAILED") != -1) {
        Serial.println("DataManager: 数据包含错误信息，不保存到" + filename);
        return false;
    }
    
    // 解析JSON数据以验证格式有效性
    JsonDocument dataDoc;
    DeserializationError error = deserializeJson(dataDoc, data);
    if (error) {
        Serial.println("DataManager: JSON解析失败: " + String(error.c_str()));
        return false;
    }
    
    // 针对不同文件类型的特殊验证
    if (filename.endsWith("iciba.json")) {
        // 验证每日一句数据是否有效
        if (!dataDoc.containsKey("note")) {
            Serial.println("DataManager: 每日一句数据无效，缺少必要字段");
            return false;
        }
    }
    
    // 写入文件 - 现在可以安全地写入，因为数据已经过验证
    // 注意：这里直接使用原始数据，不进行createStandardJsonResponse处理
    File file = SPIFFS.open(filename, "w");
    if (!file) {
        Serial.println("DataManager: 无法打开文件进行写入: " + filename);
        return false;
    }
    
    // 直接写入原始数据字符串
    if (file.print(data) == 0) {
        Serial.println("DataManager: 写入文件失败: " + filename);
        file.close();
        return false;
    }
    
    file.close();
    Serial.println("DataManager: 成功保存原始数据到" + filename);
    return true;
}