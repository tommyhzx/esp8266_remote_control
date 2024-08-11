#include <ESP8266WebServer.h>
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
}
/*****************************************
 *初始化和服务器建立连接
 *****************************************/
void startTCPClient()
{
  if (TCPclient.connect(TCP_SERVER_ADDR, atoi(TCP_SERVER_PORT)))
  {
    Serial.println("Connected to server:");
    Serial.printf("%s:%d\r\n", TCP_SERVER_ADDR, atoi(TCP_SERVER_PORT));
    // 连接上TCP时，先发布一次订阅
    String tcpTemp = "";
    tcpTemp = "cmd=1&uid=" + UID + "&topic=" + TOPIC + "\r\n";
    sendtoTCPServer(tcpTemp);
    Serial.println("sbscrib topic " + String(TOPIC));

    TCPclient.setNoDelay(true);
  }
  else
  {
    Serial.println("Failed connected to server:" + String(TCP_SERVER_ADDR));
    TCPclient.stop();
  }
}