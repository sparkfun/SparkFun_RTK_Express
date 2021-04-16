/*

*/
const int FIRMWARE_VERSION_MAJOR = 1;
const int FIRMWARE_VERSION_MINOR = 0;

#include <Wire.h>
#include <SFE_MicroOLED.h> //Click here to get the library: http://librarymanager/All#SparkFun_Micro_OLED

#define PIN_RESET 9
#define DC_JUMPER 1 // Set to either 0 (SPI, default) or 1 (I2C) based on jumper, matching the value of the DC Jumper
MicroOLED oled(PIN_RESET, DC_JUMPER);

//A 20 x 17 pixel image of a truck in a box
//Use http://en.radzio.dxp.pl/bitmap_converter/ to generate output
//Make sure the bitmap is n*8 pixels tall (pad white pixels to lower area as needed)
//Otherwise the bitmap bitmap_converter will compress some of the bytes together
uint8_t truck [] = {
  0xFF, 0x01, 0xC1, 0x41, 0x41, 0x41, 0x71, 0x11, 0x11, 0x11, 0x11, 0x11, 0x71, 0x41, 0x41, 0xC1,
  0x81, 0x01, 0xFF, 0xFF, 0x80, 0x83, 0x82, 0x86, 0x8F, 0x8F, 0x86, 0x82, 0x82, 0x82, 0x86, 0x8F,
  0x8F, 0x86, 0x83, 0x81, 0x80, 0xFF,
};

int iconHeight = 17;
int iconWidth = 19;

bool displayDetected = false;

void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println("Display Icon OLED example");

  Wire.begin();
  Wire.setClock(400000);

  beginDisplay();

  while (1) delay(10);
}

void loop()
{
  delay(10);
}
