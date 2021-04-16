/*
  From BMA220 5.6
  https://media.digikey.com/pdf/Data%20Sheets/Bosch/BMA220.pdf

 
  My best guess was the accel was pulling its interrupt pin low causing current flow through ESP32's ACCEL_INT pin. 
  Because the accel was always on via battery, the device would be configured in various states between firmware uploads
  and cause havoc with test firmware. Pulling bat caused accel to reset to POR state (no interrupts).
  Fully disabling interrupts on accel as powerDown() seems to have fixed the issue.

  188uA in sleep
  200mA Running
  75mA

  There is ~100mA surge when pressing the power off button when accel is wired into power down circuit
  Disable ints during run. Enable before sleep?

  *     

*/

#include <Wire.h>

//Hardware connections
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
const int accelInt = 4;
const int statLED = 12;
const int powerSenseAndControl = 13;
const int peripheralPower = 14;
const int powerFastOff = 16;
const int microSD_CS = 25;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "SparkFun_LIS2DH12.h" //Click here to get the library: http://librarymanager/All#SparkFun_LIS2DH12
SPARKFUN_LIS2DH12 accel;

//External Display
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <SFE_MicroOLED.h> //Click here to get the library: http://librarymanager/All#SparkFun_Micro_OLED
//#include "icons.h"

#define PIN_RESET 9
#define DC_JUMPER 1
MicroOLED oled(PIN_RESET, DC_JUMPER);
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


uint32_t powerPressedStartTime = 0; //Times how long user has been holding power button, used for power down
uint8_t debounceDelay = 20; //ms to delay between button reads
bool setupByPowerButton = false; //We can change setup via tapping power button

void setup()
{
  pinMode(statLED, OUTPUT);
  digitalWrite(statLED, HIGH);

//  pinMode(peripheralPower, OUTPUT);
//  digitalWrite(peripheralPower, LOW);

  pinMode(powerSenseAndControl, INPUT_PULLUP);

  Wire.begin(); //Start wire so that we can config accel before power off
  //Wire.setClock(400000);

  //powerOnCheck(); //After serial start in case we want to print

  Serial.begin(115200);
  Serial.println("SparkFun Accel Example");

  pinMode(peripheralPower, OUTPUT);
  digitalWrite(peripheralPower, HIGH);

  beginDisplay(); //Check if an external Qwiic OLED is attached

  delay(1000); //Give time for display after a previous power down event

  if (accel.begin() == false)
  {
    Serial.println("Accelerometer not detected. Check address jumper and wiring. Freezing...");
    while (1)
      ;
  }

  //Setup accel to pull pin low when we move, then power down. 

  pinMode(accelInt, INPUT_PULLUP);

  accel.setDataRate(LIS2DH12_POWER_DOWN); //Stop measurements

  //INT1_CFG - enable X and Y events
  //accel.setInt1(true);
  accel.setInt1(false);

  //Set INT POLARITY to Active Low to mimic power btn pressing
  //CTRL_REG6, INT_POLARITY = 1 active low
  
  accel.setIntPolarity(LOW); //Line causes bounce

  //Set INT1 interrupt
  //CTRL_REG3, I1_IA1 = 1 - Enable IA1 Interrupt on INT1 pin
  accel.setInt1IA1(true);

  //Set INT1 threshold
  //INT1_THS = 500mg / 16mb = 31
  //accel.setInt1Threshold(62); //90 degree tilt before interrupt
  accel.setInt1Threshold(31); //45 degree tilt before interrupt
  //accel.setInt1Threshold(10);

  //Set INT1 Duration
  //INT1_DURATION = 500
  //accel.setInt1Duration(30);
  accel.setInt1Duration(9);

  //Latch interrupt 1, CTRL_REG5, LIR_INT1
  //accel.setInt1Latch(true);
  accel.setInt1Latch(false); //Atempt to remove 90mA surge

  //Clear the interrupt
  while(accel.getInt1()) delay(10); //Reading int will clear it

  //Go into 8bit mode
  //accel.setDataRate(LIS2DH12_ODR_1Hz); //Very slow for wake up

  //powerDown(true);

  //The larger the avgAmount the faster we should read the sensor
  //accel.setDataRate(LIS2DH12_ODR_100Hz); //6 measurements a second
  accel.setDataRate(LIS2DH12_ODR_400Hz); //25 measurements a second

  Serial.println("Begin bubble");
}

double averagedRoll = 0.0;
double averagedPitch = 0.0;

void loop()
{
  if (digitalRead(accelInt) == LOW)
  {
    Serial.println("Accel int pin low!");
  }
  
  if (accel.getInt1()) //Reading int will clear it
  {
    Serial.println("Int!");
  }
  
  checkPowerButton(); //See if user wants to power down

  getAngles();

  //  Serial.print(averagedRoll, 0);
  //  Serial.print(", ");
  //  Serial.print(averagedPitch, 0);
  //  Serial.print(", ");
  //
  //  Serial.println();

  oled.clear(PAGE); // Clear the display's internal memory

  //Draw dot in middle
  oled.pixel(LCDWIDTH / 2, LCDHEIGHT / 2);
  oled.pixel(LCDWIDTH / 2 + 1, LCDHEIGHT / 2);
  oled.pixel(LCDWIDTH / 2, LCDHEIGHT / 2 + 1);
  oled.pixel(LCDWIDTH / 2 + 1, LCDHEIGHT / 2 + 1);

  //Draw circle relative to dot
  int radiusLarge = 10;
  int radiusSmall = 4;

  oled.circle(LCDWIDTH / 2 - averagedPitch, LCDHEIGHT / 2 + averagedRoll, radiusLarge);
  oled.circle(LCDWIDTH / 2 - averagedPitch, LCDHEIGHT / 2 + averagedRoll, radiusSmall);

  oled.display();

  //delay(100);

  //Serial.println(".");
}

void getAngles()
{
  averagedRoll = 0.0;
  averagedPitch = 0.0;
  const int avgAmount = 16;

  //Take an average readings
  for (int reading = 0 ; reading < avgAmount ; reading++)
  {
    while (accel.available() == false) delay(1);

    float accelX = accel.getX();
    float accelY = accel.getY();
    float accelZ = accel.getZ();
    accelZ *= -1.0; //Sensor is mounted to back of PCB

    double roll = atan2(accelY , accelZ) * 57.3;
    double pitch = atan2((-accelX) , sqrt(accelY * accelY + accelZ * accelZ)) * 57.3;

    averagedRoll += roll;
    averagedPitch += pitch;
  }

  averagedRoll /= (float)avgAmount;
  averagedPitch /= (float)avgAmount;

  //Avoid -0 since we're not printing the decimal portion
  if (averagedRoll < 0.5 && averagedRoll > -0.5) averagedRoll = 0;
  if (averagedPitch < 0.5 && averagedPitch > -0.5) averagedPitch = 0;

  //  Serial.print(averagedRoll, 0);
  //  Serial.print(", ");
  //  Serial.print(averagedPitch, 0);
  //  Serial.print(", ");

  //    Serial.print(accelX, 1);
  //    Serial.print(", ");
  //    Serial.print(accelY, 1);
  //    Serial.print(", ");
  //    Serial.print(accelZ, 1);
  //    Serial.print(", ");
}
