#ifndef IMAGE_DOWNLOADER_H
#define IMAGE_DOWNLOADER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>
#include <lvgl.h>

// 外部变量声明
extern TFT_eSPI tft;
extern const uint32_t screenWidth;
extern const uint32_t screenHeight;

/**
 * 从URL下载图片并保存到SPIFFS
 * @param url 图片的URL地址
 * @param filename 保存到SPIFFS的文件名
 * @return 是否下载成功
 */
bool downloadImageFromUrl(const char* url, const char* filename);

/**
 * 在TFT屏幕上显示SPIFFS中的JPEG图片
 * @param filename SPIFFS中的文件名
 * @param xpos 显示位置的X坐标
 * @param ypos 显示位置的Y坐标
 * @return 是否显示成功
 */
bool displayJpegFromFile(const char* filename, int xpos, int ypos);

/**
 * 从URL下载图片并直接在TFT上显示
 * @param url 图片的URL地址
 * @param xpos 显示位置的X坐标
 * @param ypos 显示位置的Y坐标
 * @return 是否成功
 */
bool displayImageFromUrl(const char* url, int xpos, int ypos);

/**
 * 创建LVGL图片对象并显示从URL下载的图片
 * @param parent LVGL父对象
 * @param url 图片的URL地址
 * @param x X坐标
 * @param y Y坐标
 * @return 创建的LVGL图片对象指针
 */
lv_obj_t* createLvImageFromUrl(lv_obj_t* parent, const char* url, int x, int y);

#endif // IMAGE_DOWNLOADER_H