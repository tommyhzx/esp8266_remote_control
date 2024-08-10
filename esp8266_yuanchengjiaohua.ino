/*
关于UID：在巴法云注册登陆，即可看到自己UID，推送微信消息，需要手机绑定微信，bemfa.com在控制台进行绑定即可。
QQ交流群：824273231
巴法云官网：bemfa.com
时间：2020/04/25
官方文档见官网：http://www.cloud.bemfa.com/docs/#/
*/
// DHT11数据上传+LED远程控制 //不限于DHT11,可以接其他传感器，也可以接多个传感器，这里只是例程
// DHT11数据上传主题temp   //根据自己在控制台命的名字可自己随意修改
// LED灯控制主题light002   //根据自己命的名字可自己随意修改
/*
程序讲解：ESP8266 有两个角色，一个是temp(传感器数据)主题消息的发布者，esp8266往这个主题推送消息，手机app订阅temp主题，就可以收到传感器数据了。
esp8266联网后，订阅light002，手机往这个主题推送消息，esp8266就能收到手机的控制的指令了。
*/

// 需要在arduino IDE软件中---工具-->管理库-->搜索arduinojson并安装
#include <ESP8266WiFi.h>
#include <SimpleDHT.h>

// SoftAP需要库
#include <WiFiUDP.h>
#include <ArduinoJson.h>
// web配网需要库
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
// 保存配置
#include <EEPROM.h>
#include <FS.h>
// 巴法云服务器地址默认即可
#define TCP_SERVER_ADDR "bemfa.com"
// 服务器端口//TCP创客云端口8344//TCP设备云端口8340
#define TCP_SERVER_PORT "8344"
///****************需要修改的地方*****************///
#define WIFI_CONNECT_TIMEOUT 15000 // 定义 WiFi 连接超时时间为 10 秒
#define AP_TIMEOUT 20000           // 首次上电AP等待20s
#define TCP_RCV_PERIOD 100         // tcp接收数据的时间

#define CONFIG_ONCE 1    // 只配置一次，等待10s
#define CONFIG_FOREVER 9 // 进入softAp模式不退出

#define EEPROM_FULL_ADDR 64     // EEPROM的全部地址大小
#define EEPROM_SSID_ADDR 0      // 保存在EEPROM的SSID的地址
#define EEPROM_PASSWORD_ADDR 32 // 保存在EEPROM的PASSWORD的地址
#define EEPROM_RST_ADDR 60      // 保存在EEPROM的复位次数的地址
// 用户私钥，可在控制台获取,修改为自己的UID
String UID = "c2421290f7d14fa38251e5f77aac931a";
// 主题名字，可在控制台新建
String TOPIC = "SN0000006"; // 用于传输温湿度的主题
// 设备SN码
String deviceSN = "SN0000006";

// 单片机LED引脚值
/***
1----->GPIO3   LOW 点亮  HIGH 熄灭
0----->GPIO0 继电器控制引脚
2-----> GPIO2 本地开关
***/
const int SWITCH_Pin = 2; // ESP-01s 连接开关的引脚 GPIO0
// ESP-01s的状态指示灯 GPIO2, wifi正在连接时，led闪烁，无wifi连接，led点亮，连接上，led灭
const int LED_Status = 0; //
// 主题名字，可在控制台新建
String TOPIC2 = "light002"; // 用于led控制的主题
//****************全局变量*********************//
int g_taskStarted = false; // 是否启动TCPClient
String g_wifiSSID = "";    //
String g_wifiPassword = "";
///*********************************************///
// led 控制函数
#define LED_ON 0
#define LED_OFF 1
#define SWITCH_ON 1
#define SWITCH_OFF 0
// 设置上传速率2s（1s<=upDataTime<=60s）
// 下面的2代表上传间隔是2秒
#define upDataTime 2 * 1000

// 最大字节数
#define MAX_PACKETSIZE 512
// tcp客户端相关初始化，默认即可
WiFiClient TCPclient;
String TcpClient_Buff = "";
unsigned int TcpClient_BuffIndex = 0;
unsigned long TcpClient_preTick = 0;
unsigned long preHeartTick = 0;    // 心跳
unsigned long preTCPStartTick = 0; // 连接
bool preTCPConnected = false;
unsigned long switch_on_tick = 0;
int switch_on = false;

/*****************************************
 * softap相关变量
 ******************************************/
// 根据需要修改的信息
String type = "002";  // 设备类型，001插座设备，002灯类设备，003风扇设备,005空调，006开关，009窗帘
String Name = "台灯"; // 设备昵称，可随意修改
String proto = "3";   // 3是tcp设备端口8344,1是MQTT设备

char g_config_flag = 1; // 判断是否配网
WiFiUDP Udp;
char packetBuffer[255]; // 发送数据包
bool IsWartering = 0;   // 0表示没在浇花，1表示正在浇花
/***************************************/

// 相关函数初始化
// 连接WIFI
void doWiFiTick();
void startSTA(String ssid, String password);

// TCP初始化连接
void doTCPClientTick();
void startTCPClient();
void sendtoTCPServer(String p);

/*******************************************
 *发送数据到TCP服务器
 *******************************************/
void sendtoTCPServer(String p)
{

  if (!TCPclient.connected())
  {
    Serial.println("Client is not readly");
    return;
  }
  TCPclient.print(p);
  // Serial.println("[Send to TCPServer]:String");
  // Serial.println(p);
}

/*****************************************
 *初始化和服务器建立连接
 *****************************************/
void startTCPClient()
{
  if (TCPclient.connect(TCP_SERVER_ADDR, atoi(TCP_SERVER_PORT)))
  {
    Serial.print("\nConnected to server:");
    Serial.printf("%s:%d\r\n", TCP_SERVER_ADDR, atoi(TCP_SERVER_PORT));
    // 首次连接上TCP时，先发布一次订阅
    String tcpTemp = "";
    tcpTemp = "cmd=1&uid=" + UID + "&topic=" + TOPIC + "\r\n";
    sendtoTCPServer(tcpTemp);
    Serial.println("first sbscrib" + tcpTemp);

    preTCPConnected = true;
    preHeartTick = millis();
    TCPclient.setNoDelay(true);
  }
  else
  {
    Serial.print("Failed connected to server:");
    Serial.println(TCP_SERVER_ADDR);
    TCPclient.stop();
    preTCPConnected = false;
  }
  preTCPStartTick = millis();
}

/*************************************************
 *检查数据，发送数据
 *************************************************/
void doTCPClientTick()
{
  // 检查是否断开，断开后重连
  if (WiFi.status() != WL_CONNECTED)
    return;
  // TCP客户端没有连接
  if (!TCPclient.connected())
  {

    if (preTCPConnected == true)
    {

      preTCPConnected = false;
      preTCPStartTick = millis();
      Serial.println();
      Serial.println("TCP Client disconnected.");
      TCPclient.stop();
    }
    else if (millis() - preTCPStartTick > 1 * 1000) // 重新连接
      startTCPClient();
  }
  else
  {
    // 收数据
    if (TCPclient.available())
    {
      char c = TCPclient.read();
      TcpClient_Buff += c;
      TcpClient_BuffIndex++;
      TcpClient_preTick = millis();

      if (TcpClient_BuffIndex >= MAX_PACKETSIZE - 1)
      {
        TcpClient_BuffIndex = MAX_PACKETSIZE - 2;
        TcpClient_preTick = TcpClient_preTick - 200;
      }
      preHeartTick = millis();
    }
    // 上传数据
    if (millis() - preHeartTick >= upDataTime)
    {
      preHeartTick = millis();
      // 发送心跳
      sendtoTCPServer("ping\r\n");
      /*********************数据上传*******************/
      /*
        数据用#号包裹，以便app分割出来数据，&msg=#23#80#on#\r\n，即#温度#湿度#按钮状态#，app端会根据#号分割字符串进行取值，以便显示
        如果上传的数据不止温湿度，可在#号后面继续添加&msg=#23#80#data1#data2#data3#data4#\r\n,app字符串分割的时候，要根据上传的数据进行分割
      */
      // String upstr = "";
      // upstr = "cmd=2&uid="+UID+"&topic="+TOPIC+"&msg=#"+temperature+"#"+humidity+"#"+my_led_status+"#\r\n";
      // cmd=1、3为订阅
      // cmd=1&uid=xxxxxxxxxxxxxxxxxxxxxxx&topic=xxx1,xxx2,xxx3,xxx4\r\n
      // upstr = "cmd=3&uid="+UID+"&topic="+TOPIC+"\r\n";
      // sendtoTCPServer(upstr);
      // upstr = "";
    }
    // 读取数据并判断

    if ((TcpClient_Buff.length() >= 1) && (millis() - TcpClient_preTick >= 200))
    { // 当订阅后，其它设备发布信息将会自动接收
      TCPclient.flush();
      // Serial.println("read Buff is " + TcpClient_Buff);
      // 处理接收到的数据
      processPacket(TcpClient_Buff);
      // 清空字符串，以便下次接收
      TcpClient_Buff = "";
      TcpClient_BuffIndex = 0;
    }
    // 检查开关是否超时
    checkSwitchTimeout();
  }
}
// 处理数据包，判断是否是开关控制
void processPacket(String packet)
{
  // 有时会连续收到包，若接收到的最后的数据中包含&msg=，则说明是最终数据
  int lastIndex = packet.lastIndexOf("&msg=");
  if (lastIndex != -1)
  {
    // 去除掉&msg=这5个数据
    String msgValue = packet.substring(lastIndex + 5);
    msgValue.trim();
    Serial.println("msgValue is " + msgValue);
    if (msgValue.equals("on"))
    {
      SwitchSet(SWITCH_ON);
      switch_on_tick = millis();
      switch_on = true;
    }
    else if (msgValue.equals("off"))
    {
      SwitchSet(SWITCH_OFF);
      switch_on = false;
    }
  }
}
// 如果接收到on后，连续10s未收到off消息，则自动关闭
void checkSwitchTimeout()
{
  if (switch_on && (millis() - switch_on_tick >= 10 * 1000))
  {
    SwitchSet(SWITCH_OFF);
    switch_on = false;
  }
}
// 控制开关
void SwitchSet(int status)
{
  Serial.println("switch status is");
  Serial.println(status);
  digitalWrite(SWITCH_Pin, status);
}
// 控制指示灯
void setLEDStatus(int status)
{
  digitalWrite(LED_Status, status);
}

void startSTA(String ssid, String password)
{
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
}

/**************************************************************************
                                 WIFI
***************************************************************************/
/*
  WiFiTick
  检查是否需要初始化WiFi
  检查WiFi是否连接上，若连接成功启动TCP Client
  控制指示灯
*/
void doWiFiTick()
{

  static uint32_t lastWiFiCheckTick = 0;
  // 未连接1s重连
  if (WiFi.status() != WL_CONNECTED)
  {
    if (millis() - lastWiFiCheckTick > 1000)
    {
      lastWiFiCheckTick = millis();
      // 重新连接
      startSTA(g_wifiSSID, g_wifiPassword);
      Serial.printf("Heap size:%d\r\n", ESP.getFreeHeap());
    }
  }
  // 连接成功建立
  else
  {
    if (g_taskStarted == false)
    {
      g_taskStarted = true;
      Serial.print("\r\nGet IP Address: ");
      Serial.println(WiFi.localIP());
      startTCPClient();
    }
  }
}
/**********************
 *  等待wifi连接
 ***********************/
bool waitForConnect(String SSID, String password)
{
  bool wifiConnected = false;
  int status = 0;
  startSTA(SSID, password);
  uint32_t startConnectTime = millis();
  while (millis() - startConnectTime < WIFI_CONNECT_TIMEOUT)
  {
    // 检查 WiFi 连接状态
    if (WiFi.status() == WL_CONNECTED)
    {
      wifiConnected = true;
      Serial.println("\r\nWiFi connected successfully!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      // 如果成功连接上 WiFi，则启动 TCP 客户端
      startTCPClient();
      g_taskStarted = true;
      setLEDStatus(LED_OFF);
      return true;
    }
    else
    {
      // 如果未连接上 WiFi，则等待一段时间再重新检查
      delay(1000);
      // Serial.print("connecting...\n");
      // Serial.println("WiFi Mode:");
      // Serial.println(WiFi.getMode());
      // 连接时，闪烁
      status = ~status;
      setLEDStatus(status);
    }
  }
  Serial.println("wifi connect Faild!");
  return false;
}
/*********************************
 * softAP配网
 *
 **********************************/
void apConfig(String mac)
{
  if (g_config_flag == 1)
  {
    WiFi.softAP("maoshushu_" + mac);
    // 使用udp 8266端口
    Udp.begin(8266);
    Serial.println("Started Ap Config...");
  }
  String topic = mac;
  // 如果未配网，开启AP配网，一直等待并接收配网信息
  while (g_config_flag)
  {
    int packetSize = Udp.parsePacket();
    if (packetSize)
    {
      Serial.print("Received packet of size ");
      Serial.println(packetSize);
      Serial.print("From ");
      IPAddress remoteIp = Udp.remoteIP();
      Serial.print(remoteIp);
      Serial.print(", port ");
      Serial.println(Udp.remotePort());

      int len = Udp.read(packetBuffer, 255);
      if (len > 0)
      {
        packetBuffer[len] = 0;
      }
      Serial.println("Contents:");
      Serial.println(packetBuffer);
      StaticJsonDocument<200> doc;

      DeserializationError error = deserializeJson(doc, packetBuffer);
      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      int cmdType = doc["cmdType"].as<int>();
      ;

      if (cmdType == 1)
      {
        const char *ssid = doc["ssid"];
        const char *password = doc["password"];
        const char *token = doc["token"];
        // const char* topic = doc["topic"];
        Serial.println(cmdType);
        Serial.println(ssid);
        Serial.println(password);
        Serial.println(token);
        // 将配置保存到EEPROM
        EEPROM.begin(64);
        writeStringToEEPROM(0, ssid);
        writeStringToEEPROM(32, password);
        if (EEPROM.commit())
        {
          Serial.println("EEPROM successfully committed");
        }
        else
        {
          Serial.println("ERROR! EEPROM commit failed");
        }
        EEPROM.end();
        g_wifiSSID = ssid;
        g_wifiPassword = password;
        // 收到信息，并回复
        String ReplyBuffer = "{\"cmdType\":2,\"productId\":\"" + topic + "\",\"deviceName\":\"" + Name + "\",\"protoVersion\":\"" + proto + "\"}";
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        Udp.write(ReplyBuffer.c_str());
        Udp.endPacket();
      }
      else if (cmdType == 3)
      {
        // 配网信息传递结束
        g_config_flag = 0;
        WiFi.softAPdisconnect(true);
      }
    }
  }
}

// 将字符串保存到EEPROM
void writeStringToEEPROM(int addr, const String &str)
{
  for (unsigned int i = 0; i < str.length(); i++)
  {
    EEPROM.write(addr + i, str[i]);
  }
  EEPROM.write(addr + str.length(), '\0'); // 添加字符串结束符
}
// 从EEPROM中读取字符串
String readStringFromEEPROM(int addr)
{
  String result = "";
  char c;
  while ((c = EEPROM.read(addr++)) != '\0')
  {
    result += c;
  }
  return result;
}

static int resetTime = 0;
String test = "tommybei2";
String test2 = "tommybei";
// 初始化，相当于main 函数
void setup()
{
  Serial.begin(115200);
  // 初始化引脚为输出
  pinMode(SWITCH_Pin, OUTPUT);
  pinMode(LED_Status, OUTPUT);
  setLEDStatus(LED_OFF);
  SwitchSet(SWITCH_OFF);

  // 配置1表示没有配网
  g_config_flag == 1;
  // 延迟16s，串口才能有打印
  delay(15*1000);
  // 检查是否需要清除EEPROM
  checkAndResetEEPROM();

  // 当正常启动后，清除复位标志
  resetResetFlag();
  // 测试
  EEPROM.begin(EEPROM_FULL_ADDR);
  writeStringToEEPROM(EEPROM_SSID_ADDR, test);
  writeStringToEEPROM(EEPROM_PASSWORD_ADDR, test2);
  // 先从EEPROM读取WIFI相关配置信息
  read_SSID_eeprom(g_wifiSSID, g_wifiPassword);
  Serial.println("g_wifiSSID:" + g_wifiSSID);
  Serial.println("g_wifiPassword:" + g_wifiPassword);
  Serial.println("start wifi connnect\n");
  // 初始化 WiFi 连接状态为未连接
  bool wifiConnected = false;
  // 连续连接10s
  wifiConnected = waitForConnect(g_wifiSSID, g_wifiPassword);

  // 如果在规定的时间内未连接上 WiFi，则进入 AP 配置模式
  while (!wifiConnected)
  {
    // Serial.println("\r\nWiFi connection timeout. Entering AP configuration mode.");
    // setLEDStatus(LED_ON);
    // 进入 AP 配置模式,循环等待
    apConfig(deviceSN);
    // config_AP(deviceSN);
    // 进入WIFI_STA模式再判断wifi状态
    wifiConnected = waitForConnect(g_wifiSSID, g_wifiPassword);

    // 若配网失败，重置标志位
    if (wifiConnected == false)
    {
      g_config_flag = 1;
    }
  }

  // 失败指示灯
}

// 循环
void loop()
{
  // 查询WIFI连接状态
  doWiFiTick();
  doTCPClientTick();
}
