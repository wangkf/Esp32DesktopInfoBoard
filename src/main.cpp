#include "includes.h"
#include <SPIFFS.h>
#include "network/web_config_server.h"
#include "config/config_manager.h"

// 全局变量定义
const char* ntpServer = NTP_SERVER;

// LVGL对象声明
extern lv_obj_t* mao_select_label;
extern lv_obj_t* toxic_soul_label;
extern lv_obj_t* soul_label;        // 禅语哲言标签
extern lv_obj_t* iciba_label;
extern lv_obj_t* astronauts_label;

// 标记系统是否已经初始化
bool systemInitialized = false;

// 自动换屏相关变量
bool autoScreenChangeEnabled = true;
unsigned long lastScreenChangeTime = 0;

// 光线传感器相关变量
unsigned long lastBrightnessUpdateTime = 0;

// Web配置服务器相关变量
bool webConfigMode = false;
unsigned long webConfigStartTime = 0;
const unsigned long WEB_CONFIG_TIMEOUT = 300000; // 5分钟超时

// 声明全局字体
extern lv_font_t lvgl_font_digital_48;

// LED控制配置
#define LED_CHANNEL    0       // LED通道
#define LED_FREQ       5000    // LED频率
#define LED_RESOLUTION 8       // LED分辨率

// 初始化硬件
void initHardware() {
  Serial.begin(115200);
  Serial.println("初始化硬件...");
  
  // 初始化PSRAM
  if (psramInit()) {
    Serial.println("PSRAM initialized");
  } else {
    Serial.println("PSRAM initialization failed");
  }
  
  // 初始化SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS挂载失败");
  }
  
  // 初始化光线传感器引脚
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  
  // 初始化PWM引脚用于亮度控制
  ledcSetup(LED_CHANNEL, LED_FREQ, LED_RESOLUTION);
  ledcAttachPin(SCREEN_BRIGHTNESS_PIN, LED_CHANNEL);
  ledcWrite(LED_CHANNEL, 255); // 默认最大亮度
}

// 初始化WiFi和NTP
void initWiFiAndNTP() {
  Serial.println("初始化WiFi和NTP...");
  
  // 使用ConfigManager获取WiFi配置
  ConfigManager* configManager = ConfigManager::getInstance();
  String ssid = "";
  String password = "";
  
  if (configManager->isConfigLoaded()) {
    if (configManager->getWiFiConfig(ssid, password)) {
      Serial.println("已从配置管理器加载WiFi配置");
    } else {
      Serial.println("无法从配置管理器获取WiFi配置，使用默认值");
    }
  } else {
    Serial.println("配置管理器未初始化，使用默认WiFi配置");
  }
  
  // 连接WiFi
  WiFi.begin(ssid.c_str(), password.c_str());
  
  // 等待WiFi连接
  int wifiConnectTimeout = 15; // 15秒超时
  while (WiFi.status() != WL_CONNECTED && wifiConnectTimeout > 0) {
    delay(1000);
    Serial.print(".");
    wifiConnectTimeout--;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi连接成功");
    Serial.print("IP地址: ");
    Serial.println(WiFi.localIP());
    
    // 初始化mDNS
    if (!MDNS.begin("ESP32-InfoBoard")) {
      Serial.println("Error setting up MDNS responder!");
    } else {
      Serial.println("mDNS responder started");
      // 添加HTTP服务
      MDNS.addService("http", "tcp", 80);
    }
    
    // 初始化NTP - 从ConfigManager获取时区配置
    int timezone = configManager->getNTPServerTimezone();
    long gmtOffset_sec = timezone * 3600; // 将时区转换为秒
    const long daylightOffset_sec = 0; // 不使用夏令时
    
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    // 等待时间同步
    delay(2000);
    
    // 如果Web配置服务器未运行，则使用STA模式启动它
    if (!WebConfigServer::getInstance()->isServerRunning()) {
      WebConfigServer::getInstance()->start(false); // false表示使用STA模式
      
      // 显示当前IP地址在屏幕右上角
      char ipStr[20];
      WiFi.localIP().toString().toCharArray(ipStr, sizeof(ipStr));
      char combinedIpStr[90];
      snprintf(combinedIpStr, sizeof(combinedIpStr), "\uF012 http://%s 或 http://ESP32-InfoBoard 登录更改留言板", ipStr);
      TimeManager::getInstance()->setIpInfo(combinedIpStr, lv_color_hex(0x00FF00));
    }
  } else {
    Serial.println("\nWiFi连接失败");
    
    // WiFi连接失败，启动热点模式的Web服务器
    if (!webConfigMode) {
      webConfigMode = true;
      webConfigStartTime = millis();
      WebConfigServer::getInstance()->start(true); // true表示使用AP模式
      Serial.println("已启动热点模式Web配置，请连接热点ESP32-InfoBoard，访问192.168.4.1");
      
      // 在AP模式下也初始化mDNS
      if (!MDNS.begin("ESP32-InfoBoard")) {
        Serial.println("Error setting up MDNS responder in AP mode!");
      } else {
        Serial.println("mDNS responder started in AP mode");
        // 添加HTTP服务
        MDNS.addService("http", "tcp", 80);
      }
      
      // 显示热点IP地址在屏幕右上角
      TimeManager::getInstance()->setIpInfo("\uF013192.168.4.1", lv_color_hex(0xFF0000));
    }
  }
}

// 更新屏幕亮度
void updateBrightness() {
  if (millis() - lastBrightnessUpdateTime >= brightnessUpdateInterval) {
    int lightValue = analogRead(LIGHT_SENSOR_PIN);
    
    // 注释掉调试信息
    /*
    if (lightValue % 10 == 0) {  // 每10次更新才打印一次，避免过于频繁
      Serial.printf("光线传感器值: %d\n", lightValue);
    }
    */
    
    // 将传感器值映射到亮度值（0-255）
    // 环境越暗，屏幕越亮；环境越亮，屏幕越暗
    int brightness = map(lightValue, 0, 4095, 0, 255); 
    
    // 限制亮度值范围
    brightness = constrain(brightness, 1, 255);
    
    // 设置屏幕亮度
    ledcWrite(LED_CHANNEL, brightness);
    
    // 注释掉调试信息
    /*
    if (lightValue % 10 == 0) {
      Serial.printf("设置屏幕亮度: %d\n", brightness);
    }
    */
    
    lastBrightnessUpdateTime = millis();
  }
}

// 处理按钮事件
void handleButtonEvents() {
  ButtonEvent event = ButtonManager::getInstance()->check();
  
  switch (event) {
    case SHORT_PRESS:
      if (webConfigMode) {
        // 在配置模式下，单击按钮退出配置模式
        WebConfigServer::getInstance()->stop();
        webConfigMode = false;
        Serial.println("已退出Web配置模式");
        // 停止mDNS服务
        MDNS.end();
        Serial.println("mDNS responder stopped");
        // 重新连接WiFi
        initWiFiAndNTP();
      } else {
        // 正常模式下，单击切换屏幕
        ScreenManager::getInstance()->toggleScreen();
        lastScreenChangeTime = millis(); // 重置自动换屏计时器
      }
      break;
      
    case DOUBLE_CLICK:
      if (!webConfigMode) {
        // 正常模式下，双击切换自动换屏模式
        autoScreenChangeEnabled = !autoScreenChangeEnabled;
        Serial.print("自动换屏模式: ");
        Serial.println(autoScreenChangeEnabled ? "开启" : "关闭");
        
        // 增加视觉反馈，显示自动换屏状态
        TimeManager::getInstance()->setStatusInfo(
          autoScreenChangeEnabled ? "自动换屏: 开启" : "自动换屏: 关闭", 
          autoScreenChangeEnabled ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 
          true
        );
      }
      break;
      
    case LONG_PRESS:
      if (!webConfigMode) {
        // 长按开启Web配置模式
        webConfigMode = true;
        webConfigStartTime = millis();
        
        // 如果当前已连接WiFi，则使用STA模式启动Web服务器
        // 否则使用AP模式（热点）启动
        bool useAPMode = (WiFi.status() != WL_CONNECTED);
        WebConfigServer::getInstance()->start(useAPMode);
        
        // 在Web配置模式下初始化mDNS
        if (!MDNS.begin("ESP32-InfoBoard")) {
          Serial.println("Error setting up MDNS responder in Web config mode!");
        } else {
          Serial.println("mDNS responder started in Web config mode");
          // 添加HTTP服务
          MDNS.addService("http", "tcp", 80);
        }
        
        if (useAPMode) {
          Serial.println("已进入Web配置模式，请连接热点ESP32-InfoBoard，访问192.168.4.1或http://ESP32-InfoBoard.local");
          TimeManager::getInstance()->setStatusInfo("\uF013 192.168.4.1", lv_color_hex(0xFF0000), false);
        } else {
          Serial.print("已进入Web配置模式，请访问当前IP地址: ");
          Serial.println(WiFi.localIP());
          Serial.println("或者通过mDNS访问: http://ESP32-InfoBoard.local");
          char ipStr[20];
          WiFi.localIP().toString().toCharArray(ipStr, sizeof(ipStr));
          String statusText = "\uF012" + String(ipStr);
          TimeManager::getInstance()->setStatusInfo(statusText.c_str(), lv_color_hex(0x00FF00), false);
        }
      } else {
        // 长按退出Web配置模式
        WebConfigServer::getInstance()->stop();
        webConfigMode = false;
        Serial.println("已退出Web配置模式");
        // 停止mDNS服务
        MDNS.end();
        Serial.println("mDNS responder stopped");
        // 重新连接WiFi
        initWiFiAndNTP();
      }
      break;
      
    default:
      break;
  }
}

// 处理自动换屏
void handleAutoScreenChange() {
  if (autoScreenChangeEnabled) {
    // 获取当前时间秒数
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // 检查秒数是否能被30整除
    if (timeinfo.tm_sec % 30 == 0) {
      // 获取当前时间的毫秒数
      unsigned long currentMillis = millis();
      
      // 确保每30秒只切换一次屏幕（避免在同一秒内多次触发）
      if (currentMillis - lastScreenChangeTime >= 2000) {
        // 自动切换到下一个屏幕
        ScreenManager::getInstance()->toggleScreen();
        lastScreenChangeTime = currentMillis;
      }
    }
  }
}

// 初始化系统
void initSystem() {
  if (systemInitialized) {
    return;
  }
  
  // 初始化硬件
  initHardware();
  
  // 初始化UI元素
  initUI();
  
  // 初始化WiFi和NTP - 优先进行
  initWiFiAndNTP();
  
  // 等待时间同步完成
  Serial.println("等待NTP时间同步...");
  time_t now;
  struct tm timeinfo;
  int retry = 0;
  const int retry_limit = 10;
  while (retry < retry_limit) {
    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year > 100) { // 确保时间已同步（2000年之后）
      break;
    }
    delay(1000);
    retry++;
    Serial.print(".");
  }
  
  if (retry < retry_limit) {
    Serial.println("\nNTP时间同步成功");
  } else {
    Serial.println("\nNTP时间同步超时，继续启动");
  }
  
  // 初始化所有管理器
  ScreenManager::getInstance()->init();
  TimeManager::getInstance()->init();
  DataManager::getInstance()->init();
  ButtonManager::getInstance()->begin();
  initDisplayManager();
  
  // 初始化配置管理器（必须在WebConfigServer之前初始化）
  if (!ConfigManager::getInstance()->init()) {
    Serial.println("警告：配置管理器初始化失败");
  }
  
  // 初始化Web配置服务器
  WebConfigServer::getInstance()->init();
  

  
  // 标记系统已初始化
  systemInitialized = true;
  
  // 强制启用自动换屏功能
  autoScreenChangeEnabled = true;
  lastScreenChangeTime = 0; // 重置计时器
  Serial.println("系统初始化完成，自动换屏功能已启用");
  
  // 初始化完成后显示第一个屏幕（毛主席语录屏幕）
  ScreenManager::getInstance()->switchToScreen(NEWS_SCREEN);
}

// 显示任务函数 - 在CORE_0上运行
void displayTask(void *pvParameters) {
  Serial.println("显示任务启动在CORE_0");
  
  while (true) {
    // 处理按钮事件
    handleButtonEvents();
    
    // 更新亮度（非配置模式下）
    if (!webConfigMode) {
      updateBrightness();
    }

    // 函数级别的静态变量，用于配置信息显示
    static lv_obj_t* config_label = nullptr;
    static lv_obj_t* config_line = nullptr;
    
    if (webConfigMode) {
      // 在配置模式下，显示配置信息
      TimeManager::getInstance()->updateTimeDisplay();
      
      // 显示配置模式信息
      static unsigned long lastConfigDisplayUpdate = 0;
      if (millis() - lastConfigDisplayUpdate >= 1000) {
        lastConfigDisplayUpdate = millis();
        
        // 串口打印配置信息（简化版本）
        if (WiFi.status() == WL_CONNECTED) {
          Serial.print("访问当前IP地址: ");
          Serial.println(WiFi.localIP());
        } else {
          Serial.println("连接热点访问192.168.4.1");
        }
        
        // 根据WiFi模式决定是否显示配置信息
        if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
          // AP模式或AP+STA模式下显示配置信息
          String configInfo = "配置模式已启动\n";
          if (WiFi.status() == WL_CONNECTED) {
            configInfo += "IP: " + WiFi.localIP().toString();
          } else {
            configInfo += "请连接热点ESP32-InfoBoard\n";
            configInfo += "访问: 192.168.4.1";
          }
          
          // 创建或更新配置信息标签
          if (config_label == nullptr || !lv_obj_is_valid(config_label)) {
            config_label = lv_label_create(lv_scr_act());
            lv_obj_set_style_text_font(config_label, &lvgl_font_song_16, 0);
            lv_obj_set_style_text_color(config_label, lv_color_hex(0x00FF00), 0);
            lv_obj_set_width(config_label, screenWidth - 10); // 稍宽一些，留出边距
            lv_obj_align(config_label, LV_ALIGN_BOTTOM_MID, 0, -10); // 底部中央对齐，距离底部10像素
            lv_label_set_long_mode(config_label, LV_LABEL_LONG_SCROLL); // 设置为向上滚动显示模式
          }
          
          // 设置标签文本并显示
          if (config_label && lv_obj_is_valid(config_label)) {
            lv_label_set_text(config_label, configInfo.c_str());
            lv_obj_clear_flag(config_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(config_label);
          }
          
          // 创建或更新线条
          if (config_line == nullptr || !lv_obj_is_valid(config_line)) {
            config_line = lv_line_create(lv_scr_act());
            
            // 创建线条样式
            static lv_style_t style_line;
            lv_style_init(&style_line);
            lv_style_set_line_width(&style_line, 1); // 线条宽度
            lv_style_set_line_color(&style_line, lv_color_hex(0xFF0000)); // 红色
            lv_style_set_line_rounded(&style_line, true);
            
            // 添加样式到线条对象
            lv_obj_add_style(config_line, &style_line, 0);
            
            // 设置线条位置在config_label上方
            lv_point_t line_points[] = { {0, screenHeight - 75}, {screenWidth, screenHeight - 75} };
            lv_line_set_points(config_line, line_points, 2);
          }
          
          // 显示线条
          if (config_line && lv_obj_is_valid(config_line)) {
            lv_obj_clear_flag(config_line, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(config_line);
          }
        } else {
          // STA模式下隐藏配置信息标签和线条
          if (config_label && lv_obj_is_valid(config_label)) {
            lv_obj_add_flag(config_label, LV_OBJ_FLAG_HIDDEN);
          }
          if (config_line && lv_obj_is_valid(config_line)) {
            lv_obj_add_flag(config_line, LV_OBJ_FLAG_HIDDEN);
          }
        }
      }

    } else {
      // 正常模式下的操作
      // 更新时间显示
      TimeManager::getInstance()->updateTimeDisplay();
      
      // 处理自动换屏
      handleAutoScreenChange();
      
      // 清除状态标签显示内容
      TimeManager::getInstance()->clearStatusInfo();
      
      // 确保配置信息标签和线条被隐藏
      if (config_label && lv_obj_is_valid(config_label)) {
        lv_obj_add_flag(config_label, LV_OBJ_FLAG_HIDDEN);
      }
      if (config_line && lv_obj_is_valid(config_line)) {
        lv_obj_add_flag(config_line, LV_OBJ_FLAG_HIDDEN);
      }
    }
    
    // LVGL处理
    lv_task_handler();
    
    // 短暂延迟
    delay(10);
  }
}

// Web配置服务任务函数 - 在CORE_1上运行
void webConfigTask(void *pvParameters) {
  Serial.println("Web配置服务任务启动在CORE_1");
  
  while (true) {
    // 处理Web服务器请求
    WebConfigServer::getInstance()->handleClient();
    // 短暂延迟
    delay(10);
  }
}


// 数据处理任务函数 - 在CORE_1上运行
void dataTask(void *pvParameters) {
  Serial.println("数据处理任务启动在CORE_1");
  
  while (true) {
    // 检查Web配置模式超时
    if (webConfigMode && (millis() - webConfigStartTime >= WEB_CONFIG_TIMEOUT)) {
      WebConfigServer::getInstance()->stop();
      webConfigMode = false;
      Serial.println("Web配置模式超时，已退出");
      // 重新连接WiFi
      initWiFiAndNTP();
    }
    
    // 定期检查和更新所有缓存数据（非配置模式下）
    if (!webConfigMode) {
      DataManager::getInstance()->checkAndUpdateAllCaches();
    }
    
    // 短暂延迟，降低CPU使用率
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// 设置函数
void setup() {
  // 初始化系统
  initSystem();
  
  // 创建显示任务并固定到CORE_0
  xTaskCreatePinnedToCore(
    displayTask,    // 任务函数
    "DisplayTask",  // 任务名称
    4096,           // 任务堆栈大小
    NULL,           // 传递给任务的参数
    1,              // 任务优先级
    NULL,           // 任务句柄
    0               // 核心编号 (0)
  );
  
  // 创建Web配置服务任务并固定到CORE_1
  xTaskCreatePinnedToCore(
    webConfigTask,  // 任务函数
    "WebConfigTask", // 任务名称
    4096,           // 任务堆栈大小
    NULL,           // 传递给任务的参数
    1,              // 任务优先级
    NULL,           // 任务句柄
    1               // 核心编号 (1)
  );
  
  // 创建数据处理任务并固定到CORE_1
  xTaskCreatePinnedToCore(
    dataTask,       // 任务函数
    "DataTask",     // 任务名称
    4096,           // 任务堆栈大小
    NULL,           // 传递给任务的参数
    1,              // 任务优先级
    NULL,           // 任务句柄
    1               // 核心编号 (1)
  );
}

// 循环函数 - 现在为空，因为任务已在两个核心上运行
void loop() {
  // 任务已在两个核心上运行，这里可以留空
  delay(1000);
}