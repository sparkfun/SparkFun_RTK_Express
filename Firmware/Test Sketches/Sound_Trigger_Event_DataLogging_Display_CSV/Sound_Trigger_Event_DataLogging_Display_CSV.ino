/*
  This example records the time and location of where the VM1010 mic detects a sound
  above background noise to a CSV file. Combining three or more of these files can create
  the location of the noise in 2D.

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

const char eventsFilePrefix[40] = "SFE_SoundEventLog"; //Prefix of log fix with date and time suffix
char eventsFileName[100] = "";
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
long lastFixRecordTime = 0; //Record our location every 5 seconds
bool newEventToRecord = false; //Goes true when INT pin goes high

uint32_t triggerCount = 0; //Global copy - TM2 event counter
uint32_t towMsR = 0; //Global copy - Time Of Week of rising edge (ms)
uint32_t towSubMsR = 0; //Global copy - Millisecond fraction of Time Of Week of rising edge in nanoseconds

uint32_t lastDisplayUpdate = 0;
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

  myGNSS.setAutoTIMTM2callback(&eventMessageReceived); // Enable automatic TIM TM2 messages with callback to eventMessageReceived

  myGNSS.logTIMTM2(); // Enable TIM TM2 data logging

  myGNSS.setAutoPVT(true, false); //Tell the GPS to "send" each solution, but do not update stale data when accessed

  while (Serial.available()) // Make sure the Serial buffer is empty
    Serial.read();

  getEventFileName();

  resetSoundTrigger(); // Reset the sound trigger - in case it has already been triggered
}

void loop()
{
  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  updateDisplay();

  if (newEventToRecord == true)
  {
    recordTriggerEvent();

    dotsPrinted = 0; // Reset dotsPrinted

    delay(250); // "debounce" the sound event by 250ms so we only capture one rising edge per navigation solution
    myGNSS.checkUblox(); // Check for the arrival of new data and process it.
    myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

    resetSoundTrigger(); // Reset the sound trigger. This will generate a falling edge TIM_TM2 message
    newEventToRecord = false;
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
  //  if (isConnected(0x42) == false)
  //  {
  //    Serial.println("u-blox offline!");
  //  }
  //  if (isConnected(0x3D) == false)
  //  {
  //    Serial.println("Display offline!");
  //  }
  //  if (isConnected(0x41) == false)
  //  {
  //    Serial.println("Mic offline!");
  //  }

}

void eventMessageReceived(UBX_TIM_TM2_data_t ubxDataStruct)
{
  // It is the rising edge of the sound event (TRIG) which is important
  // The falling edge is less useful, as it will be "debounced" by the loop code
  if (ubxDataStruct.flags.bits.newRisingEdge) // 1 if a new rising edge was detected
  {
    Serial.println("Rising Edge Event");

    triggerCount = ubxDataStruct.count;
    towMsR = ubxDataStruct.towMsR; // Time Of Week of rising edge (ms)
    towSubMsR = ubxDataStruct.towSubMsR; // Millisecond fraction of Time Of Week of rising edge in nanoseconds

    newEventToRecord = true;
  }
}

void resetSoundTrigger()
{
  // Reset the sound event by:
  myTrigger.digitalWrite(VM1010_MODE, LOW); // Get ready to pull the VM1010 MODE pin low
  myTrigger.pinMode(VM1010_MODE, OUTPUT); // Change the PCA9536 GPIO0 pin to an output. It will pull the VM1010 MODE pin low
  myTrigger.pinMode(VM1010_MODE, INPUT); // Change the PCA9536 GPIO0 pin back to an input (with pull-up), so it will not 'fight' the mode button
}

//Record an event to the events file
void recordTriggerEvent()
{
  File eventsFile = SD.open(eventsFileName, FILE_APPEND); // Reopen the file
  if (!eventsFile)
  {
    Serial.println("Failed to open log");
  }

  //Record time and location to event file

  Serial.println();
  Serial.print(F("Sound Event:"));

  Serial.print(triggerCount);
  Serial.print(",");
  eventsFile.print(triggerCount);
  eventsFile.print(",");

  Serial.print(F(" Time Of Week: "));
  Serial.print(towMsR); // Time Of Week of rising edge (ms)
  Serial.print(",");
  eventsFile.print(towMsR);
  eventsFile.print(",");

  Serial.print(F(" ms + "));
  Serial.print(towSubMsR); // Millisecond fraction of Time Of Week of rising edge in nanoseconds
  Serial.print(F(" ns"));
  Serial.print(",");
  eventsFile.print(towSubMsR);
  eventsFile.print(",");

  //Record location of event
  if (myGNSS.getRELPOSNED() == true)
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
    int carrierSolution = myGNSS.packetUBXNAVRELPOSNED->data.flags.bits.carrSoln;

    // Defines storage for the lat and lon as double
    double d_lat; // latitude
    double d_lon; // longitude

    // Assemble u-bloxthe high precision latitude and longitude
    d_lat = ((double)latitude) / 10000000.0; // Convert latitude from degrees * 10^-7 to degrees
    d_lat += ((double)latitudeHp) / 1000000000.0; // Now add the high resolution component (degrees * 10^-9 )
    d_lon = ((double)longitude) / 10000000.0; // Convert longitude from degrees * 10^-7 to degrees
    d_lon += ((double)longitudeHp) / 1000000000.0; // Now add the high resolution component (degrees * 10^-9 )

    // Print the lat and lon
    Serial.print(d_lat, 9);
    Serial.print(",");
    Serial.print(d_lon, 9);
    Serial.print(",");

    eventsFile.print(d_lat, 9);
    eventsFile.print(",");
    eventsFile.print(d_lon, 9);
    eventsFile.print(",");
  }

  Serial.println();

  eventsFile.println();
  eventsFile.close(); //Will record any leftover data
}

//Determine a file name to store all the sound events and locations
//File name includes date/time at power on
void getEventFileName()
{
  //Wait for time and date to become valid
  bool haveLogTime = false;
  while (haveLogTime == false)
  {
    myGNSS.checkUblox();
    Serial.println("Waiting for GNSS time/date to create event file");

    byte fixType = myGNSS.getFixType();
    Serial.print(F(" Fix: "));
    if(fixType == 0) Serial.print(F("No fix"));
    else if(fixType == 1) Serial.print(F("Dead reckoning"));
    else if(fixType == 2) Serial.print(F("2D"));
    else if(fixType == 3) Serial.print(F("3D"));
    else if(fixType == 4) Serial.print(F("GNSS + Dead reckoning"));
    else if(fixType == 5) Serial.print(F("Time only"));
    Serial.println();

    if (fixType == 3 || fixType == 5)
    {
      if (myGNSS.getTimeValid() == true && myGNSS.getDateValid() == true) //Will pass if ZED's RTC is reporting (regardless of GNSS fix)
        haveLogTime = true;
      if (myGNSS.getConfirmedTime() == true && myGNSS.getConfirmedDate() == true) //Requires GNSS fix
        haveLogTime = true;
    }

    delay(1000);
  }

  //Based on GPS data/time, create a log file in the format prefix_YYMMDD_HHMMSS.txt
  sprintf(eventsFileName, "/%s_%02d%02d%02d_%02d%02d%02d.txt",
          eventsFilePrefix,
          myGNSS.getYear() - 2000, myGNSS.getMonth(), myGNSS.getDay(),
          myGNSS.getHour(), myGNSS.getMinute(), myGNSS.getSecond()
         );

  Serial.printf("Event file name: %s\n", eventsFileName);
}
