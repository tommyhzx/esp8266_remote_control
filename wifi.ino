
void myFunction() {
    Serial.println("Hello from myFunction!");
}
/*********************************
* softAP配网
*
**********************************/
void config_AP(String mac){
  char packetBuffer[255]; //发送数据包
  String topic = mac;
  // 尚未连接，打开AP热点
  if(WiFi.getMode() != 2){
    WiFi.softAP("maoshushu_"+mac);
    //使用udp 8266端口
    Udp.begin(8266);
    Serial.println("Started Ap Config...");
    // Serial.println("WiFi Mode:");
    // Serial.println(WiFi.getMode());
  }
  else{
    int packetSize = Udp.parsePacket();
    if (packetSize) {
        Serial.print("Received packet of size ");
        Serial.println(packetSize);
        Serial.print("From ");
        IPAddress remoteIp = Udp.remoteIP();
        Serial.print(remoteIp);
        Serial.print(", port ");
        Serial.println(Udp.remotePort());
    
        int len = Udp.read(packetBuffer, 255);
        if (len > 0) {
          packetBuffer[len] = 0;
        }
        Serial.println("Contents:");
        Serial.println(packetBuffer);
        StaticJsonDocument<200> doc;
    
        DeserializationError error = deserializeJson(doc, packetBuffer);
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return;
        }
        int cmdType = doc["cmdType"].as<int>();
        if (cmdType == 1) {
              const char* ssid = doc["ssid"];
              const char* password = doc["password"];
              const char* token = doc["token"];
              //const char* topic = doc["topic"];
              Serial.println(cmdType);
              Serial.println(ssid);
              Serial.println(password);
              Serial.println(token);
              //将配置保存到EEPROM
              EEPROM.begin(64);
              writeStringToEEPROM(0, ssid);
              writeStringToEEPROM(32, password);
              if (EEPROM.commit()) {
                  Serial.println("EEPROM successfully committed");
                } else {
                  Serial.println("ERROR! EEPROM commit failed");
                }
              EEPROM.end();
              g_wifiSSID = ssid;
              g_wifiPassword = password;
              
              //收到信息，并回复
              String  ReplyBuffer = "{\"cmdType\":2,\"productId\":\""+topic+"\",\"deviceName\":\""+Name+"\",\"protoVersion\":\""+proto+"\"}";
              Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
              Udp.write(ReplyBuffer.c_str());
              Udp.endPacket();
            
          }else if(cmdType == 3){
            //配网信息传递结束
              g_config_flag = 0;
              WiFi.softAPdisconnect(true);
          }
    }
  }
}