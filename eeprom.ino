#include <EEPROM.h>

#define EEPROM_FULL_ADDR 64     // EEPROM的全部地址大小
#define EEPROM_SSID_ADDR 0      // 保存在EEPROM的SSID的地址
#define EEPROM_PASSWORD_ADDR 32 // 保存在EEPROM的PASSWORD的地址
#define EEPROM_RST_ADDR 60      // 保存在EEPROM的复位次数的地址
#define EEPROM_TIMESTAMP_ADDR 61

#define RESET_THRESHOLD 3
#define RESET_TIME_WINDOW 10000 // 10 seconds in milliseconds

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

// 输入参数：地址长度，最大64
void clear_eeprom(int addr_length)
{
  EEPROM.begin(addr_length);
  for (int i = 0; i < addr_length; i++)
  {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  EEPROM.end();
}

// 从eeprom的相关地址获取ssid以及password内容
void read_SSID_eeprom(String &ssid, String &pwd)
{
  EEPROM.begin(EEPROM_FULL_ADDR);
  ssid = readStringFromEEPROM(EEPROM_SSID_ADDR);
  pwd = readStringFromEEPROM(EEPROM_PASSWORD_ADDR);
  ssid.trim();
  pwd.trim();
  EEPROM.end();
}
// 保存配置到EEPROM
void save_config_EEPROM(String ssid, String password)
{
  EEPROM.begin(EEPROM_FULL_ADDR);
  writeStringToEEPROM(EEPROM_SSID_ADDR, ssid);
  writeStringToEEPROM(EEPROM_PASSWORD_ADDR, password);
  if (EEPROM.commit())
  {
    Serial.println("EEPROM successfully committed");
  }
  else
  {
    Serial.println("ERROR! EEPROM commit failed");
  }
  EEPROM.end();
}

// 连续复位3次则清除eeprom的内容
void checkAndResetEEPROM()
{
  uint32_t resetTime = 0;
  // 连续复位3次，则回复出厂设置
  EEPROM.begin(EEPROM_FULL_ADDR);
  resetTime = EEPROM.read(EEPROM_RST_ADDR);
  resetTime++;
  EEPROM.write(EEPROM_RST_ADDR, resetTime);
  EEPROM.commit();
  // 连续复位3次，恢复出厂设置
  if (resetTime >= 3)
  {
    clear_eeprom(EEPROM_FULL_ADDR);
    Serial.println("clear EEPROM config!");
  }
  EEPROM.end();
}
// 帮我用标准的格式写一个函数的说明注释

/* ************************** */
/* 重置复位标志
/* ************************** */
void resetResetFlag()
{
  EEPROM.begin(EEPROM_FULL_ADDR);
  EEPROM.write(EEPROM_RST_ADDR, 0);
  EEPROM.commit();
  EEPROM.end();
}
