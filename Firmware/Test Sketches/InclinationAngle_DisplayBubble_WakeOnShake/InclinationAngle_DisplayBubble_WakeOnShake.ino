/*
  From BMA220 5.6
  https://media.digikey.com/pdf/Data%20Sheets/Bosch/BMA220.pdf

  Device is turning on every ~3s
  22uF cap?
  No USB connected
  Something is pulling the power btn low:
  * ESP16 Fast off - no, still cycling with 16 disconnected
  * ESP13 Power Ctrl - trace cut, no we need to modify how we check power up/down
  *   Still turning on after 18s
  * Charge LED? Leaking back through the reverse diode D3? No.  
  * Is VRAW voltage do something odd? Not really. 4.19V when off, 3.93V when turned back on
  * Remove VRAW connection to battery charger IC. Nope.
  * POWER BTN is not going low during power off. Stays at 4.1V.
  * Is it C23? Change from 0.1 to 1.0uF. No, still turns on. Forced power down seems shorter tho.
  * Replace c20 from 22uF to 10uF: No. Force power down is shorter, 10s, but bounce on is still 17s. (All with 1uF C23)
  * RC time constant shows 10Mega ohm? Is D1 still in the circuit? No, it's not. So we aren't leaking back through D1.
  * 
  * Re-attach fast off, bounce is now 10s? Detach is 17s. Adding 1k inline is still 10s (no help).
  * Change R22 to 100k. Bounce increases to 15s
  * Increase fast off resistor to 10k, bounce is 17s so not mcuh help.
  * 
  * Ok let's back out.
  * Re-attach charge LED:
  * Re connect charge IC, 20s so we are doing good
  * Accel is connected to button
  * Re-connect ESP32 to power button line - very short bounce, 3s
  * Adding 10k to power button line
  * 
  * 
  *   Ok
  *   Charge IC hooked up
  *   No accel
  *   Fast = 10k
  *   Button ctrl = 10k
  *   C23 = 0.1uF
  *   No USB
  *   No bounce
  *   
  *   
  *   USB - bouncing
  *   No USB - 1 bounce
  *   Detach charge IC, still bounce
  *   
  *   No accel
  *   No Charge IC
  *   Does a software pullup on ctrl cause issues? Pullup bounce is 2s, nope. Disabling internal pullup same 2s bounce
  *   C23 Return to 1.0, bounce is 2
  *   Charge LED removed, bounce is 2
  *   Remove fast off, bounce is 5s
  *   Remove ctrl, bounce is 21s
  *   ADd fast off, bounce is 13s
  *   
  *   No accel
  *   No USB
  *   IC charger
  *   charge LED
  *   No fast off
  *   No ctrl
  *   Bounce is 21s
  *   So the GPIOs are significantly affecting bounce even with 10k inline
  *   Increase inlines to 100k?
  *   Eventually CTRL won't be able to detect hig/low because of divider
  *   Fast off w/ 100k doesn't turn off
  *   
  *   Does bounce occur even when software is just while(1)? Is software causing bounce? Bounce still happens. 22s.
  *   What about C22? Change to 1.0. No change. Bounce is 2s.
  *   Is it peripheral power control related?
  *   Ok, if we have everything hooked up, there is not bounce with a short tap.
  *   But once we have a full power on of peripherals, power down has bounce.
  *   We can unplug CO2 and particle. Not the issue. Still bounce.
  *   Should we give peripheral time to power off? No still bounce.
  *   Is it a GPIO on ESP32 with peripherals off that's draining? Set all to inputs?
  *   Ok, so a power button check will shut down correctly. So run to end of setup doing nothing
  *     No I2C 
  *     Not bouncing with display coming on. Is it the Accel?

  My best guess was the accel was pulling its interrupt pin low causing current flow through ESP32's ACCEL_INT pin. 
  Because the accel was always on via battery, the device would be configured in various states between firmware uploads
  and cause havoc with test firmware. Pulling bat caused accel to reset to POR state (no interrupts).
  Fully disabling interrupts on accel as powerDown() seems to have fixed the issue.


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

  Wire.begin(); //Start wire so that we can config accel before power off
  //Wire.setClock(400000);

  powerOnCheck(); //After serial start in case we want to print

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
  accel.setInt1(true);

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
  accel.setInt1Latch(true);

  //Clear the interrupt
  while(accel.getInt1()) delay(10); //Reading int will clear it

  //Go into 8bit mode
  //accel.setDataRate(LIS2DH12_ODR_1Hz); //Very slow for wake up

  powerDown(true);

  //The larger the avgAmount the faster we should read the sensor
  //accel.setDataRate(LIS2DH12_ODR_100Hz); //6 measurements a second
  accel.setDataRate(LIS2DH12_ODR_400Hz); //25 measurements a second
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
