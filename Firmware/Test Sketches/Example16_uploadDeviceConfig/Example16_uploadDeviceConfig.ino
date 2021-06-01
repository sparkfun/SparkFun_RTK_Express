/*
  Upload a configuration file to a module
  By: SparkFun Electronics / Nathan Seidle
  Date: May 19th, 2021
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.

  This example shows how to upload a u-center type HEX file to configure the entire ZED.
  Currently, we do not check if the config file is compatible with the device so be very careful.

  Feel like supporting open source hardware?
  Buy a board from SparkFun!
  ZED-F9P RTK2: https://www.sparkfun.com/products/15136
  NEO-M8P RTK: https://www.sparkfun.com/products/15005

  Hardware Connections:
  Plug a Qwiic cable into the GNSS and a BlackBoard
  If you don't have a platform with a Qwiic connection use the SparkFun Qwiic Breadboard Jumper (https://www.sparkfun.com/products/14425)
  Open the serial monitor at 115200 baud to see the output
*/

#include <Wire.h> //Needed for I2C to GNSS

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;

char inData[] = "CFG-VALGET - 06 8B 44 01 01 00 00 00 01 00 01 10 00 01 01 01 10 00 09 00 04 10 00 07 00 05 10 01 08 00 05 10 01 09 00 05 10 01 0A 00 05 10 01 0B 00 05 10 01 33 00 05 10 01 12 00 11 10 00 13 00 11 10 00 14 00 11 10 01 15 00 11 10 01 16 00 11 10 01 18 00 11 10 01 1B 00 11 10 00 25 00 11 10 00 46 00 11 10 01 52 00 11 10 00 53 00 11 10 00 61 00 11 10 00 81 00 11 10 00 82 00 11 10 00 83 00 11 10 00 51 00 14 10 00 52 00 14 10 00 53 00 14 10 00 05 00 21 10 00 01 00 22 10 00 02 00 22 10 00 03 00 22 10 00 04 00 22 10 00 12 00 24 10 00 20 00 24 10 00 30 00 24 10 00 40 00 24 10 00 50 00 24 10 00 01 00 25 10 01 01 00 31 10 01 03 00 31 10 01 05 00 31 10 01 07 00 31 10 01 0A 00 31 10 01 0D 00 31 10 01 0E 00 31 10 01 12 00 31 10 01 14 00 31 10 00 15 00 31 10 01 18 00 31 10 01 1A 00 31 10 01 1F 00 31 10 01 20 00 31 10 01 21 00 31 10 01 22 00 31 10 01 24 00 31 10 01 25 00 31 10 01 27 00 31 10 01 01 00 33 10 00 02 00 33 10 00 03 00 33 10 00 11 00 33 10 00 01 00 34 10 00 02 00 34 10 00 03 00 34 10 01";

void setup()
{
  Serial.begin(115200); // You may need to increase this for high navigation rates!
  while (!Serial)
    ; //Wait for user to open terminal
  Serial.println(F("SparkFun u-blox Example"));

  Wire.begin();
  Wire.setClock(400000);

  myGNSS.enableDebugging(); // Uncomment this line to enable debug messages

  if (myGNSS.begin() == false) //Connect to the u-blox module using Wire port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
    while (1)
      ;
  }

  myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)

  myGNSS.setPacketCfgPayloadSize(1024); //Increase max payload to allow for 580 bytes of config data

  //myGNSS.i2cTransactionSize = 64;
}

void loop()
{

  if (Serial.available())
  {
    byte incoming = Serial.read();

    if (incoming == 'a')
    {
      Serial.println("Uploading config data");
      uploadDeviceConfig(inData, strlen(inData));

      Serial.println("Config write complete!");
    }
  }
}


void uploadDeviceConfig(char *deviceData, uint16_t len)
{
  char *token;
  int tokenCounter;

  //Get past "CFG-VALGET - " and first 8 bytes of data. sendVal already computes these for us.
  token = strtok(deviceData, " ");
  tokenCounter = 0;
  while (token != NULL)
  {
    if (tokenCounter == 8) break;
    token = strtok(NULL, " "); //Get next token
    tokenCounter++;
  }

  //Turn the remainder into a binary array
  uint8_t myData[1024];
  memset(myData, 0xAC, 1024);

  token = strtok(NULL, " "); //Pickup where we left off
  tokenCounter = 0;
  while (token != NULL)
  {
    token = strtok(NULL, " "); //Get next token
    if (token == NULL) break;

    myData[tokenCounter] = htoi(token);
    tokenCounter++;
  }

  //Debug output
//  Serial.print(F("myData("));
//  Serial.print(tokenCounter);
//  Serial.println(F(" bytes):"));
//  for (int x = 0 ; x < tokenCounter ; x++)
//  {
//    //if(x % 32 == 0) Serial.println();
//    Serial.print(" ");
//    if (myData[x] < 0x10) Serial.print("0");
//    Serial.print(myData[x], HEX);
//  }
//  Serial.println();

  //Send this data to device
  myGNSS.setVal(myData, tokenCounter); //data, len, layer, timeout
}

//Convert HEX string to integer
//http://johnsantic.com/comp/htoi.html
uint8_t htoi (const char *ptr)
{
  unsigned int value = 0;
  char ch = *ptr;

  while (ch == ' ' || ch == '\t')
    ch = *(++ptr);

  for (;;) {

    if (ch >= '0' && ch <= '9')
      value = (value << 4) + (ch - '0');
    else if (ch >= 'A' && ch <= 'F')
      value = (value << 4) + (ch - 'A' + 10);
    else if (ch >= 'a' && ch <= 'f')
      value = (value << 4) + (ch - 'a' + 10);
    else
      return value;
    ch = *(++ptr);
  }
}
