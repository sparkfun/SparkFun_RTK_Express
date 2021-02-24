//The display can enter a variety of screens
enum DisplayScreen
{
  SCREEN_ROVER = 0,
  SCREEN_ROVER_RTCM, //RTCM packets received (RTK float/fix)
  SCREEN_BASE_SURVEYING_NOTSTARTED, //Wait for min accuracy before starting SVIN
  SCREEN_BASE_SURVEYING_STARTED, //Display mean and elapsed time
  SCREEN_BASE_TRANSMITTING, //Display RTCM packets transmitted
  SCREEN_BASE_FAILED, //Display error
  SCREEN_BASE_FIXED_TRANSMITTING, //Fixed coordinates, Display RTCM packets transmitted
};
volatile byte currentScreen = SCREEN_ROVER;

//These are the devices on board RTK Surveyor that may be on or offline.
struct struct_online {
  bool microSD = false;
  bool display = false;
  bool dataLogging = false;
  bool serialOutput = false;
  bool eeprom = false;
} online;

//Radio status LED goes from off (LED off), no connection (blinking), to connected (solid)
enum RadioState
{
  RADIO_OFF = 0,
  BT_ON_NOCONNECTION, //WiFi is off
  BT_CONNECTED,
  WIFI_ON_NOCONNECTION, //BT is off
  WIFI_CONNECTED,
};
volatile byte radioState = RADIO_OFF;

//Base status goes from Rover-Mode (LED off), surveying in (blinking), to survey is complete/trasmitting RTCM (solid)
typedef enum
{
  BASE_OFF = 0,
  BASE_SURVEYING_IN_NOTSTARTED, //User has indicated base, but current pos accuracy is too low
  BASE_SURVEYING_IN_SLOW,
  BASE_SURVEYING_IN_FAST,
  BASE_TRANSMITTING,
} BaseState;
volatile BaseState baseState = BASE_OFF;
unsigned long baseStateBlinkTime = 0;
const unsigned long maxSurveyInWait_s = 60L * 15L; //Re-start survey-in after X seconds
