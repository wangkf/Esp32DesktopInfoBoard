# ESP32DesktopInfoBoard

ESP32桌面信息板是一个基于ESP32微控制器的多功能信息显示系统，可以实时展示新闻、名言警句等信息，并支持自动换屏、光线感应调节亮度和Web配置等功能。

## 项目概述

ESP32桌面信息板旨在为用户提供一个美观、实用的桌面信息展示设备，能够实时显示各种有用的信息，并通过简单的按钮操作实现不同信息界面的切换。系统采用了模块化设计，便于维护和扩展。

## 硬件要求

- ESP32开发板
- 3.2寸触摸屏（或其他兼容的TFT显示屏）
- 光线传感器（用于自动调节亮度）
- 按钮（用于屏幕切换和配置）
- 连接线和外壳

## 项目结构

项目采用模块化设计，主要包括以下目录和文件：

```
src/
├── main.cpp                # 主程序文件，包含系统初始化和任务管理
├── config/
│   ├── config.h            # 系统配置文件，定义常量和结构体
│   ├── config_manager.cpp  # 配置管理器实现
│   └── config_manager.h    # 配置管理器接口
├── manager/
│   ├── button_manager.cpp  # 按钮管理器实现
│   ├── button_manager.h    # 按钮管理器接口
│   ├── data_manager.cpp    # 数据管理器实现
│   ├── data_manager.h      # 数据管理器接口
│   ├── screen_manager.cpp  # 屏幕管理器实现
│   ├── screen_manager.h    # 屏幕管理器接口
│   ├── time_manager.cpp    # 时间管理器实现
│   └── time_manager.h      # 时间管理器接口
├── model/
│   ├── astronaut_data.h    # 宇航员数据模型
│   └── news_data.h         # 新闻数据模型
├── network/
│   ├── net_http.cpp        # HTTP网络请求实现
│   ├── net_http.h          # HTTP网络请求接口
│   ├── web_config_server.cpp # Web配置服务器实现
│   └── web_config_server.h # Web配置服务器接口
├── ui/
│   ├── display_manager.cpp # 显示管理器实现
│   ├── display_manager.h   # 显示管理器接口
│   ├── init_ui.cpp         # UI初始化
│   └── init_ui.h           # UI初始化接口
├── includes.h              # 通用头文件包含
├── maoselect.h             # 毛泽东选集内容
└── toxicsoul.h             # 心灵鸡汤内容
```

## 文件及函数功能描述

### 主程序文件

#### main.cpp

**功能**: 主程序文件，包含系统初始化、任务管理和主要功能实现。

**主要函数**: 
- `initHardware()`: 初始化硬件，包括显示屏、按键和光线传感器
- `initWiFiAndNTP()`: 初始化WiFi连接和NTP时间同步
- `updateBrightness()`: 根据光线传感器自动调节屏幕亮度
- `handleButtonEvents()`: 处理按钮事件（短按、双击、长按等）
- `handleAutoScreenChange()`: 处理自动换屏逻辑
- `initSystem()`: 初始化整个系统，包括硬件、UI、网络和各个管理器
- `displayTask()`: 显示任务函数，在CORE_0上运行
- `webConfigTask()`: Web配置服务任务函数，在CORE_1上运行
- `dataTask()`: 数据处理任务函数，在CORE_1上运行
- `setup()`: Arduino setup函数，创建任务并分配到不同核心
- `loop()`: Arduino loop函数，目前为空

### 配置文件

#### config/config.h

**功能**: 定义系统常量、结构体和基础配置信息。

**主要内容**: 
- NTP服务器配置
- 屏幕状态枚举
- 按钮事件类型枚举
- 硬件引脚配置
- 屏幕配置
- 按钮相关配置
- 自动换屏设置
- 刷新间隔配置
- 光线传感器配置

#### config/config_manager.h/cpp

**功能**: 管理系统配置，从data/config.json文件中读取和保存配置信息。

**主要内容**: 
- WiFi配置信息管理
- API密钥管理
- NTP时区配置
- 显示亮度配置
- 刷新间隔配置

### 管理类

#### config/config_manager.h/cpp

**功能**: 负责管理和加载系统配置，从data/config.json文件读取配置信息。

**主要函数**: 
- `ConfigManager::getInstance()`: 获取单例实例
- `ConfigManager::init()`: 初始化配置管理器
- `ConfigManager::loadConfig()`: 从文件加载配置
- `ConfigManager::saveConfig()`: 保存配置到文件
- `ConfigManager::isConfigLoaded()`: 检查配置是否已加载
- `ConfigManager::getWifiSSID()/getWifiPassword()`: 获取WiFi配置信息
- `ConfigManager::getApiKey()`: 获取API密钥
- `ConfigManager::getNtpTimezone()`: 获取NTP时区设置
- `ConfigManager::getDisplayBrightness()`: 获取显示亮度设置
- `ConfigManager::getRefreshInterval()`: 获取数据刷新间隔
- `ConfigManager::setWifiConfig()`: 设置WiFi配置
- `ConfigManager::setApiKey()`: 设置API密钥
- `ConfigManager::setNtpTimezone()`: 设置NTP时区
- `ConfigManager::setDisplayBrightness()`: 设置显示亮度
- `ConfigManager::setRefreshInterval()`: 设置数据刷新间隔

#### manager/time_manager.h/cpp

**功能**: 管理时间显示和更新。

**主要函数**: 
- `TimeManager::getInstance()`: 获取单例实例
- `TimeManager::init()`: 初始化时间管理器，创建时间相关标签
- `TimeManager::updateTimeDisplay()`: 更新时间显示
- `TimeManager::getCurrentTime()`: 获取当前时间
- `TimeManager::setStatusInfo()`: 设置状态信息
- `TimeManager::clearStatusInfo()`: 清除状态信息
- `TimeManager::updateMinuteDisplay()`: 更新分钟显示（每分钟更新一次）
- `TimeManager::updateSecondDisplay()`: 更新秒钟显示（每秒更新一次）
- `TimeManager::getDateLabel()/getWeekdayLabel()/getHourMinuteLabel()/getSecondLabel()`: 获取各种时间标签

#### manager/data_manager.h/cpp

**功能**: 管理各种数据的获取、缓存和更新。

**主要函数**: 
- `DataManager::getInstance()`: 获取单例实例
- `DataManager::init()`: 初始化数据管理器
- `DataManager::getCurrentDate()`: 获取当前日期 (YYYYMMDD格式)
- `DataManager::checkAndUpdateAllCaches()`: 检查并更新所有需要的缓存数据
- `DataManager::checkAndUpdateCache()`: 检查并更新特定类型的数据缓存
- `DataManager::forceRefreshAllData()`: 强制刷新所有数据
- `DataManager::fetchNewsData()/fetchICIBAData()/fetchAstronautsData()`: 从服务器获取各类数据
- `DataManager::initFileSystem()`: 初始化文件系统
- `DataManager::saveCacheData()`: 保存缓存数据到文件
- `DataManager::loadCacheData()`: 从文件加载缓存数据

#### manager/screen_manager.h/cpp

**功能**: 管理所有屏幕的切换和显示。

**主要函数**: 
- `ScreenManager::getInstance()`: 获取单例实例
- `ScreenManager::init()`: 初始化屏幕管理器
- `ScreenManager::toggleScreen()`: 切换到下一个屏幕
- `ScreenManager::switchToScreen()`: 直接切换到指定屏幕
- `ScreenManager::getCurrentScreen()`: 获取当前屏幕状态
- `ScreenManager::refreshCurrentScreenData()`: 刷新当前屏幕数据
- `ScreenManager::showNewsScreen()/showMaoSelectScreen()/showToxicSoulScreen()/showIcibaScreen()/showAstronautsScreen()`: 显示各类屏幕
- `ScreenManager::hideAllScreens()`: 隐藏所有屏幕元素
- `ScreenManager::setConfigIconStatus()`: 设置配置模式图标状态

#### manager/button_manager.h/cpp

**功能**: 管理按钮事件的检测和处理。

**主要函数**: 
- `ButtonManager::getInstance()`: 获取单例实例
- `ButtonManager::begin()`: 初始化按钮
- `ButtonManager::check()`: 检查按钮事件（应该在loop中调用）
- `ButtonManager::isPressed()`: 获取当前按钮状态
- `ButtonManager::getClickCount()`: 获取点击计数
- `ButtonManager::update()`: 更新按钮状态（在loop中调用）
- `ButtonManager::setTimeThresholds()`: 设置按钮事件的时间阈值
- `ButtonManager::getLastEventTime()`: 获取上次事件时间

### 网络组件

#### network/web_config_server.h/cpp

**功能**: 创建WiFi热点和提供web配置界面。

**主要函数**: 
- `WebConfigServer::getInstance()`: 获取单例实例
- `WebConfigServer::init()`: 初始化Web配置服务器
- `WebConfigServer::start()`: 启动Web配置服务器
- `WebConfigServer::stop()`: 停止Web配置服务器
- `WebConfigServer::handleClient()`: 处理Web服务器请求
- `WebConfigServer::isServerRunning()`: 检查服务器是否正在运行
- `WebConfigServer::handleRoot()`: 处理主页请求
- `WebConfigServer::handleWiFiConfig()`: 处理WiFi配置请求
- `WebConfigServer::handleJsonFile()`: 处理JSON文件查看请求
- `WebConfigServer::handleNotFound()`: 处理404错误
- `WebConfigServer::handleRestart()`: 处理系统重启请求

### UI组件

#### ui/display_manager.h/cpp

**功能**: 管理各种数据的显示逻辑。

**主要函数**: 
- `displayNewsDataFromFile()`: 从文件读取JSON数据并显示新闻信息
- `displayIcibaDataFromFile()`: 从文件读取JSON数据并显示金山词霸每日信息
- `displayAstronautsDataFromFile()`: 从文件读取JSON数据并显示宇航员信息
- `initDisplayManager()`: 初始化显示管理器
- `testDisplayImageFromUrl()`: 测试从URL显示图片的功能

#### ui/image_downloader.h/cpp

**功能**: 负责图片的下载、保存和显示。

**主要函数**: 
- `downloadImageFromUrl()`: 从URL下载图片并保存到SPIFFS
- `displayJpegFromFile()`: 在TFT屏幕上显示SPIFFS中的JPEG图片
- `displayImageFromUrl()`: 从URL下载图片并直接在TFT上显示
- `createLvImageFromUrl()`: 创建LVGL图片对象并显示从URL下载的图片

### 数据模型

#### model/news_data.h

**功能**: 定义新闻数据结构。

**主要结构**: 
- `NewsItem`: 新闻数据模型

## 主要功能

### 1. 新闻信息展示

显示最新的新闻资讯，支持定期更新。新闻数据通过API获取并缓存。

### 3. 名言警句展示

轮流展示毛泽东选集和心灵鸡汤内容，为用户提供正能量和思考。

### 4. 金山词霸每日一句

显示金山词霸的每日一句，帮助用户学习英语。

### 5. 国际空间站宇航员信息

显示当前在国际空间站上的宇航员信息。

### 6. ButtonManager功能

- 短按：切换显示内容
- 双击：开启/关闭自动换屏功能
- 长按：进入Web配置模式

### 7. 自动调节亮度

基于光线传感器的读数，自动调节屏幕亮度，以适应不同的环境光线条件。

### 8. Web配置功能

支持通过Web界面配置WiFi信息、城市代码等参数，无需重新编译上传代码。

## 环境要求

- Arduino IDE（推荐使用最新版本）
- ESP32开发板支持包
- LVGL库（用于图形界面）
- ArduinoJSON库（用于JSON数据处理）
- WiFi库（用于网络连接）
- HTTPClient库（用于HTTP请求）
- SPIFFS库（用于文件系统）
- OneButton库（用于按钮事件处理）

## 配置修改

配置参数现在通过以下方式进行管理：

1. **通过Web配置界面修改**：长按按钮进入Web配置模式，连接ESP32创建的WiFi热点，然后在浏览器中访问配置页面进行修改。

2. **直接编辑配置文件**：修改`data/config.json`文件中的相关配置项。

主要配置项包括：
- WiFi名称和密码
- API密钥
- NTP时区设置
- 显示亮度
- 数据刷新间隔

注意：不再需要直接修改config.h文件中的WiFi、API密钥等配置，这些配置现在由ConfigManager类统一管理。

## 环境要求

- Arduino IDE（推荐使用最新版本）
- ESP32开发板支持包
- LVGL库（用于图形界面）
- ArduinoJSON库（用于JSON数据处理）
- WiFi库（用于网络连接）
- HTTPClient库（用于HTTP请求）
- SPIFFS库（用于文件系统）
- OneButton库（用于按钮事件处理）

## 编译和上传

1. 确保已安装所有必要的库。
2. 将ESP32开发板连接到计算机。
3. 在Arduino IDE中选择正确的开发板型号和端口。
4. 编译并上传代码到ESP32开发板。

## 使用方法

1. 上电后，系统会自动初始化并连接WiFi。
2. 连接成功后，系统会同步时间并开始显示信息。
3. 短按按钮可以手动切换不同的显示内容。
4. 双击按钮可以开启或关闭自动换屏功能。
5. 长按按钮可以进入Web配置模式，通过手机或电脑连接ESP32创建的WiFi热点进行配置。

## 自动功能

- **自动换屏**: 默认启用，每隔30秒自动切换到下一个显示内容。可以通过双击按钮关闭或开启。
- **自动调节亮度**: 根据环境光线自动调节屏幕亮度。
- **数据自动更新**: 新闻等数据会定期自动更新（默认每2小时更新一次）。

## 代码优化亮点

### 1. 单例模式的广泛应用

采用单例模式设计各个管理器类，确保系统中只有一个实例，避免资源浪费和状态不一致问题。

### 2. 多任务处理

利用ESP32的双核特性，将显示任务和数据处理任务分别分配到不同的核心，提高系统响应速度和稳定性。

### 3. 数据缓存机制

实现了高效的数据缓存机制，减少网络请求次数，提高系统响应速度，同时降低功耗。

### 4. 文件系统集成

集成SPIFFS文件系统，支持数据的持久化存储和读取，确保断电后数据不丢失。

### 5. Web配置界面

提供Web配置界面，方便用户在不重新编译代码的情况下修改系统配置。

### 6. 自适应亮度调节

基于光线传感器实现自适应亮度调节，既节能又提高了用户体验。

### 7. 长文本处理优化

所有标签都配置为自动换行模式（LV_LABEL_LONG_WRAP），确保文本过长时不会出现滚动条，保持界面整洁美观。

## 注意事项

1. 请注意，`src/manager/data_manager.h`文件中仍然包含一个硬编码的新闻API URL和密钥：`https://apis.tianapi.com/bulletin/index?key=38c43566fe20217eab8108f8243b5a89`。建议修改这个文件，移除URL中的硬编码密钥，改为使用ConfigManager中管理的API密钥。

2. 请确保通过Web配置界面或data/config.json文件设置有效的API密钥，否则可能无法获取新闻等数据。系统默认使用"myapikey"作为占位符，需要替换为真实有效的API密钥。

3. 在连接不稳定的网络环境下，数据更新可能会失败。

4. 长时间运行可能会导致内存泄漏，建议定期重启设备。

5. 在Web配置模式下，系统会退出正常显示模式，完成配置后需要手动退出。

6. 首次启动时，由于需要同步时间和获取数据，可能需要较长时间才能显示完整信息。

7. 系统默认WiFi配置为SSID: "Mywifi", 密码: "12345678"，请务必通过Web配置界面或data/config.json文件修改为您自己的WiFi信息。

## 未来改进方向

1. **增加更多显示内容**: 考虑添加股票信息、日历、待办事项等实用功能。
2. **语音交互**: 集成语音识别功能，实现语音控制。
3. **电池供电**: 优化功耗，支持电池供电，提高便携性。
4. **云同步**: 实现数据的云同步，支持多设备数据共享。
5. **自定义显示内容**: 允许用户自定义显示内容和布局。
6. **离线模式优化**: 进一步优化离线模式下的用户体验。

## 致谢

感谢所有为本项目提供支持和贡献的开发者和用户！