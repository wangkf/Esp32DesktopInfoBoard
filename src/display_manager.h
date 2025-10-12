#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>

/**
 * 从文件读取JSON数据并显示天气信息
 */
void displayWeatherDataFromFile();

/**
 * 从文件读取JSON数据并显示新闻信息
 */
void displayNewsDataFromFile();

/**
 * 从文件读取JSON数据并显示金山词霸每日信息
 */
void displayIcibaDataFromFile();

/**
 * 从文件读取JSON数据并显示宇航员信息
 */
void displayAstronautsDataFromFile();

/**
 * 从文件读取JSON数据并显示APRS信息
 */
void displayAPRSDataFromFile();

/**
 * 初始化显示管理器
 */
void initDisplayManager();

#endif // DISPLAY_MANAGER_H