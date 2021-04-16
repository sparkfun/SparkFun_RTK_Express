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
const char logFileName[30] = "/TIM_TM2.ubx"; //Name of the file to be created
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

  Wire.begin(); // Start I2C communication with the GNSS and the Qwiic Sound Trigger

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
  File myFile = SD.open(logFileName, FILE_WRITE);
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
    uint8_t myBuffer[packetLength]; // Create our own buffer to hold the data while we write it to SD card

    myGNSS.extractFileBufferData((uint8_t *)&myBuffer, packetLength); // Extract exactly packetLength bytes from the UBX file buffer and put them into myBuffer

    File myFile = SD.open(logFileName, FILE_APPEND); // Reopen the file
    if(!myFile)
    {
      Serial.println("Failed to reopen log");
    }

    myFile.write(myBuffer, packetLength); // Write exactly packetLength bytes from myBuffer to the ubxDataFile on the SD card

    myFile.flush();

    if (millis() - lastUbxCountUpdate > 1000)
    {
      lastUbxCountUpdate = millis();
      Serial.printf("UBX file size and kB/s: %d / %0.02f\n", myFile.size(), (float)myFile.size() / millis());
    }

    myFile.close(); // Close the log file after the write. Slower but safer...

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
