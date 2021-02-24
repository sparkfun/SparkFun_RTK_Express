//System can enter a variety of states starting at Rover_No_Fix at power on
typedef enum
{
  STATE_ROVER_NO_FIX = 0,
  STATE_ROVER_FIX,
  STATE_ROVER_RTK_FLOAT,
  STATE_ROVER_RTK_FIX,
  STATE_BASE_TEMP_SURVEY_NOT_STARTED, //User has indicated base, but current pos accuracy is too low
  STATE_BASE_TEMP_SURVEY_STARTED,
  STATE_BASE_TEMP_TRANSMITTING,
  STATE_BASE_TEMP_WIFI_STARTED,
  STATE_BASE_TEMP_WIFI_CONNECTED,
  STATE_BASE_FIXED_TRANSMITTING,
  STATE_BASE_FIXED_WIFI_STARTED,
  STATE_BASE_FIXED_WIFI_CONNECTED,
} SystemState;
volatile SystemState systemState = STATE_ROVER_NO_FIX;


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

const unsigned long maxSurveyInWait_s = 60L * 15L; //Re-start survey-in after X seconds
