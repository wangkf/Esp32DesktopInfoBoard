#ifndef WEATHER_DATA_H
#define WEATHER_DATA_H

// 天气数据模型
struct WeatherData {
  String cityName;
  String weatherCondition;
  int temperature;
  int humidity;
  int windSpeed;
  String updateTime;
};

// 未来天气预测模型
struct ForecastData {
  String date;
  String dayCondition;
  String nightCondition;
  int maxTemp;
  int minTemp;
};

#endif // WEATHER_DATA_H