/*
   Used as designer for main display.
*/
const int FIRMWARE_VERSION_MAJOR = 1;
const int FIRMWARE_VERSION_MINOR = 0;

#define RTK_IDENTIFIER (FIRMWARE_VERSION_MAJOR * 0x10 + FIRMWARE_VERSION_MINOR)

#include "settings.h"

//Hardware connections
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
const int muxA = 2;
const int muxB = 4;
const int powerSenseAndControl = 13;
const int setupButton = 14;
const int microSD_CS = 25;
const int powerFastOff = 27;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <Wire.h>

//GNSS configuration
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define MAX_PAYLOAD_SIZE 384 // Override MAX_PAYLOAD_SIZE for getModuleInfo which can return up to 348 bytes

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
//SFE_UBLOX_GNSS myGNSS;

// Extend the class for getModuleInfo - See Example21_ModuleInfo
class SFE_UBLOX_GNSS_ADD : public SFE_UBLOX_GNSS
{
  public:
    boolean getModuleInfo(uint16_t maxWait = 1100); //Queries module, texts

    struct minfoStructure // Structure to hold the module info (uses 341 bytes of RAM)
    {
      char swVersion[30];
      char hwVersion[10];
      uint8_t extensionNo = 0;
      char extension[10][30];
    } minfo;
};

SFE_UBLOX_GNSS_ADD myGNSS;

//This string is used to verify the firmware on the ZED-F9P. This
//firmware relies on various features of the ZED and may require the latest
//u-blox firmware to work correctly. We check the module firmware at startup but
//don't prevent operation if firmware is mismatched.
char latestZEDFirmware[] = "FWVER=HPG 1.13";

//Used for config ZED for things not supported in library: getPortSettings, getSerialRate, getNMEASettings, getRTCMSettings
//This array holds the payload data bytes. Global so that we can use between config functions.
uint8_t settingPayload[MAX_PAYLOAD_SIZE];
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Connection settings to NTRIP Caster
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <WiFi.h>
WiFiClient caster;
const char * ntrip_server_name = "SparkFun_RTK_Express";

unsigned long lastServerSent_ms = 0; //Time of last data pushed to caster
unsigned long lastServerReport_ms = 0; //Time of last report of caster bytes sent
int maxTimeBeforeHangup_ms = 10000; //If we fail to get a complete RTCM frame after 10s, then disconnect from caster

uint32_t serverBytesSent = 0; //Just a running total
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Hardware serial and BT buffers
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;
#include "esp_bt.h" //Core access is needed for BT stop. See customBTstop() for more info.

HardwareSerial GPS(2);
#define RXD2 16
#define TXD2 17

#define SERIAL_SIZE_RX 4096 //Reduced from 16384 to make room for WiFi/NTRIP server capabilities
uint8_t rBuffer[SERIAL_SIZE_RX]; //Buffer for reading F9P
uint8_t wBuffer[SERIAL_SIZE_RX]; //Buffer for writing to F9P
TaskHandle_t F9PSerialReadTaskHandle = NULL; //Store handles so that we can kill them if user goes into WiFi NTRIP Server mode
TaskHandle_t F9PSerialWriteTaskHandle = NULL; //Store handles so that we can kill them if user goes into WiFi NTRIP Server mode
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


//Battery fuel gauge and PWM LEDs
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library
SFE_MAX1704X lipo(MAX1704X_MAX17048);

int battLevel = 0; //SOC measured from fuel gauge, in %. Used in multiple places (display, serial debug, log)
float battVoltage = 0.0;
float battChangeRate = 0.0;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//External Display
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SFE_MicroOLED.h> //Click here to get the library: http://librarymanager/All#SparkFun_Micro_OLED
#include "icons.h"

#define PIN_RESET 9
#define DC_JUMPER 1
MicroOLED oled(PIN_RESET, DC_JUMPER);
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
uint8_t unitMACAddress[6]; //Use MAC address in BT broadcast and display
char deviceName[20]; //The serial string that is broadcast. Ex: 'Surveyor Base-BC61'

uint32_t powerPressedStartTime = 0; //Times how long user has been holding power button, used for power down
uint8_t debounceDelay = 20; //ms to delay between button reads

uint32_t lastBattUpdate = 0;
uint32_t lastDisplayUpdate = 0;

uint32_t lastSystemStateUpdate = 0;
//bool setupButtonEvent = false;

uint32_t lastSatelliteDishIconUpdate = 0;
bool satelliteDishIconDisplayed = false; //Toggles as lastSatelliteDishIconUpdate goes above 1000ms
uint32_t lastCrosshairIconUpdate = 0;
bool crosshairIconDisplayed = false; //Toggles as lastCrosshairIconUpdate goes above 1000ms
uint32_t lastBaseIconUpdate = 0;
bool baseIconDisplayed = false; //Toggles as lastSatelliteDishIconUpdate goes above 1000ms
uint32_t lastWifiIconUpdate = 0;
bool wifiIconDisplayed = false; //Toggles as lastWifiIconUpdate goes above 1000ms

uint32_t lastRTCMPacketSent = 0; //Used to count RTCM packets sent during base mode
uint32_t rtcmPacketsSent = 0; //Used to count RTCM packets sent during base mode

uint32_t casterResponseWaitStartTime = 0; //Used to detect if caster service times out

int deleteMeElapsedTime = 0;

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup()
{
  Serial.begin(115200);
  Serial.println("OLED example");

  pinMode(powerSenseAndControl, INPUT_PULLUP);
  pinMode(powerFastOff, INPUT);
  powerOnCheck(); //After serial start in case we want to print

  Wire.begin();
  //Wire.setClock(400000);

  //0x3D is default on Qwiic board
  if (isConnected(0x3D) == true || isConnected(0x3C) == true)
  {
    Serial.println("Display detected");
    online.display = true;
  }
  else
    Serial.println("No display detected");

  if (online.display)
  {
    oled.begin();     // Initialize the OLED
    oled.clear(PAGE); // Clear the display's internal memory
    oled.clear(ALL);  // Clear the library's display buffer
  }

  beginFuelGauge(); //Configure battery fuel guage monitor
  checkBatteryLevels(); //Take an immediate reading so first display will be current

  beginBluetooth(); //Get MAC, start radio

  beginGNSS(); //Connect and configure ZED-F9P

  //radioState = BT_CONNECTED; //Uncomment to display BT icon
  //baseState = BASE_SURVEYING_IN_SLOW;

  //  radioState = WIFI_CONNECTED;
  //  oled.clear(PAGE); // Clear the display's internal buffer
  //  paintBaseTempWiFiStarted();
  //  oled.display(); //Push internal buffer to display
  //
  //  while (1) delay(10);

  settings.enableNtripServer = true;
  //settings.fixedBase = true;
}

void loop()
{
  myGNSS.checkUblox(); //Regularly poll to get latest data and any RTCM

  checkPowerButton(); //See if user wants to power down

  checkSetupButton(); //See if user wants to be rover or base

  checkBatteryLevels();

  updateSystemState();

  updateDisplay();

  delay(10);
}

//Given the current state, see if conditions have moved us to a new state
//A user pressing the setup button (change between rover/base) is handled by checkSetupButton()
void updateSystemState()
{
  if (millis() - lastSystemStateUpdate > 500)
  {
    lastSystemStateUpdate = millis();

    //Move between states as needed
    switch (systemState)
    {
      case (STATE_ROVER_NO_FIX):
        {
          if (myGNSS.getFixType() == 3) //3D
            systemState = STATE_ROVER_FIX;
        }
        break;

      case (STATE_ROVER_FIX):
        {
          byte rtkType = myGNSS.getCarrierSolutionType();
          if (rtkType == 1) //RTK Float
            systemState = STATE_ROVER_RTK_FLOAT;
          else if (rtkType == 2) //RTK Fix
            systemState = STATE_ROVER_RTK_FIX;
        }
        break;

      case (STATE_ROVER_RTK_FLOAT):
        {
          byte rtkType = myGNSS.getCarrierSolutionType();
          if (rtkType == 0) //No RTK
            systemState = STATE_ROVER_FIX;
          if (rtkType == 2) //RTK Fix
            systemState = STATE_ROVER_RTK_FIX;
        }
        break;

      case (STATE_ROVER_RTK_FIX):
        {
          byte rtkType = myGNSS.getCarrierSolutionType();
          if (rtkType == 0) //No RTK
            systemState = STATE_ROVER_FIX;
          if (rtkType == 1) //RTK Float
            systemState = STATE_ROVER_RTK_FLOAT;
        }
        break;

      //Wait for horz acc of 5m or less before starting survey in
      case (STATE_BASE_TEMP_SURVEY_NOT_STARTED):
        {
          //Check for <5m horz accuracy
          uint32_t accuracy = myGNSS.getHorizontalAccuracy(250); //This call defaults to 1100ms and can cause the core to crash with WDT timeout

          float f_accuracy = accuracy;
          f_accuracy = f_accuracy / 10000.0; // Convert the horizontal accuracy (mm * 10^-1) to a float

          Serial.print(F("Waiting for Horz Accuracy < 5 meters: "));
          Serial.print(f_accuracy, 2); // Print the accuracy with 2 decimal places
          Serial.println();

          if (f_accuracy > 0.0 && f_accuracy < 5.0)
          {
            if (surveyIn() == true) //Begin survey
              systemState = STATE_BASE_TEMP_SURVEY_STARTED;
          }
        }
        break;

      //Check survey status until it completes or 15 minutes elapses and we go back to rover
      case (STATE_BASE_TEMP_SURVEY_STARTED):
        {
          if (myGNSS.getFixType() == 5) //We have a TIME fix which is survey in complete
          {
            Serial.println(F("Base survey complete! RTCM now broadcasting."));
            systemState = STATE_BASE_TEMP_TRANSMITTING;
          }
          else
          {
            Serial.print(F("Time elapsed: "));
            Serial.print((String)myGNSS.getSurveyInObservationTime());
            Serial.print(F(" Accuracy: "));
            Serial.print((String)myGNSS.getSurveyInMeanAccuracy());
            Serial.print(F(" SIV: "));
            Serial.print(myGNSS.getSIV());
            Serial.println();

            if (myGNSS.getSurveyInObservationTime() > maxSurveyInWait_s)
            {
              Serial.printf("Survey-In took more than %d minutes. Returning to rover mode.\n", maxSurveyInWait_s / 60);

              resetSurvey();

              displayRoverStart();

              //Configure for rover mode
              Serial.println(F("Rover Mode"));

              //If we are survey'd in, but switch is rover then disable survey
              if (configureUbloxModuleRover() == false)
              {
                Serial.println(F("Rover config failed!"));
                displayRoverFail();
                return;
              }

              beginBluetooth(); //Restart Bluetooth with 'Rover' name

              systemState = STATE_ROVER_NO_FIX;
              displayRoverSuccess();
            }
          }
        }
        break;

      //Leave base temp transmitting if user has enabled WiFi/NTRIP
      case (STATE_BASE_TEMP_TRANSMITTING):
        {
          if (settings.enableNtripServer == true)
          {
            //Turn off Bluetooth and turn on WiFi
            endBluetooth();

            Serial.printf("Connecting to local WiFi: %s\n", settings.wifiSSID);
            WiFi.begin(settings.wifiSSID, settings.wifiPW);

            radioState = WIFI_ON_NOCONNECTION;
            systemState = STATE_BASE_TEMP_WIFI_STARTED;
          }
        }
        break;

      //Check to see if we have connected over WiFi
      case (STATE_BASE_TEMP_WIFI_STARTED):
        {
          byte wifiStatus = WiFi.status();
          if (wifiStatus == WL_CONNECTED)
          {
            radioState = WIFI_CONNECTED;

            systemState = STATE_BASE_TEMP_WIFI_CONNECTED;
          }
          else
          {
            Serial.print(F("WiFi Status: "));
            switch (wifiStatus) {
              case WL_NO_SHIELD: Serial.println(F("WL_NO_SHIELD"));
              case WL_IDLE_STATUS: Serial.println(F("WL_IDLE_STATUS"));
              case WL_NO_SSID_AVAIL: Serial.println(F("WL_NO_SSID_AVAIL"));
              case WL_SCAN_COMPLETED: Serial.println(F("WL_SCAN_COMPLETED"));
              case WL_CONNECTED: Serial.println(F("WL_CONNECTED"));
              case WL_CONNECT_FAILED: Serial.println(F("WL_CONNECT_FAILED"));
              case WL_CONNECTION_LOST: Serial.println(F("WL_CONNECTION_LOST"));
              case WL_DISCONNECTED: Serial.println(F("WL_DISCONNECTED"));
            }
          }
        }
        break;

      case (STATE_BASE_TEMP_WIFI_CONNECTED):
        {
          if (settings.enableNtripServer == true)
          {
            //Open connection to caster service
            if (caster.connect(settings.casterHost, settings.casterPort) == true) //Attempt connection
            {
              systemState = STATE_BASE_TEMP_CASTER_STARTED;

              Serial.printf("Connected to %s:%d\n", settings.casterHost, settings.casterPort);

              const int SERVER_BUFFER_SIZE  = 512;
              char serverBuffer[SERVER_BUFFER_SIZE];

              snprintf(serverBuffer, SERVER_BUFFER_SIZE, "SOURCE %s /%s\r\nSource-Agent: NTRIP %s/%s\r\n\r\n",
                       settings.mountPointPW, settings.mountPoint, ntrip_server_name, "App Version 1.0");

              Serial.printf("Sending credentials:\n%s\n", serverBuffer);
              caster.write(serverBuffer, strlen(serverBuffer));

              casterResponseWaitStartTime = millis();
            }
          }
        }
        break;

      //Wait for response for caster service and make sure it's valid
      case (STATE_BASE_TEMP_CASTER_STARTED):
        {
          //Check if caster service responded
          if (caster.available() == 0)
          {
            if (millis() - casterResponseWaitStartTime > 5000)
            {
              Serial.println(F("Caster failed to respond. Do you have your caster address and port correct?"));
              caster.stop();
              delay(10); //Yield to RTOS

              systemState = STATE_BASE_TEMP_WIFI_CONNECTED; //Return to previous state
            }
          }
          else
          {
            //Check reply
            bool connectionSuccess = false;
            char response[512];
            int responseSpot = 0;
            while (caster.available())
            {
              response[responseSpot++] = caster.read();
              if (strstr(response, "200") > 0) //Look for 'ICY 200 OK'
                connectionSuccess = true;
              if (responseSpot == 512 - 1) break;
            }
            response[responseSpot] = '\0';
            Serial.printf("Caster responded with: %s\n", response);

            if (connectionSuccess == false)
            {
              Serial.printf("Caster responded with bad news: %s. Are you sure your caster credentials are correct?", response);
              systemState = STATE_BASE_TEMP_WIFI_CONNECTED; //Return to previous state
            }
            else
            {
              //We're connected!
              Serial.println(F("Connected to caster"));

              //Reset flags
              lastServerReport_ms = millis();
              lastServerSent_ms = millis();
              serverBytesSent = 0;

              systemState = STATE_BASE_TEMP_CASTER_CONNECTED;
            }
          }
        }
        break;

      //Monitor connected state
      case (STATE_BASE_TEMP_CASTER_CONNECTED):
        {
          if (caster.connected() == false)
          {
              systemState = STATE_BASE_TEMP_WIFI_CONNECTED; //Return to 2 earlier states to try to reconnect
          }
        }
        break;

      //Leave base fixed transmitting if user has enabled WiFi/NTRIP
      case (STATE_BASE_FIXED_TRANSMITTING):
        {
          if (settings.enableNtripServer == true)
          {
            //Turn off Bluetooth and turn on WiFi
            endBluetooth();

            Serial.printf("Connecting to local WiFi: %s", settings.wifiSSID);
            WiFi.begin(settings.wifiSSID, settings.wifiPW);

            radioState = WIFI_ON_NOCONNECTION;
            systemState = STATE_BASE_FIXED_WIFI_STARTED;
          }
        }
        break;

      //Check to see if we have connected over WiFi
      case (STATE_BASE_FIXED_WIFI_STARTED):
        {
          byte wifiStatus = WiFi.status();
          if (wifiStatus == WL_CONNECTED)
          {
            radioState = WIFI_CONNECTED;

            systemState = STATE_BASE_FIXED_WIFI_CONNECTED;
          }
          else
          {
            Serial.print(F("WiFi Status: "));
            switch (wifiStatus) {
              case WL_NO_SHIELD: Serial.println(F("WL_NO_SHIELD")); break;
              case WL_IDLE_STATUS: Serial.println(F("WL_IDLE_STATUS")); break;
              case WL_NO_SSID_AVAIL: Serial.println(F("WL_NO_SSID_AVAIL")); break;
              case WL_SCAN_COMPLETED: Serial.println(F("WL_SCAN_COMPLETED")); break;
              case WL_CONNECTED: Serial.println(F("WL_CONNECTED")); break;
              case WL_CONNECT_FAILED: Serial.println(F("WL_CONNECT_FAILED")); break;
              case WL_CONNECTION_LOST: Serial.println(F("WL_CONNECTION_LOST")); break;
              case WL_DISCONNECTED: Serial.println(F("WL_DISCONNECTED")); break;
            }
          }
        }
        break;

      case (STATE_BASE_FIXED_WIFI_CONNECTED):
        {
          if (settings.enableNtripServer == true)
          {
            //Open connection to caster service
            if (caster.connect(settings.casterHost, settings.casterPort) == true) //Attempt connection
            {
              systemState = STATE_BASE_FIXED_CASTER_STARTED;

              Serial.printf("Connected to %s:%d\n", settings.casterHost, settings.casterPort);

              const int SERVER_BUFFER_SIZE  = 512;
              char serverBuffer[SERVER_BUFFER_SIZE];

              snprintf(serverBuffer, SERVER_BUFFER_SIZE, "SOURCE %s /%s\r\nSource-Agent: NTRIP %s/%s\r\n\r\n",
                       settings.mountPointPW, settings.mountPoint, ntrip_server_name, "App Version 1.0");

              Serial.printf("Sending credentials:\n%s\n", serverBuffer);
              caster.write(serverBuffer, strlen(serverBuffer));

              casterResponseWaitStartTime = millis();
            }
          }
        }
        break;

      //Wait for response for caster service and make sure it's valid
      case (STATE_BASE_FIXED_CASTER_STARTED):
        {
          //Check if caster service responded
          if (caster.available() < 10)
          {
            if (millis() - casterResponseWaitStartTime > 5000)
            {
              Serial.println(F("Caster failed to respond. Do you have your caster address and port correct?"));
              caster.stop();
              delay(10); //Yield to RTOS

              systemState = STATE_BASE_FIXED_WIFI_CONNECTED; //Return to previous state
            }
          }
          else
          {
            //Check reply
            bool connectionSuccess = false;
            char response[512];
            int responseSpot = 0;
            while (caster.available())
            {
              response[responseSpot++] = caster.read();
              if (strstr(response, "200") > 0) //Look for 'ICY 200 OK'
                connectionSuccess = true;
              if (responseSpot == 512 - 1) break;
            }
            response[responseSpot] = '\0';
            Serial.printf("Caster responded with: %s\n", response);

            if (connectionSuccess == false)
            {
              Serial.printf("Caster responded with bad news: %s. Are you sure your caster credentials are correct?", response);
              systemState = STATE_BASE_FIXED_WIFI_CONNECTED; //Return to previous state
            }
            else
            {
              //We're connected!
              Serial.println(F("Connected to caster"));

              //Reset flags
              lastServerReport_ms = millis();
              lastServerSent_ms = millis();
              serverBytesSent = 0;

              systemState = STATE_BASE_FIXED_CASTER_CONNECTED;
            }
          }
        }
        break;

      //Monitor connected state
      case (STATE_BASE_FIXED_CASTER_CONNECTED):
        {
          if (caster.connected() == false)
          {
              systemState = STATE_BASE_FIXED_WIFI_CONNECTED; //Return to 2 earlier states to try to reconnect
          }
        }
        break;

    }

    //Debug print
    switch (systemState)
    {
      case (STATE_ROVER_NO_FIX):
        Serial.println(F("State: Rover - No Fix"));
        break;
      case (STATE_ROVER_FIX):
        Serial.println(F("State: Rover - Fix"));
        break;
      case (STATE_ROVER_RTK_FLOAT):
        Serial.println(F("State: Rover - RTK Float"));
        break;
      case (STATE_ROVER_RTK_FIX):
        Serial.println(F("State: Rover - RTK Fix"));
        break;
      case (STATE_BASE_TEMP_SURVEY_NOT_STARTED):
        Serial.println(F("State: Base-Temp - Survey Not Started"));
        break;
      case (STATE_BASE_TEMP_SURVEY_STARTED):
        Serial.println(F("State: Base-Temp - Survey Started"));
        break;
      case (STATE_BASE_TEMP_TRANSMITTING):
        Serial.println(F("State: Base-Temp - Transmitting"));
        break;
      case (STATE_BASE_TEMP_WIFI_STARTED):
        Serial.println(F("State: Base-Temp - WiFi Started"));
        break;
      case (STATE_BASE_TEMP_WIFI_CONNECTED):
        Serial.println(F("State: Base-Temp - WiFi Connected"));
        break;
      case (STATE_BASE_TEMP_CASTER_STARTED):
        Serial.println(F("State: Base-Temp - Caster Started"));
        break;
      case (STATE_BASE_TEMP_CASTER_CONNECTED):
        Serial.println(F("State: Base-Temp - Caster Connected"));
        break;
      case (STATE_BASE_FIXED_TRANSMITTING):
        Serial.println(F("State: Base-Fixed - Transmitting"));
        break;
      case (STATE_BASE_FIXED_WIFI_STARTED):
        Serial.println(F("State: Base-Fixed - WiFi Started"));
        break;
      case (STATE_BASE_FIXED_WIFI_CONNECTED):
        Serial.println(F("State: Base-Fixed - WiFi Connected"));
        break;
      case (STATE_BASE_FIXED_CASTER_STARTED):
        Serial.println(F("State: Base-Fixed - Caster Started"));
        break;
      case (STATE_BASE_FIXED_CASTER_CONNECTED):
        Serial.println(F("State: Base-Fixed - Caster Connected"));
        break;
      default:
        Serial.printf("State Unknown: %d\n", systemState);
        break;
    }
  }
}
