#ifdef USES_P101

// #######################################################################################################
// ######################## Plugin 101: Temperature and Humidity sensor AHT10 (I2C) ######################
// #######################################################################################################

#include "_Plugin_Helper.h"

#define PLUGIN_101
#define PLUGIN_ID_101         101
#define PLUGIN_NAME_101       "Environment - AHT10 (I2C) [TESTING]"
#define PLUGIN_VALUENAME1_101 "Temperature"
#define PLUGIN_VALUENAME2_101 "Humidity"

#define AHT10_I2C_ADDRESS      PCONFIG(0)

#define AHT10_BUSY             Plugin_101_isBusy(event)
#define AHT10_CALIBRATED       Plugin_101_isCalibrated(event)
#define AHT10_INIT             Plugin_101_init(event)

struct sensorData {
  float temperature;
  float humidity;
};

boolean Plugin_101(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      Device[++deviceCount].Number           = PLUGIN_ID_101;
      Device[deviceCount].Type               = DEVICE_TYPE_I2C;
      Device[deviceCount].VType              = Sensor_VType::SENSOR_TYPE_TEMP_HUM;
      Device[deviceCount].Ports              = 0;
      Device[deviceCount].PullUpOption       = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption      = true;
      Device[deviceCount].ValueCount         = 2;
      Device[deviceCount].SendDataOption     = true;
      Device[deviceCount].TimerOption        = true;
      Device[deviceCount].GlobalSyncOption   = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_101);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_101));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_101));
      break;
    }

    case PLUGIN_WEBFORM_SHOW_I2C_PARAMS:
    {
      int optionValues[2] = { 0x38, 0x39 };
      addFormSelectorI2C(F("i2c_addr"), 2, optionValues, PCONFIG(0));
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      PCONFIG(0) = getFormItemInt(F("i2c_addr"));

      success = true;
      break;
    }

    case PLUGIN_INIT:
    {
      String log = getTaskDeviceName(event->TaskIndex);

      if(AHT10_INIT){
        log += F(": INIT OK");
        addLog(LOG_LEVEL_INFO, log);
      }else{
        log += F(": INIT FAIL");
        addLog(LOG_LEVEL_ERROR, log);
      }
      break;
    }

    case PLUGIN_READ:
    {
      String log = getTaskDeviceName(event->TaskIndex);
      struct sensorData data;

      if(AHT10_BUSY) {
        addLog(LOG_LEVEL_ERROR, log+F(": Device is busy."))
        delay(100);
        success = false;
        break;
      }
      Plugin_101_readSensor(event, &data);
      if(data.humidity > 100) {
        UserVar[event->BaseVarIndex]     = NAN;
        UserVar[event->BaseVarIndex + 1] = NAN;
        log += F(": ERROR DATA ");
        log += UserVar[event->BaseVarIndex + 1];
        addLog(LOG_LEVEL_ERROR, log);
        success = false;
        break;
      }else if(data.humidity > 0){
        UserVar[event->BaseVarIndex]     = data.temperature;
        UserVar[event->BaseVarIndex + 1] = data.humidity;
        success = true;
      }

      log += F(": Temperature: ");
      log += UserVar[event->BaseVarIndex];
      log += F("; Humidity: ");
      log += UserVar[event->BaseVarIndex + 1];
      addLog(LOG_LEVEL_INFO, log);

      break;
    }
  }
  return success;
}

unsigned char Plugin_101_readStatus(struct EventStruct *event)
{
    Wire.requestFrom(AHT10_I2C_ADDRESS, 1);
    return Wire.read();
}

void Plugin_101_readSensor(struct EventStruct *event, struct sensorData *data)
{
    u_long ans[6];

    Wire.beginTransmission(AHT10_I2C_ADDRESS);
    Wire.write(0xAC); 
    Wire.write(0x33);   
    Wire.write(0x00);
    Wire.endTransmission();
    delay(100);

    Wire.requestFrom(AHT10_I2C_ADDRESS, 6);

    for(u_char i = 0; Wire.available() > 0; i++)
    {
        ans[i] = Wire.read();
    }   

    data->humidity = (((ans[1] << 16) | (ans[2] << 8) | ans[3]) >> 4) * 100 / 0x100000;;
    data->temperature = (((ans[3] & 0x0F) << 16) | (ans[4] << 8) | ans[5]) * 200 / 0x100000 - 50;
}

void Plugin_101_reset(struct EventStruct *event)
{
  Wire.beginTransmission(AHT10_I2C_ADDRESS);
  Wire.write(0xBA); 
  Wire.endTransmission();
  delay(20);
}

bool Plugin_101_init(struct EventStruct *event)
{
  delay(40);
  //set normal mode(read & wait)
  Wire.beginTransmission(AHT10_I2C_ADDRESS);
  Wire.write(0xA8); 
  Wire.write(0x00);   
  Wire.write(0x00); 
  Wire.endTransmission();
  delay(350);
  
  //load factory cal coef
  Wire.beginTransmission(AHT10_I2C_ADDRESS);
  Wire.write(0xE1); 
  Wire.write(0x08);   
  Wire.write(0x00);  
  Wire.endTransmission();
  delay(500);

  return AHT10_CALIBRATED && ! AHT10_BUSY;
}

bool Plugin_101_isBusy(struct EventStruct *event)
{
  byte status = Plugin_101_readStatus(event);
  return bitRead(status, 7);
}

bool Plugin_101_isCalibrated(struct EventStruct *event)
{
  byte status = Plugin_101_readStatus(event);
  return bitRead(status, 3);
}

#endif // USES_P101
