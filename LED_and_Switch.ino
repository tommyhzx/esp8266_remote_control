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