/*
  From BMA220 5.6

*/

#include <Wire.h>

#include "SparkFun_LIS2DH12.h" //Click here to get the library: http://librarymanager/All#SparkFun_LIS2DH12
SPARKFUN_LIS2DH12 accel;

void setup()
{
  Serial.begin(115200);
  Serial.println("SparkFun Accel Example");

  Wire.begin();

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

void loop()
{
  double averagedRoll = 0.0;
  double averagedPitch = 0.0;
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

  Serial.print(averagedRoll, 0);
  Serial.print(", ");
  Serial.print(averagedPitch, 0);
  Serial.print(", ");

  //    Serial.print(accelX, 1);
  //    Serial.print(", ");
  //    Serial.print(accelY, 1);
  //    Serial.print(", ");
  //    Serial.print(accelZ, 1);
  //    Serial.print(", ");


  Serial.println();


  //delay(100);
}
