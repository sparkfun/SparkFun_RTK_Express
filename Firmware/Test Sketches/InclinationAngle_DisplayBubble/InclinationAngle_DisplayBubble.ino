/*
  From BMA220 5.6
  https://media.digikey.com/pdf/Data%20Sheets/Bosch/BMA220.pdf

*/

#include <Wire.h>

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

void setup()
{
  Serial.begin(115200);
  Serial.println("SparkFun Accel Example");

  Wire.begin();
  //Wire.setClock(400000);

  beginDisplay(); //Check if an external Qwiic OLED is attached

  while(1);

  if (accel.begin() == false)
  {
    Serial.println("Accelerometer not detected. Check address jumper and wiring. Freezing...");
    while (1)
      ;
  }

  //The larger the avgAmount the faster we should read the sensor
  //accel.setDataRate(LIS2DH12_ODR_100Hz); //6 measurements a second
  accel.setDataRate(LIS2DH12_ODR_400Hz); //25 measurements a second
}

double averagedRoll = 0.0;
double averagedPitch = 0.0;

void loop()
{
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
    float accelZ = accel.getY();
    float accelY = accel.getZ();
    accelZ *= -1.0;
    accelX *= -1.0;

 // Y -Z X = good but left/right axis wrong
 // X -Z Y = close, one axis still bad
 // -X -Z Y = 
 
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

  Serial.print(averagedRoll, 0);
  Serial.print(", ");
  Serial.print(averagedPitch, 0);
  Serial.print(", ");

  //  Serial.print(accelX, 1);
  //  Serial.print(", ");
  //  Serial.print(accelY, 1);
  //  Serial.print(", ");
  //  Serial.print(accelZ, 1);
  //  Serial.print(", ");

  Serial.println();
}
