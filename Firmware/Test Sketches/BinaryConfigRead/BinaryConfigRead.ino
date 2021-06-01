/*

*/

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS i2cGNSS;

#define MAX_PAYLOAD_SIZE 600 // Override MAX_PAYLOAD_SIZE for binary config read can return up to 580 bytes
uint8_t payloadCfg[MAX_PAYLOAD_SIZE];
ubxPacket packetCfg = {0, 0, 0, 0, 0, payloadCfg, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

void setup()
{
  Serial.begin(115200);
  delay(250);
  Serial.println("Receiver Config Read");

  Wire.begin();
  Wire.setClock(400000);

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

  //i2cGNSS.enableDebugging();

  i2cGNSS.setPacketCfgPayloadSize(1024); //Increase max payload to allow for 580 byte key responses

  //Download config data to Serial port
  //Read the RAM layer. Currently only RAM layer is tested.
  //Device can take up to 1000ms to respond with keys
  //downloadDeviceConfig(Serial, VAL_LAYER_RAM, 1000); 
  i2cGNSS.downloadDeviceConfig();
  Serial.println("Config read complete!");
}

void loop()
{
  delay(100);
}

//Read 18 sets of key values, for a total of 1088 keys
//Format the output to match the config files that u-center can understand
//Layer number can be 0 (RAM) or layer 7 (Default)
void downloadDeviceConfig(Stream &port, uint8_t layerNumber, uint16_t maxWait)
{
  //Stream *downloadPort = &port;

  for (int x = 0 ; x < 18 ; x++)
  {
    i2cGNSS.getVal(0x0FFF0000, layerNumber, x * 64, maxWait); //Advance by 64 keys each time

    //All lines start with a VALGET
    port.print(F("CFG-VALGET - 06 8B "));

//    //Pretty print the response length
//    uint16_t responseLength = packetCfg.len;
//    if ((responseLength & 0xFF) < 0x10) downloadPort->print(F("0"));
//    downloadPort->print(responseLength & 0xFF, HEX);
//    downloadPort->print(F(" "));
//    if ((responseLength >> 8) < 0x10) downloadPort->print(F("0"));
//    downloadPort->print(responseLength >> 8, HEX);
//
//    //Pretty print the payload
//    for (int x = 0 ; x < 32 ; x++)
//    {
//      downloadPort->print(F(" "));
//      if (payloadCfg[x] < 0x10) downloadPort->print(F("0"));
//      downloadPort->print(payloadCfg[x], HEX);
//    }
//    downloadPort->println();
  }
}

//Using getVal, get a batch of 64 key values
//Gen9 seems to have 1088 key values so 18 reads are neccesary.
//The max response size is 580 bytes
uint8_t getDeviceConfiguration(uint16_t startingAddress, uint8_t layer, uint16_t maxWait)
{
  uint32_t key = 0x0FFF0000; //Magic config read key. Found using u-center and downloading Receiver Configuration from Tools menu.

  packetCfg.cls = UBX_CLASS_CFG; // This is the message Class
  packetCfg.id = UBX_CFG_VALGET; // This is the message ID
  packetCfg.len = 4 + 4 * 1; //While multiple keys are allowed, we will send only one key at a time
  packetCfg.startingSpot = 0;

  //Clear packet payload
  memset(payloadCfg, 0, packetCfg.len);

  //VALGET uses different memory layer definitions to VALSET
  //because it can only return the value for one layer.
  //So we need to fiddle the layer here.
  //And just to complicate things further, the ZED-F9P only responds
  //correctly to layer 0 (RAM) and layer 7 (Default)!
  uint8_t getLayer = 7;                         // 7 is the "Default Layer"
  if ((layer & VAL_LAYER_RAM) == VAL_LAYER_RAM) // Did the user request the RAM layer?
  {
    getLayer = 0; // Layer 0 is RAM
  }

  getLayer = 0; // Layer 0 is RAM

  payloadCfg[0] = 0;        //Message Version - set to 0
  payloadCfg[1] = getLayer; //Layer

  payloadCfg[2] = startingAddress >> 8 * 0; //Position - skip this many key values
  payloadCfg[3] = startingAddress >> 8 * 1;

  //Load key into outgoing payload
  payloadCfg[4] = key >> 8 * 0; //Key LSB
  payloadCfg[5] = key >> 8 * 1;
  payloadCfg[6] = key >> 8 * 2;
  payloadCfg[7] = key >> 8 * 3;

  //  if (_printDebug == true)
  //  {
  //    _debugSerial->print(F("key: 0x"));
  //    _debugSerial->print(key, HEX);
  //    _debugSerial->println();
  //  }

  //Send VALGET command with this key

  sfe_ublox_status_e retVal = i2cGNSS.sendCommand(&packetCfg, maxWait);
  //  if (_printDebug == true)
  //  {
  //    _debugSerial->print(F("getVal: sendCommand returned: "));
  //    _debugSerial->println(statusString(retVal));
  //  }

  //The response is now sitting in payload, ready for extraction
  return (retVal);
}
