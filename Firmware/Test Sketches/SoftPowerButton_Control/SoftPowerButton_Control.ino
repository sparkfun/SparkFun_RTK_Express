/*
  
  This example assume the Fast Off pin is connected.

  A simple tap of the power button will turn on the system.

  If the power button is not being pressed (accidental tap) the system will
  turn off after ~20ms.

  If the system turns on and sees the power button held for 0.5s, it will begin
  normal operation.

  During normal system operation, if user presses the power button for 2s, the system
  will shut down after an additional ~1500ms. So total turn off time is 3.5s
  
  Power on tap to turn off: 2453, 3694, 2932, 3219
  Software turnoff: 1386, 1135, 1440, 747, 1494, 1549
  Override hold time: 5599, 5052, 5596, 5055

  20-30nA at 70F
  Support charging, 120mV drop @ 180mA
*/

//v16
int POWER_CTRL = 13; //Input
int FAST_OFF = 27;
int STAT_LED = 12;

//Breakout board
//int POWER_CTRL = 14; //Input
//int FAST_OFF = 32;
//int STAT_LED = 13;

long powerPressedStartTime = 0;

int debounceDelay = 20;

//Uncomment the following line to turn on shutdown time measurements
//#define PRINT_TIMER_OUTPUT

void setup()
{
  Serial.begin(115200);
  Serial.println("Soft power switch testing");

  pinMode(POWER_CTRL, INPUT_PULLUP);

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

  //Here we display something to user indicating system is on and running
  //For example an external display or LED turns on
  pinMode(STAT_LED, OUTPUT);
  digitalWrite(STAT_LED, HIGH);

  Serial.println("Press 'r' to enter infinite loop to test power-down override");
  Serial.println("Press 'z' to do a soft powerdown");
  Serial.println("Press and hold power button for 2.5 to do a soft powerdown");
}

void loop()
{
  if (Serial.available())
  {
    byte incoming = Serial.read();

    if (incoming == 'z')
    {
      Serial.println("Powering down");
      powerDown();
    }
    else if (incoming == 'r')
    {
      Serial.println("System locked. Now hold power button to force power down without using software.");

      //Here we wait for user press button so we can time it
      while (digitalRead(POWER_CTRL) == HIGH) delay(1);

      powerPressedStartTime = millis();
      Serial.print("Doing nothing, waiting for power override to kick in");
      while (1)
      {
#ifdef PRINT_TIMER_OUTPUT
        Serial.println(millis() - powerPressedStartTime);
#endif
        delay(1);

        if(digitalRead(POWER_CTRL) == HIGH) break;
      }
      Serial.println("User released button before forced powered could complete. Try again, but hold power button for 10s.");
    }
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
