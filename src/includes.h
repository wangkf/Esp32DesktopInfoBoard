#ifndef INCLUDES_H
#define INCLUDES_H

// Arduino核心库
#include <Arduino.h>

// JSON库
#include <ArduinoJson.h>

// FreeRTOS支持
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 配置文件
#include "config/config.h"

// 管理器模块
#include "manager/screen_manager.h"
#include "manager/time_manager.h"
#include "manager/data_manager.h"
#include "manager/button_manager.h"

// UI模块
#include "ui/display_manager.h"
#include "ui/weather_display.h"
#include "ui/weather_manager.h"

// 初始化模块
#include "init_ui.h"

// 网络模块
#include "network/net_http.h"

#endif // INCLUDES_H