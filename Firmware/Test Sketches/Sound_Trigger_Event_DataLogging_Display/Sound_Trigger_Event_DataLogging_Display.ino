/*
  Records events from Qwiic Sound Trigger (VM1010) with the u-blox ZED-F9P and UBX TIM_TM2 messages
  By: Paul Clark
  SparkFun Electronics
  Date: March 10th, 2021
  License: MIT. Please see LICENSE.md for more details.

  Feel like supporting our work? Buy a board from SparkFun!
  Qwiic Sound Trigger:                 https://www.sparkfun.com/products/17979
  ZED-F9P RTK SMA:                     https://www.sparkfun.com/products/16481
  ZED-F9P RTK2:                        https://www.sparkfun.com/products/15136
  MicroMod Data Logging Carrier Board: https://www.sparkfun.com/products/16829
  MicroMod Artemis Processor Board:    https://www.sparkfun.com/products/16401

  Based on the u-blox GNSS library example DataLoggingExample2_TIM_TM2:
  Configuring the ZED-F9P GNSS to automatically send TIM TM2 reports over I2C and log them to file on SD card

  This example shows how to configure the u-blox ZED-F9P GNSS to send TIM TM2 reports when a sound event is detected
  and automatically log the data to SD card in UBX format.

  To minimise I2C bus errors, it is a good idea to open the I2C pull-up split pad links on
  the MicroMod Data Logging Carrier Board and the u-blox module breakout and the Qwiic Sound Trigger.

  Each time the Qwiic Sound Trigger detects a sound, it pulls its TRIG pin high. The ZED-F9P will
  detect this on its INT pin and generate a TIM TM2 message. The Artemis will log the TIM TM2 message
  to SD card. You can then study the timing of the sound pulse with nanosecond resolution!
  The code will "debounce" the sound event and reset the VM1010 for the next sound event after 250ms.

  Note: TIM TM2 can only capture the timing of one rising edge and one falling edge per
  navigation solution. So with setNavigationFrequency set to 4Hz, we can only see the timing
  of one rising edge every 250ms. The code "debounces" each sound event to make sure there will
  only be one rising edge event per navigation solution.

  TIM TM2 messages are only produced when a rising or falling edge is detected on the INT pin.
  If you disconnect your TRIG to INT jumper wire, the messages will stop.

  Data is logged in u-blox UBX format. Please see the u-blox protocol specification for more details.
  You can replay and analyze the data using u-center:
  https://www.u-blox.com/en/product/u-center

*/

//Hardware connections
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//RTK Express v11
const int muxA = 2;
const int muxB = 4;

const int sdChipSelect = 25; //Primary SPI Chip Select is CS for the MicroMod Artemis Processor. Adjust for your processor if necessary.
const int powerSenseAndControl = 13;
const int setupButton = 14;
const int powerFastOff = 27;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//External Display
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SFE_MicroOLED.h> //Click here to get the library: http://librarymanager/All#SparkFun_Micro_OLED
//#include "icons.h"

#define PIN_RESET 9
#define DC_JUMPER 1
MicroOLED oled(PIN_RESET, DC_JUMPER);
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//I2C GPIO setup
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_PCA9536_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_PCA9536
PCA9536 myTrigger;

#define VM1010_MODE 0 // The VM1010 mode pin is connected to GPIO0 on the PCA9536
#define VM1010_TRIG 1 // The VM1010 trigger pin (Dout) is connected to GPIO1 on the PCA9536
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//SD for file logging
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SPI.h>
#include <SD.h>

//File myFile; //File that all GNSS data is written to
const char ubxFileName[30] = "/TIM_TM2.ubx"; //Contains the event times in UBX format
const char locationFileName[30] = "/locationData.txt"; //Contains NMEA location data of the mic's location
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//I2C and GNSS for time events
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <Wire.h> //Needed for I2C to GNSS and PCA9536

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;
#define packetLength 36 // TIM TM2 is 28 + 8 bytes in length (including the sync chars, class, id, length and checksum bytes)
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Global variables
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
int dotsPrinted = 0; // Print dots in rows of 50 while waiting for a TIM TM2 message
long lastUbxCountUpdate = 0; //Times when printing of Ubx byte count occurs
long ubxCount = 0; //Number of bytes written to SD
long triggerCounts = 0; //For display
long lastFixRecordTime = 0; //Record our location every 5 seconds
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void setup()
{
  Serial.begin(115200);
  delay(250);
  Serial.println("SparkFun Qwiic Sound Trigger Example");

  pinMode(muxA, OUTPUT);
  pinMode(muxB, OUTPUT);
  setMuxport(1);
  Serial.println("Mux set to PPS out/Ext Int in");

  pinMode(powerSenseAndControl, INPUT);
  pinMode(powerFastOff, INPUT);


  Wire.begin(); // Start I2C communication with the GNSS and the Qwiic Sound Trigger

  beginDisplay(); //Check if an external Qwiic OLED is attached

  if (myTrigger.begin() == false)
  {
    Serial.println(F("Sound Trigger (PCA9536) not detected. Please check wiring. Freezing..."));
    while (1)
      ;
  }

  myTrigger.pinMode(VM1010_TRIG, INPUT);
  myTrigger.pinMode(VM1010_MODE, INPUT);

  Serial.println("Initializing SD card...");

  // See if the card is present and can be initialized:
  if (!SD.begin(sdChipSelect))
  {
    Serial.println("Card failed, or not present. Freezing...");
    while (1);
  }
  Serial.println("SD card initialized.");

  // Create or open a file called "TIM_TM2.ubx" on the SD card.
  // If the file already exists, the new data is appended to the end of the file.
  File myFile = SD.open(ubxFileName, FILE_APPEND);
  if (!myFile)
  {
    Serial.println(F("Failed to create UBX data file! Freezing..."));
    while (1);
  }
  myFile.close(); // Close the log file. It will be reopened when a sound event is detected.

  //myGNSS.enableDebugging(); // Uncomment this line to enable helpful GNSS debug messages on Serial

  myGNSS.setFileBufferSize(361); // setFileBufferSize must be called _before_ .begin

  if (myGNSS.begin() == false) //Connect to the u-blox module using Wire port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing..."));
    while (1);
  }

  myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); //Save (only) the communications port settings to flash and BBR

  myGNSS.setNavigationFrequency(4); //Produce four navigation solutions per second

  myGNSS.setAutoTIMTM2callback(&printTIMTM2data); // Enable automatic TIM TM2 messages with callback to printTIMTM2data

  myGNSS.logTIMTM2(); // Enable TIM TM2 data logging

  while (Serial.available()) // Make sure the Serial buffer is empty
    Serial.read();

  Serial.println(F("Press any key to stop logging."));

  resetSoundTrigger(); // Reset the sound trigger - in case it has already been triggered
}

void loop()
{
  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  // Check to see if a new packetLength-byte TIM TM2 message has been stored
  // This indicates if a sound event was detected
  if (myGNSS.fileBufferAvailable() >= packetLength)
  {
    triggerCounts++;

    uint8_t myBuffer[packetLength]; // Create our own buffer to hold the data while we write it to SD card

    myGNSS.extractFileBufferData((uint8_t *)&myBuffer, packetLength); // Extract exactly packetLength bytes from the UBX file buffer and put them into myBuffer

    File ubxFile = SD.open(ubxFileName, FILE_APPEND); // Reopen the file
    if (!ubxFile)
    {
      Serial.println("Failed to reopen log");
    }

    ubxFile.write(myBuffer, packetLength); // Write exactly packetLength bytes from myBuffer to the ubxDataFile on the SD card

    ubxFile.flush();

    if (millis() - lastUbxCountUpdate > 1000)
    {
      lastUbxCountUpdate = millis();
      Serial.printf("UBX file size and kB/s: %d / %0.02f\n", ubxFile.size(), (float)ubxFile.size() / millis());
    }

    //Update display
    oled.clear(PAGE); // Clear the display's internal memory

    //Trigger count
    oled.setFontType(0); //Small
    oled.setCursor(0, 0); //x, y
    oled.print("Trig:");

    oled.setFontType(1); //Set font to type 1: 8x16
    oled.setCursor(0, 9); //x, y
    oled.print(triggerCounts);

    //File Size
    oled.setFontType(0); //Small
    oled.setCursor(0, 9 + 15); //x, y
    oled.print("KB:");

    oled.setFontType(1); //Set font to type 1: 8x16
    oled.setCursor(0, 9 + 15 + 9); //x, y
    oled.print(ubxFile.size());

    //SIV
    oled.setFontType(0); //Small
    oled.setCursor(38, 0); //x, y
    oled.print("SIV:");

    oled.setFontType(1); //Set font to type 1: 8x16
    oled.setCursor(38, 9); //x, y
    oled.print(myGNSS.getSIV());

    //RTK
    oled.setFontType(0); //Small
    oled.setCursor(38, 9 + 15); //x, y
    oled.print("RTK:");

    oled.setFontType(0); //Small
    oled.setCursor(38, 9 + 15 + 9); //x, y

    if (myGNSS.getRELPOSNED() == true)
    {
      if (myGNSS.packetUBXNAVRELPOSNED->data.flags.bits.carrSoln == 0)
        oled.print("None");
      else if (myGNSS.packetUBXNAVRELPOSNED->data.flags.bits.carrSoln == 1)
        oled.print("Flo");
      else if (myGNSS.packetUBXNAVRELPOSNED->data.flags.bits.carrSoln == 2)
        oled.print("Fix");
    }

    oled.display();

    ubxFile.close(); // Close the log file after the write. Slower but safer...

    // "debounce" the sound event by 250ms so we only capture one rising edge per navigation solution
    for (int i = 0; i < 250; i++)
    {
      delay(1); // Wait 250 * 1ms
    }

    resetSoundTrigger(); // Reset the sound trigger. This will generate a falling edge TIM_TM2 message
  }

  // Check if we should stop logging:
  if (Serial.available())
  {
    Serial.println();
    Serial.println(F("Logging stopped. Freezing..."));
    while (1); // Do nothing more
  }

  Serial.print("."); // Print dots in rows of 50 while we wait for a sound event
  delay(50);
  if (++dotsPrinted > 50)
  {
    Serial.println();
    dotsPrinted = 0;
  }

  //Ping each device to verify connectivity
  if (isConnected(0x42) == false)
  {
    Serial.println("u-blox offline!");
  }
  if (isConnected(0x3D) == false)
  {
    Serial.println("Display offline!");
  }
  if (isConnected(0x41) == false)
  {
    Serial.println("Mic offline!");
  }

  //We shouldn't be moving so only record location every 5s
  if (millis() - lastFixRecordTime > 5000)
  {
    lastFixRecordTime = millis();
    if (myGNSS.getRELPOSNED() == true)
    {
      if (myGNSS.packetUBXNAVRELPOSNED->data.flags.bits.carrSoln == 0) //No fix
        //if (myGNSS.packetUBXNAVRELPOSNED->data.flags.bits.carrSoln == 2) //RTK
      {
        Serial.println("Logging position");

        File locationFile = SD.open(locationFileName, FILE_APPEND); // Reopen the file
        if (!locationFile)
        {
          Serial.println("Failed to reopen position log");
        }
        else
        {

          // First, let's collect the position data
          int32_t latitude = myGNSS.getHighResLatitude();
          int8_t latitudeHp = myGNSS.getHighResLatitudeHp();
          int32_t longitude = myGNSS.getHighResLongitude();
          int8_t longitudeHp = myGNSS.getHighResLongitudeHp();
          int32_t ellipsoid = myGNSS.getElipsoid();
          int8_t ellipsoidHp = myGNSS.getElipsoidHp();
          int32_t msl = myGNSS.getMeanSeaLevel();
          int8_t mslHp = myGNSS.getMeanSeaLevelHp();
          uint32_t accuracy = myGNSS.getHorizontalAccuracy();

          // Defines storage for the lat and lon as double
          double d_lat; // latitude
          double d_lon; // longitude

          // Assemble u-bloxthe high precision latitude and longitude
          d_lat = ((double)latitude) / 10000000.0; // Convert latitude from degrees * 10^-7 to degrees
          d_lat += ((double)latitudeHp) / 1000000000.0; // Now add the high resolution component (degrees * 10^-9 )
          d_lon = ((double)longitude) / 10000000.0; // Convert longitude from degrees * 10^-7 to degrees
          d_lon += ((double)longitudeHp) / 1000000000.0; // Now add the high resolution component (degrees * 10^-9 )

          // Print the lat and lon
          Serial.print("Lat (deg): ");
          Serial.print(d_lat, 9);
          Serial.print(", Lon (deg): ");
          Serial.print(d_lon, 9);

          locationFile.print(d_lat, 9);
          locationFile.print(",");
          locationFile.print(d_lon, 9);
          locationFile.println();

          locationFile.close();
        }
      }
    }
  }
}

void printTIMTM2data(UBX_TIM_TM2_data_t ubxDataStruct)
{
  // It is the rising edge of the sound event (TRIG) which is important
  // The falling edge is less useful, as it will be "debounced" by the loop code

  if (ubxDataStruct.flags.bits.newRisingEdge) // 1 if a new rising edge was detected
  {
    Serial.println();
    Serial.print(F("Sound Event Detected!"));

    Serial.print(F("  Rising Edge Counter: ")); // Rising edge counter
    Serial.print(ubxDataStruct.count);

    Serial.print(F("  Time Of Week: "));
    Serial.print(ubxDataStruct.towMsR); // Time Of Week of rising edge (ms)
    Serial.print(F(" ms + "));
    Serial.print(ubxDataStruct.towSubMsR); // Millisecond fraction of Time Of Week of rising edge in nanoseconds
    Serial.println(F(" ns"));

    dotsPrinted = 0; // Reset dotsPrinted
  }
}

void resetSoundTrigger()
{
  // Reset the sound event by:
  myTrigger.digitalWrite(VM1010_MODE, LOW); // Get ready to pull the VM1010 MODE pin low
  myTrigger.pinMode(VM1010_MODE, OUTPUT); // Change the PCA9536 GPIO0 pin to an output. It will pull the VM1010 MODE pin low
  myTrigger.pinMode(VM1010_MODE, INPUT); // Change the PCA9536 GPIO0 pin back to an input (with pull-up), so it will not 'fight' the mode button
}

void setMuxport(int channelNumber)
{
  if (channelNumber > 3) return; //Error check

  switch (channelNumber)
  {
    case 0:
      digitalWrite(muxA, LOW);
      digitalWrite(muxB, LOW);
      break;
    case 1:
      digitalWrite(muxA, HIGH);
      digitalWrite(muxB, LOW);
      break;
    case 2:
      digitalWrite(muxA, LOW);
      digitalWrite(muxB, HIGH);
      break;
    case 3:
      digitalWrite(muxA, HIGH);
      digitalWrite(muxB, HIGH);
      break;
  }
}
