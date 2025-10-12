#include "data/mock_data.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "network/net_http.h"

// 声明外部函数
extern bool writeJsonToFile(const String& filename, const JsonDocument& doc);

/**
 * 创建模拟宇航员数据
 */
void createMockAstronautsData() {
    Serial.println("创建模拟宇航员数据");
    
    // 检查文件是否已存在
    if (SPIFFS.exists("/astronauts.json")) {
        Serial.println("宇航员数据文件已存在，跳过创建模拟数据");
        return;
    }
    
    // 创建模拟数据
    JsonDocument astronautsJson;
    
    // 设置状态信息
    astronautsJson["code"] = 200;
    astronautsJson["msg"] = "success";
    
    // 创建result对象
    JsonObject resultObj = astronautsJson["result"].to<JsonObject>();
    resultObj["total"] = 10; // 假设有10名宇航员
    
    JsonArray astronauts = resultObj["astronauts"].to<JsonArray>();
    
    // 添加模拟宇航员数据
    JsonObject person1 = astronauts.add<JsonObject>();
    person1["name"] = "张泉灵";
    person1["craft"] = "中国空间站";
    
    JsonObject person2 = astronauts.add<JsonObject>();
    person2["name"] = "Mark Vande Hei";
    person2["craft"] = "国际空间站";
    
    JsonObject person3 = astronauts.add<JsonObject>();
    person3["name"] = "Pyotr Dubrov";
    person3["craft"] = "国际空间站";
    
    JsonObject person4 = astronauts.add<JsonObject>();
    person4["name"] = "Anton Shkaplerov";
    person4["craft"] = "国际空间站";
    
    // 写入文件
    writeJsonToFile("/astronauts.json", astronautsJson);
}

/**
 * 创建所有模拟数据文件
 */
void createAllMockData() {
    Serial.println("创建所有模拟数据文件");
    
    // 确保SPIFFS已初始化
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS挂载失败，无法创建模拟数据");
        return;
    }
    
    // 创建各个模拟数据文件
    createMockAstronautsData();
    
    Serial.println("模拟数据创建完成");
}