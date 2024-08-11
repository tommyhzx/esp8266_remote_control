#include <WiFiUDP.h>

void myFunction()
{
  Serial.println("Hello from myFunction!");
}
// 开启sta模式，并连接wifi
void startSTA(String ssid, String password)
{
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
}
/**********************
 *  等待wifi连接
 ***********************/
bool connect_WIFI(String SSID, String password)
{
  int LEDstatus = 0;

  startSTA(SSID, password);

  uint32_t startConnectTime = millis();
  while (millis() - startConnectTime < WIFI_CONNECT_TIMEOUT)
  {
    // 检查 WiFi 连接状态
    if (WiFi.status() == WL_CONNECTED)
    {
      // 打印连接成功信息
      Serial.println("\r\nWiFi connected successfully!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      // 连接成功，LED 灯关闭
      setLEDStatus(LED_OFF);
      return true;
    }
    else
    {
      // 如果未连接上 WiFi，则等待一段时间再重新检查
      delay(1000);
      Serial.print("connecting...\n");
      // 连接时，闪烁
      LEDstatus = ~LEDstatus;
      setLEDStatus(LEDstatus);
    }
  }
  Serial.println("wifi connect Faild!");
  return false;
}
/*********************************
 * softAP配网
 *@return true:配网成功 false:配网失败
 **********************************/
bool config_AP(String mac, String &wifi_ssid, String &wifi_pwd)
{
  char packetBuffer[255]; // 发送数据包
  String topic = mac;
  // 尚未连接，打开AP热点
  if (WiFi.getMode() != 3)
  {
    WiFi.softAP("maoshushu_" + mac);
    // 使用udp 8266端口
    Udp.begin(8266);
    Serial.println("Started Ap Config...");
  }
  else
  { // 已经连接，等待配网信息
    int packetSize = Udp.parsePacket();
    if (packetSize)
    {
      Serial.println("Received packet of size " + packetSize);
      IPAddress remoteIp = Udp.remoteIP();

      int len = Udp.read(packetBuffer, 255);
      if (len > 0)
      {
        packetBuffer[len] = 0;
      }
      Serial.println("UDP packet contents: " + String(packetBuffer));
      // 解析json数据
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, packetBuffer);
      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        return false;
      }
      int cmdType = doc["cmdType"].as<int>();
      if (cmdType == 1)
      {
        const char *ssid = doc["ssid"];
        const char *password = doc["password"];
        const char *token = doc["token"];
        // const char* topic = doc["topic"];

        wifi_ssid = ssid;
        wifi_pwd = password;
        Serial.println("wifi_ssid:" + wifi_ssid + " wifi_pwd:" + wifi_pwd);     
        // 收到信息，并回复
        String ReplyBuffer = "{\"cmdType\":2,\"productId\":\"" + topic + "\",\"deviceName\":\"" + Name + "\",\"protoVersion\":\"" + proto + "\"}";
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        Udp.write(ReplyBuffer.c_str());
        Udp.endPacket();
      }
      else if (cmdType == 3)
      {
        // 配网信息传递结束
        WiFi.softAPdisconnect(true);
        return true;
      }
    }
  }
  // 未完成配网
  return false;
}

/*
  WiFiTick
  检查是否需要初始化WiFi
  检查WiFi是否连接上，若连接成功启动TCP Client
  控制指示灯
*/
void check_and_reconnect_wifi()
{

  static uint32_t lastWiFiCheckTick = 0;
  String wifiSSID = "";
  String wifiPassword = "";
  // 未连接1s重连
  if (WiFi.status() != WL_CONNECTED)
  {
    if (millis() - lastWiFiCheckTick > 1000)
    {
      lastWiFiCheckTick = millis();
      Serial.println("WiFi not connected, reconnecting...");
      // 重新连接
      read_SSID_eeprom(wifiSSID, wifiPassword);
      startSTA(wifiSSID, wifiPassword);
    }
  }
}