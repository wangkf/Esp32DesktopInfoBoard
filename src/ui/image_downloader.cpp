#include "ui/image_downloader.h"
#include <FS.h>

/**
 * 从URL下载图片并保存到SPIFFS
 * @param url 图片的URL地址
 * @param filename 保存到SPIFFS的文件名
 * @return 是否下载成功
 */
bool downloadImageFromUrl(const char* url, const char* filename) {
  // 检查WiFi连接
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi未连接，无法下载图片");
    return false;
  }

  HTTPClient http;
  Serial.print("正在下载图片: ");
  Serial.println(url);

  // 开始HTTP请求
  http.begin(url);
  http.setTimeout(10000); // 设置超时时间为10秒
  
  // 添加User-Agent头，有些服务器可能需要
  http.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");

  // 发送GET请求
  int httpCode = http.GET();

  // 检查响应代码
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP请求失败，错误代码: %d\n", httpCode);
    http.end();
    return false;
  }

  // 获取响应体大小
  int len = http.getSize();
  if (len <= 0) {
    Serial.println("获取图片大小失败");
    http.end();
    return false;
  }

  Serial.printf("图片大小: %d 字节\n", len);

  // 打开文件进行写入
  File file = SPIFFS.open(filename, "w");
  if (!file) {
    Serial.printf("无法创建文件: %s\n", filename);
    http.end();
    return false;
  }

  // 获取数据流
  WiFiClient *stream = http.getStreamPtr();

  // 缓冲区大小
  const size_t bufferSize = 1024;
  uint8_t buffer[bufferSize];
  size_t totalBytesWritten = 0;

  // 读取数据并写入文件
  while (http.connected() && (len > 0 || len == -1)) {
    size_t size = stream->available();
    if (size) {
      int bytesRead = stream->readBytes(buffer, ((size > bufferSize) ? bufferSize : size));
      file.write(buffer, bytesRead);
      totalBytesWritten += bytesRead;
      if (len > 0) {
        len -= bytesRead;
      }
    }
    delay(1);
  }

  // 关闭文件和HTTP连接
  file.close();
  http.end();

  Serial.printf("图片已保存到: %s, 写入字节数: %u\n", filename, totalBytesWritten);
  return true;
}

/**
 * 在TFT屏幕上显示SPIFFS中的JPEG图片
 * @param filename SPIFFS中的文件名
 * @param xpos 显示位置的X坐标
 * @param ypos 显示位置的Y坐标
 * @return 是否显示成功
 */
bool displayJpegFromFile(const char* filename, int xpos, int ypos) {
  // 检查文件是否存在
  if (!SPIFFS.exists(filename)) {
    Serial.printf("文件不存在: %s\n", filename);
    return false;
  }

  // 打开文件
  File jpegFile = SPIFFS.open(filename, FILE_READ);
  if (!jpegFile) {
    Serial.printf("无法打开文件: %s\n", filename);
    return false;
  }

  Serial.println("===========================");
  Serial.print("显示图片文件: ");
  Serial.println(filename);
  Serial.println("===========================");

  // 解码JPEG图片
  bool decoded = JpegDec.decodeFsFile(jpegFile);
  jpegFile.close();

  if (!decoded) {
    Serial.println("JPEG文件解码失败或格式不支持!");
    return false;
  }

  // 打印图片信息
  Serial.printf("图片尺寸: %d x %d\n", JpegDec.width, JpegDec.height);
  Serial.printf("组件数: %d\n", JpegDec.comps);
  Serial.printf("每行列数: %d\n", JpegDec.MCUSPerRow);
  Serial.printf("每列行数: %d\n", JpegDec.MCUSPerCol);
  Serial.printf("扫描类型: %d\n", JpegDec.scanType);
  Serial.printf("块宽度: %d\n", JpegDec.MCUWidth);
  Serial.printf("块高度: %d\n", JpegDec.MCUHeight);

  // 渲染图片到屏幕
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  bool swapBytes = tft.getSwapBytes();
  tft.setSwapBytes(true);
  
  // 计算边缘块的宽度和高度
  uint32_t min_w = min((uint32_t)mcu_w, max_x % mcu_w);
  uint32_t min_h = min((uint32_t)mcu_h, max_y % mcu_h);

  // 保存当前块的尺寸
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // 记录开始时间，用于计算绘制时间
  uint32_t drawTime = millis();

  // 计算图片右下角坐标
  max_x += xpos;
  max_y += ypos;

  // 读取、解码并显示数据
  while (JpegDec.read()) {
    pImg = JpegDec.pImage;

    // 计算当前块的左上角坐标
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // 检查是否需要调整块的宽度（右侧边缘）
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // 检查是否需要调整块的高度（底部边缘）
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // 复制像素到连续块
    if (win_w != mcu_w) {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++) {
        p += mcu_w;
        for (int w = 0; w < win_w; w++) {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // 绘制块
    tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
  }

  // 恢复原始的交换字节设置
  tft.setSwapBytes(swapBytes);

  // 计算绘制时间
  drawTime = millis() - drawTime;
  Serial.printf("绘制图片耗时: %u 毫秒\n", drawTime);
  return true;
}

/**
 * 从URL下载图片并直接在TFT上显示
 * @param url 图片的URL地址
 * @param xpos 显示位置的X坐标
 * @param ypos 显示位置的Y坐标
 * @return 是否成功
 */
bool displayImageFromUrl(const char* url, int xpos, int ypos) {
  // 临时文件名
  const char* tempFilename = "/temp_image.jpg";
  
  // 下载图片到临时文件
  if (!downloadImageFromUrl(url, tempFilename)) {
    return false;
  }
  
  // 显示下载的图片
  bool result = displayJpegFromFile(tempFilename, xpos, ypos);
  
  // 删除临时文件
  SPIFFS.remove(tempFilename);
  
  return result;
}

/**
 * 创建LVGL图片对象并显示从URL下载的图片
 * @param parent LVGL父对象
 * @param url 图片的URL地址
 * @param x X坐标
 * @param y Y坐标
 * @return 创建的LVGL图片对象指针
 */
lv_obj_t* createLvImageFromUrl(lv_obj_t* parent, const char* url, int x, int y) {
  // 临时文件名
  const char* tempFilename = "/lvgl_temp_image.jpg";
  
  // 下载图片到临时文件
  if (!downloadImageFromUrl(url, tempFilename)) {
    return NULL;
  }
  
  // 创建LVGL图片对象
  lv_obj_t* img = lv_img_create(parent);
  
  // 设置图片位置
  lv_obj_set_pos(img, x, y);
  
  // 尝试加载图片
  // 注意：LVGL默认不支持直接从SPIFFS加载JPEG，需要使用TFT_eSPI直接显示
  // 这里简化处理，使用TFT_eSPI直接显示，然后返回LVGL对象
  
  // 先隐藏LVGL层，直接使用TFT_eSPI显示
  lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
  
  // 使用TFT_eSPI显示图片
  displayJpegFromFile(tempFilename, x, y);
  
  // 重新显示LVGL层
  lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
  
  // 刷新显示
  lv_obj_invalidate(img);
  
  // 删除临时文件
  SPIFFS.remove(tempFilename);
  
  return img;
}