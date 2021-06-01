/*
  MicroOLED_Cube.ino
  Rotating a 3-D Cube on the MicroOLED Breakout
  Jim Lindblom @ SparkFun Electronics
  Original Creation Date: October 27, 2014

  This sketch uses the MicroOLED library to draw a 3-D projected
  cube, and rotate it along all three axes.

  Hardware Connections:
    This example assumes you are using Qwiic. See the SPI examples for
    a detailed breakdown of connection info.

  This code is beerware; if you see me (or any other SparkFun employee) at the
  local, and you've found our code helpful, please buy us a round!

  Distributed as-is; no warranty is given.
*/

#include <Wire.h>
#include <SFE_MicroOLED.h> //Click here to get the library: http://librarymanager/All#SparkFun_Micro_OLED

#define DC_JUMPER 1
#define PIN_RESET 9 // Optional - Connect RST on display to pin 9 on Arduino

MicroOLED oled(PIN_RESET, DC_JUMPER); //Example I2C declaration

int SCREEN_WIDTH = oled.getLCDWidth();
int SCREEN_HEIGHT = oled.getLCDHeight();

float d = 3;
float px[] = {
  -d, d, d, -d, -d, d, d, -d
};
float py[] = {
  -d, -d, d, d, -d, -d, d, d
};
float pz[] = {
  -d, -d, -d, -d, d, d, d, d
};

float p2x[] = {
  0, 0, 0, 0, 0, 0, 0, 0
};
float p2y[] = {
  0, 0, 0, 0, 0, 0, 0, 0
};

float r[] = {
  0, 0, 0
};

#define SHAPE_SIZE 600
// Define how fast the cube rotates. Smaller numbers are faster.
// This is the number of ms between draws.
#define ROTATION_SPEED 0

// I2C is great, but will result in a much slower update rate. The
// slower framerate may be a worthwhile tradeoff, if you need more
// pins, though.
void setup()
{
  Serial.begin(115200);
  delay(250);
  Serial.println("I2C test");

  delay(100);   //Give display time to power on
  Wire.begin(); //Setup I2C bus
  Wire.setClock(400000);

  oled.begin();
  oled.clear(ALL);
  oled.display();

  //oled.enableDebugging();
  oled.setI2CTransactionSize(64);

  uint16_t transSize = oled.getI2CTransactionSize();
  Serial.printf("transSize: %d\n", transSize);
}

void loop()
{
  if (Serial.available())
  {
    byte incoming = Serial.read();
    if (incoming == 'a')
    {
      drawCube();
    }
    else if (incoming == 'b')
    {
      while (1)
      {
        drawCube();
      }
    }
    else if (incoming == 'c')
    {
      int dataSize = 128*4;
      char dataBytes[dataSize];
      uint8_t val = 0;
      for (int x = 0 ; x < dataSize ; x++)
      {
        dataBytes[x] = val;
        val++;
      }

      Wire.beginTransmission(0b0111101);
      Wire.write((uint8_t *)&dataBytes, dataSize);
      Wire.endTransmission();
      while (1);
    }

  }
}

void drawCube()
{
  r[0] = r[0] + PI / 180.0; // Add a degree
  r[1] = r[1] + PI / 180.0; // Add a degree
  r[2] = r[2] + PI / 180.0; // Add a degree
  if (r[0] >= 360.0 * PI / 180.0)
    r[0] = 0;
  if (r[1] >= 360.0 * PI / 180.0)
    r[1] = 0;
  if (r[2] >= 360.0 * PI / 180.0)
    r[2] = 0;

  for (int i = 0; i < 8; i++)
  {
    float px2 = px[i];
    float py2 = cos(r[0]) * py[i] - sin(r[0]) * pz[i];
    float pz2 = sin(r[0]) * py[i] + cos(r[0]) * pz[i];

    float px3 = cos(r[1]) * px2 + sin(r[1]) * pz2;
    float py3 = py2;
    float pz3 = -sin(r[1]) * px2 + cos(r[1]) * pz2;

    float ax = cos(r[2]) * px3 - sin(r[2]) * py3;
    float ay = sin(r[2]) * px3 + cos(r[2]) * py3;
    float az = pz3 - 150;

    p2x[i] = SCREEN_WIDTH / 2 + ax * SHAPE_SIZE / az;
    p2y[i] = SCREEN_HEIGHT / 2 + ay * SHAPE_SIZE / az;
  }

  oled.clear(PAGE);
  for (int i = 0; i < 3; i++)
  {
    oled.line(p2x[i], p2y[i], p2x[i + 1], p2y[i + 1]);
    oled.line(p2x[i + 4], p2y[i + 4], p2x[i + 5], p2y[i + 5]);
    oled.line(p2x[i], p2y[i], p2x[i + 4], p2y[i + 4]);
  }
  oled.line(p2x[3], p2y[3], p2x[0], p2y[0]);
  oled.line(p2x[7], p2y[7], p2x[4], p2y[4]);
  oled.line(p2x[3], p2y[3], p2x[7], p2y[7]);
  oled.display();
}
