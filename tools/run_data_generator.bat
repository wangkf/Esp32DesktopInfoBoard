@echo off

REM 检查platformio是否已安装
platformio --version >nul 2>&1
if %errorlevel% neq 0 (
    echo 错误：未找到PlatformIO CLI。请先安装PlatformIO。
    echo 安装指南：https://platformio.org/install/cli
    pause
    exit /b 1
)

REM 进入当前脚本所在目录
cd /d %~dp0

REM 构建并运行数据生成工具
echo 正在构建数据生成工具...
platformio run -e esp32dev

if %errorlevel% neq 0 (
    echo 构建失败，请检查错误信息。
    pause
    exit /b 1
)

echo 构建成功！
echo.
echo 请将ESP32开发板连接到电脑，然后按任意键继续上传程序...
pause

echo 正在上传程序到ESP32...
platformio run -e esp32dev --target upload

if %errorlevel% neq 0 (
    echo 上传失败，请检查连接和端口设置。
    pause
    exit /b 1
)

echo 程序上传成功！
echo.
echo 正在启动串行监视器查看程序输出...
echo 注意：程序将连接到WiFi并生成初始数据，完成后会显示数据保存信息。
echo 按Ctrl+C可以关闭监视器。
platformio device monitor -e esp32dev

pause