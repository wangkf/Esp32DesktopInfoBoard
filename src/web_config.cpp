#include "web_config.h"

const char* AP_NAME = "ESP32桌面信息牌";//Web配网模式下的AP-wifi名字

//暂时存储wifi账号密码、城市代码、NewsKEY
char sta_ssid[32] = {0};
char sta_password[64] = {0};
char sta_citycode[32] = {0};
char sta_NewsKEY[256] = {0};
char blynkTimes[8] = {0};

//配网页面代码 
String page_html = R"(
<!DOCTYPE html>
<html lang='en'>
	<head>
		<meta charset='UTF-8'>
		
		<meta name="viewport" content="width=device-width,initial-scale=1.0,maximum-scale=1.0,minimum-scale=1.0,user-scalable=no">
		<title>ESP32桌面小电视</title>
		<style type="text/css">
			* { margin: 0; padding: 0; }
			 html { height: 100%; }
			 h2 {text-align: center;color: #fff;line-height: 2.2;}
			 body { height: 100%; background-color: #1F6F4A; 50% 50% no-repeat; background-size: cover;}
			 .dowebok { position: absolute; left: 50%; top: 30%; width: 380px; height: 440px; margin: -200px 0 0 -200px; border: 3px solid #fff; border-radius: 10px; overflow: hidden;}
			 
			 .form-item { position: relative; width: 360px; margin: 0 auto; padding-bottom: 20px;}
			 .form-item input { width: 288px; height: 48px; padding-left: 10px; border: 1px solid #fff; border-radius: 25px; font-size: 18px; color: #fff; background-color: transparent; outline: none;}
			 .send_button { width: 360px; height: 50px; border: 0; border-radius: 25px; font-size: 18px; color: #1f6f4a; outline: none; cursor: pointer; background-color: #fff; }
			 
			 .tip { display: none; position: absolute; left: 20px; top: 52px; font-size: 14px; color: #f50; }
			 .reg-bar { width: 360px; margin: 20px auto 0; font-size: 14px; overflow: hidden;}
			 .reg-bar a { color: #fff; text-decoration: none; }
			 .reg-bar a:hover { text-decoration: underline; }
			 .reg-bar .reg { float: left; }
			 .reg-bar .forget { float: right; }
			 .dowebok ::-webkit-input-placeholder { font-size: 18px; line-height: 1.4; color: #fff;}
			 .dowebok :-moz-placeholder { font-size: 18px; line-height: 1.4; color: #fff;}
			 .dowebok ::-moz-placeholder { font-size: 18px; line-height: 1.4; color: #fff;}
			 .dowebok :-ms-input-placeholder { font-size: 18px; line-height: 1.4; color: #fff;}
			  
			 @media screen and (max-width: 500px) {
			 * { box-sizing: border-box; }
			 .dowebok { position: static; width: auto; height: auto; margin: 0 30px; border: 0; border-radius: 0; }
			 .logo { margin: 50px auto; }
			 .form-item { width: auto; }
			 .form-item input, .form-item button, .reg-bar { width: 100%; }
			 }
			 
		</style>
	</head>
	
	<body>
		<div class="dowebok">
			<h2>参 数 配 置</h2>
			<form style='text-align: center;padding-top: 20px' name='input' action='/' method='POST'>	 
				 <div class="form-item">
					<input id="username" type="text" name='ssid' autocomplete="off" placeholder="WiFi名称">
				 </div>
				 <div class="form-item">
					<input id="password" type="password" name='password' autocomplete="off" placeholder="WiFi密码">
				 </div>
				 <div class="form-item">
				 	<input id="citycode" type="citycode" name='citycode' autocomplete="off" placeholder="城市代码,留空则定位获取">
				 </div>
				 <div class="form-item">
				 	<input id="NewsKEY" type="NewsKEY" name='NewsKEY' autocomplete="off" placeholder="授权码,系统已内置">
				 </div>
				 <div class="form-item">
					 <div id="">
						<input id="send_button" type='submit' value='保存并连接'>
					 </div>
				</div>
			</form>	
		 </div>
	</body>
</html>
)";

const byte DNS_PORT = 53;//DNS端口号
IPAddress apIP(192, 168, 4, 1);//esp32-AP-IP地址
DNSServer dnsServer;//创建dnsServer实例
WebServer server(80);//创建WebServer

unsigned long getChipId() {
  unsigned long  mcuchipId = 0;
  for(int i=0; i<31; i=i+8) {
    mcuchipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  return mcuchipId;
}

void handleRoot() {//访问主页回调函数
  long chipid = getChipId();
  server.send(200, "text/html", page_html);
}

void handleRootPost() {//Post回调函数
  Serial.println("handleRootPost");
  if (server.hasArg("ssid")) {//判断是否有账号参数
    Serial.print("got ssid:");
    strcpy(sta_ssid, server.arg("ssid").c_str());//将账号参数拷贝到sta_ssid中
    Serial.println(sta_ssid);
  } else {//没有参数
    Serial.println("error, not found ssid");
    server.send(200, "text/html", "<meta charset='UTF-8'>提示:请输入WiFi名称");//返回错误页面
    return;
  }
  //密码与账号同理
  if (server.hasArg("password")) {
    Serial.print("got password:");
    strcpy(sta_password, server.arg("password").c_str());
    Serial.println(sta_password);
  } else {
    Serial.println("error, not found password");
    server.send(200, "text/html", "<meta charset='UTF-8'>提示:请输入WiFi密码");
    return;
  }


  if (server.hasArg("citycode")) {
    Serial.print("got citycode:");
    strcpy(sta_citycode, server.arg("citycode").c_str());
    Serial.println(sta_citycode);
  } else {
    //Serial.println("error, not found citycode");
    //server.send(200, "text/html", "<meta charset='UTF-8'>提示：请输入城市代码");
    return;
  }

  if (server.hasArg("NewsKEY")) {
    Serial.print("got NewsKEY:");
    strcpy(sta_NewsKEY, server.arg("NewsKEY").c_str());
    Serial.println(sta_NewsKEY);
  } else {
    Serial.println("error, not found NewsKEY");
    server.send(200, "text/html", "<meta charset='UTF-8'>提示:请输入授权码");
    return;
  }

  preferences.begin("wifi", false);
  preferences.putString( "ssid" , sta_ssid);
  preferences.putString( "password", sta_password);
  preferences.putString( "citycode", sta_citycode);
  String nvs_sta_NewsKEY = String(sta_NewsKEY);
  if(nvs_sta_NewsKEY.length() >= 30) { //如果授权码有更新，则做修改，否则无，默认使用上次的记录
    preferences.putString( "NewsKEY", sta_NewsKEY);
  }
  preferences.end();

  server.send(200, "text/html", "<meta charset='UTF-8'><h1>保存成功，ESP32桌面小电视重启中...</h1>");//返回保存成功页面
  delay(2000);
  //连接wifi
  //connectNewWifi();

  ESP.restart(); //重启ESP32
}

void initBasic(void){//初始化基础
  //Serial.begin(115200);
  //WiFi.hostname("Smart-ESP32");//设置ESP32设备名
}

void initSoftAP(void){//初始化AP模式
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  if(WiFi.softAP(AP_NAME)){
    Serial.println("热点已开：ESP32桌面信息牌");
  }
}

void initWebServer(void){//初始化WebServer
  //server.on("/",handleRoot);
  //上面那行必须以下面这种格式去写否则无法强制门户
  server.on("/", HTTP_GET, handleRoot);//设置主页回调函数
  server.onNotFound(handleRoot);//设置无法响应的http请求的回调函数
  server.on("/", HTTP_POST, handleRootPost);//设置Post请求回调函数
  server.begin();//启动WebServer
  Serial.println("Web服务已启动");
}

void initDNS(void){//初始化DNS服务器
  if(dnsServer.start(DNS_PORT, "*", apIP)){//判断将所有地址映射到esp32的ip上是否成功
    Serial.println("DNS服务成功");
  }
  else Serial.println("DNS服务失败");
}

void connectNewWifi(void){
  WiFi.mode(WIFI_STA);//切换为STA模式
 // WiFi.setAutoConnect(true);//设置自动连接
    WiFi.begin(PrefSSID.c_str(), PrefPassword.c_str());//连接上一次连接成功的wifi
  Serial.println("");
  Serial.print("Connect to wifi");
  int count = 0;
   while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    count++;
    if(count > 20){//如果10秒内没有连上，就开启Web配网 可适当调整这个时间
      initSoftAP();
      initWebServer();
      initDNS();
      break;//跳出 防止无限初始化
    }
    Serial.print(".");
  }
  Serial.println("");
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("WIFI Connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    server.stop();
  }
}

//强制门户Web配网
bool setWiFi_Flag = false;
void setWiFi()
{
  //配网页面显示
  LV_IMG_DECLARE(Page_Setting);
  lv_obj_t *img_WIFISetting = lv_img_create(lv_scr_act());
  lv_img_set_src(img_WIFISetting, &Page_Setting);
  lv_obj_set_pos(img_WIFISetting, 2, 0);
  for (int i = 0; i <= 100; i++)
  {
    lv_timer_handler();
    delay(5);
  }
  initBasic();
  initSoftAP();
  initWebServer();
  initDNS();
  while (setWiFi_Flag == false)
  {
    server.handleClient();
    dnsServer.processNextRequest();
    if (WiFi.status() == WL_CONNECTED)
    {
      server.stop();
      setWiFi_Flag = true;
    }
  }
}

void getNews()
{
  //获取今日简报
  HTTPClient httpClient;
  //启动内置天行数据APIKEY
  if (TianXingAPIKEY_Flag == true)
  {
    APIKEY = "9307ac0871681b03f6d0ed892caaea83";
  }
  String URL = "http://api.tianapi.com/bulletin/index?key=" + APIKEY;
  //创建 HTTPClient 对象

  httpClient.begin(URL);

  //设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://api.tianapi.com/");

  //启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.println("正在获取今日简报");
  // Serial.println(URL);

  //如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK && label_message_data != NULL)
  {
    String Neswstr = httpClient.getString();
    // Serial.println(Neswstr);
    Serial.println("获取今日简报成功");
    DynamicJsonDocument NeswData(4096);
    DeserializationError error = deserializeJson(NeswData, Neswstr);
    
    if (!error) {
      // 安全地访问JSON数据
      if (NeswData.containsKey("newslist")) {
        JsonArray newslist = NeswData["newslist"];
        if (newslist.size() >= 3) {
          // 确保能安全获取新闻标题
          String title1 = newslist[0].containsKey("title") ? newslist[0]["title"].as<String>() : "";
          String title2 = newslist[1].containsKey("title") ? newslist[1]["title"].as<String>() : "";
          String title3 = newslist[2].containsKey("title") ? newslist[2]["title"].as<String>() : "";
          
          lv_label_set_text_fmt(label_message_data,
                                "1、%s\n2、%s\n3、%s",
                                title1.c_str(), title2.c_str(), title3.c_str());
        }
      }
    }
  }
  httpClient.end();
}

// 定义头文件中声明但未在源文件中定义的变量
Preferences preferences;
String PrefSSID = "";
String PrefPassword = "";
String cityCode = "101110101"; // 默认西安
String APIKEY = "9307ac0871681b03f6d0ed892caaea83";
int GIFNUM = 0;
bool TianXingAPIKEY_Flag = true;
lv_obj_t *label_message_data = NULL;

const char *versionNumber = "1.0.0";

// 引脚定义
const int button = 0;
const int localPort = 123;
const int updateTime = 60000;

int Filter_Value = 0;
int setModeValue = 0;

// 空的占位符函数
const unsigned char connect_wifi_map[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 为缺失的图片资源创建空的占位符定义
extern const unsigned char Page_Setting[];
const unsigned char Page_Setting[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

extern const unsigned char finsh_00[];
const unsigned char finsh_00[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

extern const unsigned char longmao_00[];
const unsigned char longmao_00[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

extern const unsigned char love_00[];
const unsigned char love_00[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

extern const unsigned char taikongren_00[];
const unsigned char taikongren_00[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

extern const unsigned char huojian_00[];
const unsigned char huojian_00[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

extern const unsigned char fengche_00[];
const unsigned char fengche_00[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

extern const unsigned char setting_00[];
const unsigned char setting_00[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

extern const unsigned char wain_00[];
const unsigned char wain_00[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

