/*
  From BMA220 5.6
  https://media.digikey.com/pdf/Data%20Sheets/Bosch/BMA220.pdf

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
  Serial.begin(115200);
  Serial.println("SparkFun Accel Example");

  pinMode(powerSenseAndControl, INPUT_PULLUP);
  pinMode(powerFastOff, INPUT);
  //powerOnCheck(); //After serial start in case we want to print

  Wire.begin();
  Wire.setClock(400000);

  beginDisplay(); //Check if an external Qwiic OLED is attached

  if (accel.begin() == false)
  {
    Serial.println("Accelerometer not detected. Check address jumper and wiring. Freezing...");
    while (1)
      ;
  }

  //The larger the avgAmount the faster we should read the sensor
  //accel.setDataRate(LIS2DH12_ODR_100Hz); //6 measurements a second
  accel.setDataRate(LIS2DH12_ODR_400Hz); //25 measurements a second

  //Set INT POLARITY to Active Low to mimic power btn pressing
  //CTRL_REG6, INT_POLARITY = 1 active low
  accel.setIntPolarity(LOW);

  //Set INT1 interrupt
  //CTRL_REG3, I1_IA1 = 1 - Enable IA1 Interrupt on INT1 pin
  accel.enableInt1IA1();
  
  //Set INT1 threshold
  //INT1_THS = 500mg / 16mb = 31
  //accel.setInt1Threshold(31);
  accel.setInt1Threshold(10);

  Serial.print("threshold: ");
  Serial.println(accel.getInt1Threshold());

  //Set INT1 Duration
  //INT1_DURATION = 500
  //accel.setInt1Duration(30);
  accel.setInt1Duration(9);

  Serial.print("duration: ");
  Serial.println(accel.getInt1Duration());

  //Clear the interrupt
}

double averagedRoll = 0.0; 
double averagedPitch = 0.0;

void loop()
{
  if(accel.getInt1()) //Reading int will clear it
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
