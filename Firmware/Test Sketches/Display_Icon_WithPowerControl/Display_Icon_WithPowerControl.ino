/*
  Micro OLED Display Icon
  Nathan Seidle @ SparkFun Electronics
  Original Creation Date: November 15, 2020

  Draw a variable sized icon anywhere on the display

  This is helpful when you need a icon (GPS, battery, etc)
  on a certain part of the screen.

  This code is beerware; if you see me (or any other SparkFun
  employee) at the local, and you've found our code helpful,
  please buy us a round!

  Distributed as-is; no warranty is given.
*/
#include <Wire.h>
#include <SFE_MicroOLED.h> //Click here to get the library: http://librarymanager/All#SparkFun_Micro_OLED

//v16
int POWER_CTRL = 13;
int FAST_OFF = 27;
int STAT_LED = 12;
long powerPressedStartTime = 0;
int debounceDelay = 20;

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

  pinMode(POWER_CTRL, INPUT_PULLUP);

  powerCheck();

  Wire.begin();
  Wire.setClock(400000);

  //0x3D is default on Qwiic board
  if (isConnected(0x3D) == true || isConnected(0x3C) == true)
  {
    Serial.println("Display detected");
    displayDetected = true;
  }
  else
  {
    Serial.println("No display detected. Freezing...");
    while (1);
  }

  if (displayDetected)
  {
    oled.begin();     // Initialize the OLED
    oled.clear(PAGE); // Clear the display's internal memory
    oled.clear(ALL);  // Clear the library's display buffer
    oled.display();   // Display what's in the buffer (splashscreen)

  }
}

int iconX = 0;
int iconXChangeAmount = 1;
int iconY = 0;
int iconYChangeAmount = 1;

void loop()
{
  if (displayDetected)
  {
    oled.drawIcon(iconX, iconY, iconWidth, iconHeight, truck, sizeof(truck), true);
    oled.display();

    //Move the icon
    iconX += iconXChangeAmount;
    iconY += iconYChangeAmount;

    if (iconX + iconWidth >= 64)
      iconXChangeAmount *= -1; //Change direction
    if (iconX == 0)
      iconXChangeAmount *= -1; //Change direction

    if (iconY + iconHeight >= 48)
      iconYChangeAmount *= -1; //Change direction
    if (iconY == 0)
      iconYChangeAmount *= -1; //Change direction
  }

  if (digitalRead(POWER_CTRL) == LOW && powerPressedStartTime == 0)
  {
    //Debounce check
    delay(debounceDelay);
    if (digitalRead(POWER_CTRL) == LOW)
    {
      Serial.println("User is pressing power button. Start timer.");
      powerPressedStartTime = millis();
    }
  }
  else if (digitalRead(POWER_CTRL) == LOW && powerPressedStartTime > 0)
  {
    //Debounce check
    delay(debounceDelay);
    if (digitalRead(POWER_CTRL) == LOW)
    {
      if ((millis() - powerPressedStartTime) > 2000)
      {
        Serial.println("Time to power down!");
        powerDown();
      }
    }
  }
  else if (digitalRead(POWER_CTRL) == HIGH && powerPressedStartTime > 0)
  {
    //Debounce check
    delay(debounceDelay);
    if (digitalRead(POWER_CTRL) == HIGH)
    {
      Serial.print("Power button released after ms: ");
      Serial.println(millis() - powerPressedStartTime);
    }
    powerPressedStartTime = 0; //Reset var to return to normal 'on' state
  }
}

void powerCheck()
{
  //User has pressed the power button to turn on the system
  //Was it an accidental bump or do they really want to turn on?
  //Let's make sure they continue to press for two seconds
  Serial.print("Initial power on check.");
  powerPressedStartTime = millis();
  while (digitalRead(POWER_CTRL) == LOW)
  {
    //Wait for user to stop pressing button.
    //What if user has left something heavy pressing the power button?
    //The soft power switch will automatically turn off the system! Handy.
    delay(100);

    if (millis() - powerPressedStartTime > 500)
      break;
    Serial.print(".");
  }
  Serial.println();

  if (millis() - powerPressedStartTime < 500)
  {
    Serial.println("Power button tap. Returning to off state. Powering down.");
    powerDown();
  }

  Serial.println("User wants to turn system on!");
  powerPressedStartTime = 0; //Reset var to return to normal 'on' state
}

void powerDown()
{
  //Indicate to user we are shutting down
  digitalWrite(STAT_LED, LOW);

  pinMode(POWER_CTRL, OUTPUT);
  digitalWrite(POWER_CTRL, LOW);

  pinMode(FAST_OFF, OUTPUT);
  digitalWrite(FAST_OFF, LOW);

  powerPressedStartTime = millis();
  Serial.print("Pulling power line low");
  while (1)
  {
#ifdef PRINT_TIMER_OUTPUT
    Serial.println(millis() - powerPressedStartTime);
#endif
    delay(1);
  }
}
