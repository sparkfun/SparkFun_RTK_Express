/*

*/

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS i2cGNSS;
#define MAX_PAYLOAD_SIZE 384 // Override MAX_PAYLOAD_SIZE for getModuleInfo which can return up to 348 bytes
uint8_t settingPayload[MAX_PAYLOAD_SIZE];

#include "settings.h"

const byte menuTimeout = 15; //Menus will exit/timeout after this number of seconds

void setup()
{
  Serial.begin(115200);
  delay(250);
  Serial.println("Sentence Lookup");

  Wire.begin();

  if (i2cGNSS.begin() == false)
  {
    //Try again with power on delay
    delay(1000); //Wait for ZED-F9P to power up before it can respond to ACK
    if (i2cGNSS.begin() == false)
    {
      Serial.println(F("u-blox GNSS not detected at default I2C address. Hard stop."));
      while (1);
    }
  }

  Serial.println("0) ZED TX out/RX in");
  Serial.println("1) PPS out/Ext Int in");
  Serial.println("2) SCL out/SDA in");
  Serial.println("3) ESP26 DAC out/ESP39 ADC in");

  //settings.messageSendRate.nmea_gga = 2;

  configureGNSSMessageRates();
}

void loop()
{
  if (Serial.available())
  {
    byte incoming = Serial.read();

    if (incoming == '0')
    {
      Serial.println("ZED TX out/RX in");
    }
    else if (incoming == '1')
    {
      Serial.println("PPS out/Ext Int in");
    }
    else if (incoming == '2')
    {
      Serial.printf("before dtm: %d\n", settings.message.nmea_dtm.msgRate);
      inputMessageRate(settings.message.nmea_dtm);
      Serial.printf("after dtm: %d\n", settings.message.nmea_dtm.msgRate);
    }
    else if (incoming == '3')
    {
      Serial.printf("before: %d\n", settings.message.nmea_gga.msgRate);
      inputMessageRate(settings.message.nmea_gga);
      Serial.printf("after: %d\n", settings.message.nmea_gga.msgRate);
    }
  }

  delay(100);

}

//Prompt the user to enter the message rate for a given ID
//Assign the given value to the message
void inputMessageRate(ubxMsg &message)
{
  Serial.printf("Enter %s message rate (0 to disable): ", message.msgTextName);
  int64_t rate = getNumber(menuTimeout); //Timeout after x seconds

  while (rate < 0 || rate > 60) //Arbitrary 60 fixes per report limit
  {
    Serial.println(F("Error: message rate out of range"));
    Serial.printf("Enter %s message rate (0 to disable): ", message.msgTextName);
    rate = getNumber(menuTimeout); //Timeout after x seconds

    if (rate == STATUS_GETNUMBER_TIMEOUT || rate == STATUS_PRESSED_X)
      return; //Give up
  }

  if (rate == STATUS_GETNUMBER_TIMEOUT || rate == STATUS_PRESSED_X)
    return;

  message.msgRate = rate;
}


//Updates the message rates on the ZED-F9P for all known messages
bool configureGNSSMessageRates()
{
  long startTime = millis();

  bool response = true;

  //NMEA
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_dtm);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_gbs);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_gga);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_gll);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_gns);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_grs);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_gsa);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_gst);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_gsv);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_rmc);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_vlw);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_vtg);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_zda);

  //NAV
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_clock);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_dop);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_eoe);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_geofence);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_hpposecef);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_hpposllh);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_odo);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_orb);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_posecef);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_posllh);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_pvt);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_relposned);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_sat);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_sig);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_status);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_svin);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_timebds);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_timegal);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_timeglo);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_timegps);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_timels);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_timeutc);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_velecef);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_velned);

  //RXM
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rxm_measx);
  //response &= configureMessageRate(COM_PORT_UART1, settings.message.rxm_pmreq);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rxm_rawx);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rxm_rlm);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rxm_rtcm);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.rxm_sfrbx);

  //MON
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_comms);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_hw2);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_hw3);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_hw);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_io);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_msgpp);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_rf);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_rxbuf);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_rxr);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_txbuf);
  
  //TIM
  response &= configureMessageRate(COM_PORT_UART1, settings.message.tim_tm2);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.tim_tp);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.tim_vrfy);

  //RTCM
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1005);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1074);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1077);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1084);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1087);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1094);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1097);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1124);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1127);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1230);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_4072_0);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_4072_1);

  long stopTime = millis();

  Serial.printf("configureMessateRate time: %ld ms", stopTime - startTime);
  if (response == true)
    Serial.println(" Success");
  else
    Serial.println(" Fail");

  return (response);
}


//Given a message, set the message rate on the ZED-F9P
bool configureMessageRate(uint8_t portID, ubxMsg message)
{
  uint8_t currentSendRate = getMessageRate(message.msgClass, message.msgID, portID); //Qeury the module for the current setting

  bool response = true;
  if (currentSendRate != message.msgRate)
    response &= i2cGNSS.configureMessage(message.msgClass, message.msgID, portID, message.msgRate); //Update setting
  return response;
}

//Lookup the send rate for a given message+port
uint8_t getMessageRate(uint8_t msgClass, uint8_t msgID, uint8_t portID)
{
  ubxPacket customCfg = {0, 0, 0, 0, 0, settingPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_MSG; // This is the message ID
  customCfg.len = 2;
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 1250; // Wait for up to 1250ms (Serial may need a lot longer e.g. 1100)

  settingPayload[0] = msgClass;
  settingPayload[1] = msgID;

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.printf("getMessageSetting failed: Class-0x%02X ID-0x%02X\n\r", msgClass, msgID);
    return (false);
  }

  uint8_t sendRate = settingPayload[2 + portID];

  return (sendRate);
}



//Given a setting name, get library define
//uint8_t lookupMessage(long *settingName)
//{
//  switch ((const long)settingName)
//  {
//    case ((const long) ((long)&settings.message.nmea_gga)):
//      return (UBX_NMEA_GGA);
//    default:
//      return (UBX_NMEA_GGA);
//  }
//
//  return(1);
//}
