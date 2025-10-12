#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
void displayNewsDataFromFile();      //从文件读取JSON数据并显示新闻信息
void displayIcibaDataFromFile();     //从文件读取JSON数据并显示金山词霸每日信息
void displayAstronautsDataFromFile();//从文件读取JSON数据并显示宇航员信息
void initDisplayManager();           //初始化显示管理器

/* 测试从URL显示图片的功能
 * @param url 图片的URL地址
 */
void testDisplayImageFromUrl(const char* url);
#endif // DISPLAY_MANAGER_H